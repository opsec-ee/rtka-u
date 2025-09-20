/**
 * File rtka_u.c
 * Copyright (c) 2025 - H. Overman <opsec.ee@pm.me>
 * DOI: https://doi.org/10.5281/zenodo.17148691
 * ORCHID: https://orcid.org/0009-0007-9737-762X
 */

/* # Scalar mode
 * gcc -O3 -march=native rtka_u.c -lm -o rtka_scalar

 * # Parallel mode
 * gcc -O3 -march=native -DPARALLEL_ENABLED rtka_u.c -lpthread -lm -o rtka_parallel

 * # Thread safety check
 * gcc -fsanitize=thread -march=native -DPARALLEL_ENABLED rtka_u.c -lpthread -lm -o rtka_tsan
 * or gcc -fsanitize=thread -mavx -DPARALLEL_ENABLED rtka_u.c -lpthread -lm -o rtka_tsan
 *
 * 100000 revolution Stats: TRUE 96848, FALSE 739, UNKNOWN 2413 | Avg 0.000 ms | Total 27.569 ms
 * srand(42U); seed (scalar, parallel, and tsan) produce identical T | F | U regression
*/

#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <stdbool.h>
#include <stdint.h>
#include <time.h>
#include <assert.h>

#ifdef PARALLEL_ENABLED
#include <pthread.h>
#include <immintrin.h>
#include <stdatomic.h>
#include <sched.h>
#ifdef _GNU_SOURCE
#include <numa.h>
#endif
#endif

#ifdef __GNUC__
#define ALWAYS_INLINE __attribute__((always_inline)) inline
#define LIKELY(x) __builtin_expect(!!(x), 1)
#define UNLIKELY(x) __builtin_expect(!!(x), 0)
#define CACHE_ALIGN __attribute__((aligned(64)))
#else
#define ALWAYS_INLINE inline
#define LIKELY(x) (x)
#define UNLIKELY(x) (x)
#define CACHE_ALIGN
#endif

#define CACHE_LINE_SIZE 64U

typedef enum {
    RTKA_FALSE = -1,
    RTKA_UNKNOWN = 0,
    RTKA_TRUE = 1
} rtka_value_t;

typedef enum {
    OP_AND = 0,
    OP_OR = 1,
    OP_NOT = 2,
    OP_IMPLY = 3,
    OP_EQUIV = 4,
    OP_VALUE = 5
} op_type_t;

typedef struct expr_node {
    union {
        struct {
            uint32_t op : 3;
            int32_t value : 2;
            uint32_t visited : 1;
            uint32_t depth : 8;
            uint32_t subtree_size : 10;
            uint32_t height : 4;
            int32_t balance_factor : 3;
            uint32_t heat : 1;
        } bits;
        uint32_t packed;
    };
    float confidence;
    struct expr_node *left;
    struct expr_node *right;
#ifdef PARALLEL_ENABLED
    _Atomic(int) children_complete;
    char padding[CACHE_LINE_SIZE -
                 (sizeof(uint32_t) + sizeof(float) +
                  2 * sizeof(void*) + sizeof(_Atomic(int)))];
#else
    char padding[CACHE_LINE_SIZE -
                 (sizeof(uint32_t) + sizeof(float) +
                  2 * sizeof(void*))];
#endif
} CACHE_ALIGN expr_node_t;

#ifdef PARALLEL_ENABLED
typedef struct {
    _Atomic(float) theta;
    _Atomic(float) alpha;
    _Atomic(float) beta;
    _Atomic(float) sigmoid_k;
    _Atomic(float) x0;
    _Atomic(float) var_threshold;
    _Atomic(bool) adaptive_enabled;
    pthread_mutex_t update_lock;
} threshold_config_t;
#else
typedef struct {
    float theta;
    float alpha;
    float beta;
    float sigmoid_k;
    float x0;
    float var_threshold;
    bool adaptive_enabled;
} threshold_config_t;
#endif

static threshold_config_t g_threshold = {
    .theta = 0.5f,
    .alpha = 1.0f,
    .beta = 1.0f,
    .sigmoid_k = 10.0f,
    .x0 = 0.35f,
    .var_threshold = 0.1f,
    .adaptive_enabled = true
#ifdef PARALLEL_ENABLED
    , .update_lock = PTHREAD_MUTEX_INITIALIZER
#endif
};

#define LUT_SIZE 101U
#define VAR_LUT_SIZE 101U
static float sigmoid_lut[LUT_SIZE] CACHE_ALIGN;
static float var_weight_lut[VAR_LUT_SIZE] CACHE_ALIGN;

static void init_sigmoid_lut(void) __attribute__((constructor));
static void init_sigmoid_lut(void) {
    for (uint32_t i = 0U; i < LUT_SIZE; i++) {
        float x = (float)i / (float)(LUT_SIZE - 1U);
#ifdef PARALLEL_ENABLED
        float k = atomic_load_explicit(&g_threshold.sigmoid_k, memory_order_relaxed);
        float x0 = atomic_load_explicit(&g_threshold.x0, memory_order_relaxed);
#else
        float k = g_threshold.sigmoid_k;
        float x0 = g_threshold.x0;
#endif
        sigmoid_lut[i] = 1.0f / (1.0f + expf(-k * (x - x0)));
    }
}

static void init_var_lut(void) __attribute__((constructor));
static void init_var_lut(void) {
    for (uint32_t i = 0U; i < VAR_LUT_SIZE; i++) {
        float var = (float)i / (float)(VAR_LUT_SIZE - 1U);
        var_weight_lut[i] = 1.0f / (1.0f + var);
    }
}

ALWAYS_INLINE static float sigmoid_lut_interp(float conf) {
    if (UNLIKELY(conf < 0.0f)) return sigmoid_lut[0];
    if (UNLIKELY(conf > 1.0f)) return sigmoid_lut[LUT_SIZE - 1U];

    float idx = conf * (float)(LUT_SIZE - 1U);
    uint32_t low = (uint32_t)idx;
    float frac = idx - (float)low;

    if (LIKELY(low < LUT_SIZE - 1U)) {
        return (1.0f - frac) * sigmoid_lut[low] + frac * sigmoid_lut[low + 1U];
    }
    return sigmoid_lut[LUT_SIZE - 1U];
}

static rtka_value_t apply_threshold_coercion(rtka_value_t value, float confidence) {
#ifdef PARALLEL_ENABLED
    bool enabled = atomic_load_explicit(&g_threshold.adaptive_enabled, memory_order_relaxed);
    float theta = atomic_load_explicit(&g_threshold.theta, memory_order_relaxed);
#else
    bool enabled = g_threshold.adaptive_enabled;
    float theta = g_threshold.theta;
#endif
    if (!enabled) return value;
    if (confidence >= theta) return value;

    float coercion_strength = 1.0f - sigmoid_lut_interp(confidence);
    if (coercion_strength > 0.8f) {
        return RTKA_UNKNOWN;
    }
    return value;
}

static void update_threshold(bool decision_correct) {
#ifdef PARALLEL_ENABLED
    pthread_mutex_lock(&g_threshold.update_lock);
    float old_alpha = atomic_load_explicit(&g_threshold.alpha, memory_order_relaxed);
    float old_beta = atomic_load_explicit(&g_threshold.beta, memory_order_relaxed);
    if (decision_correct) {
        atomic_store_explicit(&g_threshold.alpha, old_alpha + 1.0f, memory_order_relaxed);
        old_alpha += 1.0f;
    } else {
        atomic_store_explicit(&g_threshold.beta, old_beta + 1.0f, memory_order_relaxed);
        old_beta += 1.0f;
    }
    float new_theta = old_alpha / (old_alpha + old_beta);
    float current_theta = atomic_load_explicit(&g_threshold.theta, memory_order_relaxed);
    atomic_store_explicit(&g_threshold.theta, 0.9f * current_theta + 0.1f * new_theta, memory_order_relaxed);
    atomic_store_explicit(&g_threshold.x0, new_theta * 0.7f, memory_order_relaxed);
    init_sigmoid_lut();
    pthread_mutex_unlock(&g_threshold.update_lock);
#else
    if (!g_threshold.adaptive_enabled) return;
    if (decision_correct) {
        g_threshold.alpha += 1.0f;
    } else {
        g_threshold.beta += 1.0f;
    }
    float new_theta = g_threshold.alpha / (g_threshold.alpha + g_threshold.beta);
    g_threshold.theta = 0.9f * g_threshold.theta + 0.1f * new_theta;
    g_threshold.x0 = g_threshold.theta * 0.7f;
    init_sigmoid_lut();
#endif
}

// Core Operations
ALWAYS_INLINE static rtka_value_t rtka_and(rtka_value_t a, rtka_value_t b) {
    return (a < b) ? a : b;
}

ALWAYS_INLINE static rtka_value_t rtka_or(rtka_value_t a, rtka_value_t b) {
    return (a > b) ? a : b;
}

ALWAYS_INLINE static rtka_value_t rtka_not(rtka_value_t a) {
    return -a;
}

ALWAYS_INLINE static rtka_value_t rtka_imply(rtka_value_t a, rtka_value_t b) {
    return rtka_or(rtka_not(a), b);
}

ALWAYS_INLINE static rtka_value_t rtka_equiv(rtka_value_t a, rtka_value_t b) {
    return (a == b) ? RTKA_TRUE : ((a == RTKA_UNKNOWN || b == RTKA_UNKNOWN) ? RTKA_UNKNOWN : RTKA_FALSE);
}

ALWAYS_INLINE static float conf_and(float c1, float c2) {
    return c1 * c2;
}

ALWAYS_INLINE static float conf_or(float c1, float c2) {
    return 1.0f - (1.0f - c1) * (1.0f - c2);
}

ALWAYS_INLINE static float conf_not(float c1) {
    return c1;
}

ALWAYS_INLINE static float conf_imply(float c_ant, float c_cons) {
    return conf_or(1.0f - c_ant, c_cons);
}

ALWAYS_INLINE static float conf_equiv(float c1, float c2) {
    return conf_and(c1, c2);
}

// Scalar Evaluation
static bool evaluate_node_scalar(expr_node_t* node, float threshold) {
    if (UNLIKELY(!node)) return false;

    if (node->left) __builtin_prefetch(node->left, 0, 3);
    if (node->right) __builtin_prefetch(node->right, 0, 3);

    if (node->left) evaluate_node_scalar(node->left, threshold);
    if (node->right) evaluate_node_scalar(node->right, threshold);

    rtka_value_t left_val = node->left ? node->left->bits.value : RTKA_UNKNOWN;
    rtka_value_t right_val = node->right ? node->right->bits.value : RTKA_UNKNOWN;
    float left_conf = node->left ? node->left->confidence : 0.0f;
    float right_conf = node->right ? node->right->confidence : 0.0f;

    switch (node->bits.op) {
        case OP_AND:
            node->bits.value = rtka_and(left_val, right_val);
            node->confidence = conf_and(left_conf, right_conf);
            if (node->bits.value == RTKA_FALSE) return true;
            break;
        case OP_OR:
            node->bits.value = rtka_or(left_val, right_val);
            node->confidence = conf_or(left_conf, right_conf);
            if (node->bits.value == RTKA_TRUE && node->confidence >= threshold) return true;
            break;
        case OP_NOT:
            node->bits.value = rtka_not(left_val);
            node->confidence = conf_not(left_conf);
            break;
        case OP_IMPLY:
            node->bits.value = rtka_imply(left_val, right_val);
            node->confidence = conf_imply(left_conf, right_conf);
            break;
        case OP_EQUIV:
            node->bits.value = rtka_equiv(left_val, right_val);
            node->confidence = conf_equiv(left_conf, right_conf);
            break;
        case OP_VALUE:
            break;
        default:
            node->bits.value = RTKA_UNKNOWN;
            node->confidence = 0.0f;
            return false;
    }

    node->bits.value = apply_threshold_coercion(node->bits.value, node->confidence);
    update_threshold(node->bits.value != RTKA_UNKNOWN);
    return true;
}

rtka_value_t rtka_evaluate_scalar(expr_node_t* root, float threshold) {
    if (UNLIKELY(!root)) return RTKA_UNKNOWN;
    evaluate_node_scalar(root, threshold);
    return root->bits.value;
}

// Tree Metadata
static uint32_t compute_tree_metadata(expr_node_t* node, uint32_t depth) {
    if (UNLIKELY(!node)) return 0U;

    node->bits.depth = depth;
    node->bits.visited = 1U;

    if (node->bits.op == OP_VALUE) {
        node->bits.subtree_size = 1U;
        node->bits.height = 0U;
        node->bits.balance_factor = 0;
        node->bits.heat = (node->confidence > 0.5f) ? 1U : 0U;
        return 1U;
    }

    uint32_t left_size = compute_tree_metadata(node->left, depth + 1U);
    uint32_t right_size = compute_tree_metadata(node->right, depth + 1U);

    node->bits.subtree_size = 1U + left_size + right_size;

    uint32_t left_height = node->left ? node->left->bits.height + 1U : 0U;
    uint32_t right_height = node->right ? node->right->bits.height + 1U : 0U;

    node->bits.height = (left_height > right_height) ? left_height : right_height;
    node->bits.balance_factor = (int32_t)(right_height - left_height);
    node->bits.heat = (left_size + right_size > 5U) ? 1U : 0U;

    return node->bits.subtree_size;
}

// Sensor Fusion
#define NUM_SENSORS 8U
typedef struct {
    rtka_value_t detection;
    float confidence;
    float variance;
} sensor_input_t;

typedef struct {
    rtka_value_t fused;
    float confidence;
} fusion_result_t;

ALWAYS_INLINE static float geometric_mean(const float* restrict confs, uint32_t n) {
    if (UNLIKELY(n == 0U)) return 1.0f;
    float prod = 1.0f;
    for (uint32_t i = 0U; i < n; i++) {
        prod *= confs[i];
    }
    return powf(prod, 1.0f / (float)n);
}

static fusion_result_t fuse_sensors_adaptive(const sensor_input_t* restrict sensors, uint32_t n) {
    fusion_result_t res = {RTKA_UNKNOWN, 0.0f};
    float weighted_sum = 0.0f;
    float weight_sum = 0.0f;
    float conf_prod_or = 1.0f;
    float confs[NUM_SENSORS] = {0.0f};
    uint32_t conf_idx = 0U;
    float total_var = 0.0f;
    bool early_term = false;

    for (uint32_t i = 0U; i < n; i++) {
        uint32_t idx = (uint32_t)(sensors[i].variance * (float)(VAR_LUT_SIZE - 1U));
        float weight = (idx < VAR_LUT_SIZE) ? var_weight_lut[idx] : 0.0f;
        float reweighted = (float)sensors[i].detection * weight * sensors[i].confidence;
        weighted_sum += reweighted;
        weight_sum += sensors[i].confidence;

        conf_prod_or *= (1.0f - sensors[i].confidence);
        if (conf_idx < NUM_SENSORS) confs[conf_idx++] = sensors[i].confidence;
        total_var += sensors[i].variance;

        float theta =
#ifdef PARALLEL_ENABLED
        atomic_load_explicit(&g_threshold.theta, memory_order_relaxed);
#else
        g_threshold.theta;
#endif
        if (sensors[i].detection == RTKA_TRUE && sensors[i].confidence >= theta) {
            early_term = true;
            break;
        }
    }

    float consensus_val = (weight_sum > 0.0f) ? weighted_sum / weight_sum : 0.0f;
    float fused_conf_or = 1.0f - conf_prod_or;

    if (early_term) {
        res.fused = RTKA_TRUE;
        res.confidence = fused_conf_or;
        goto done;
    }

    res.fused = (consensus_val > 0.5f) ? RTKA_TRUE : (consensus_val < -0.5f ? RTKA_FALSE : RTKA_UNKNOWN);
    res.confidence = fused_conf_or;
    if (res.fused == RTKA_UNKNOWN) {
        float geo_mean = geometric_mean(confs, conf_idx);
        float deviation = fabsf(consensus_val) / 0.5f;
        res.confidence = geo_mean * (1.0f - deviation);
    }

    res.fused = apply_threshold_coercion(res.fused, res.confidence);
    update_threshold(res.confidence > 0.5f);

done:
    float avg_var = total_var / (float)n;
#ifdef PARALLEL_ENABLED
    float current_thresh = atomic_load_explicit(&g_threshold.var_threshold, memory_order_relaxed);
    if (avg_var > current_thresh * 2.0f) {
        atomic_store_explicit(&g_threshold.var_threshold, current_thresh * 1.1f, memory_order_relaxed);
    }
#else
    if (avg_var > g_threshold.var_threshold * 2.0f) {
        g_threshold.var_threshold *= 1.1f;
    }
#endif
    return res;
}

// PARALLEL EXTENSIONS
#ifdef PARALLEL_ENABLED

// SIMD Operations
static void simd_conf_and_batch(float* restrict result, const float* restrict c1, const float* restrict c2, size_t n) {
    bool aligned = (((uintptr_t)result & 31U) == 0U) &&
                   (((uintptr_t)c1 & 31U) == 0U) &&
                   (((uintptr_t)c2 & 31U) == 0U);

    size_t i = 0U;
    if (aligned) {
        for (; i + 7U < n; i += 8U) {
            __m256 v1 = _mm256_load_ps(&c1[i]);
            __m256 v2 = _mm256_load_ps(&c2[i]);
            __m256 vr = _mm256_mul_ps(v1, v2);
            _mm256_store_ps(&result[i], vr);
        }
    } else {
        for (; i + 7U < n; i += 8U) {
            __m256 v1 = _mm256_loadu_ps(&c1[i]);
            __m256 v2 = _mm256_loadu_ps(&c2[i]);
            __m256 vr = _mm256_mul_ps(v1, v2);
            _mm256_storeu_ps(&result[i], vr);
        }
    }

    for (; i + 3U < n; i += 4U) {
        result[i] = c1[i] * c2[i];
        result[i + 1U] = c1[i + 1U] * c2[i + 1U];
        result[i + 2U] = c1[i + 2U] * c2[i + 2U];
        result[i + 3U] = c1[i + 3U] * c2[i + 3U];
    }

    for (; i < n; i++) {
        result[i] = c1[i] * c2[i];
    }
}

// Work-Stealing Deque
#define DEQUE_CAPACITY 1024U
#define DEQUE_MASK (DEQUE_CAPACITY - 1U)

typedef struct {
    expr_node_t** tasks CACHE_ALIGN;
    _Atomic(int64_t) top;
    _Atomic(int64_t) bottom;
    size_t capacity;
} ws_deque_t;

static ws_deque_t* deque_create(size_t capacity) {
    ws_deque_t* deque =
#ifdef _GNU_SOURCE
    (numa_available() >= 0) ? numa_alloc_onnode(sizeof(ws_deque_t), numa_node_of_cpu(sched_getcpu())) :
#endif
    aligned_alloc(CACHE_LINE_SIZE, sizeof(ws_deque_t));
    if (UNLIKELY(!deque)) return NULL;
    deque->tasks =
#ifdef _GNU_SOURCE
    (numa_available() >= 0) ? numa_alloc_onnode(capacity * sizeof(expr_node_t*), numa_node_of_cpu(sched_getcpu())) :
#endif
    calloc(capacity, sizeof(expr_node_t*));
    if (UNLIKELY(!deque->tasks)) {
        free(deque);
        return NULL;
    }
    deque->capacity = capacity;
    atomic_store_explicit(&deque->top, 0, memory_order_relaxed);
    atomic_store_explicit(&deque->bottom, 0, memory_order_relaxed);
    return deque;
}

static void deque_destroy(ws_deque_t* deque) {
    if (!deque) return;
#ifdef _GNU_SOURCE
    if (numa_available() >= 0) {
        if (deque->tasks) {
            numa_free(deque->tasks, deque->capacity * sizeof(expr_node_t*));
        }
        numa_free(deque, sizeof(ws_deque_t));
    } else
#endif
    {
        free(deque->tasks);
        free(deque);
    }
}

static void deque_push_bottom(ws_deque_t* restrict deque, expr_node_t* task) {
    if (UNLIKELY(!deque || !task)) return;
    int64_t b = atomic_load_explicit(&deque->bottom, memory_order_relaxed);
    deque->tasks[(size_t)b & DEQUE_MASK] = task;
    atomic_thread_fence(memory_order_release);
    atomic_store_explicit(&deque->bottom, b + 1, memory_order_relaxed);
}

static expr_node_t* deque_pop_bottom(ws_deque_t* restrict deque) {
    if (UNLIKELY(!deque)) return NULL;
    int64_t b = atomic_load_explicit(&deque->bottom, memory_order_relaxed) - 1;
    atomic_store_explicit(&deque->bottom, b, memory_order_relaxed);
    atomic_thread_fence(memory_order_seq_cst);

    int64_t t = atomic_load_explicit(&deque->top, memory_order_relaxed);
    if (t <= b) {
        expr_node_t* task = deque->tasks[(size_t)b & DEQUE_MASK];
        if (t == b) {
            int64_t expected = t;
            if (!atomic_compare_exchange_strong_explicit(&deque->top, &expected, t + 1, memory_order_seq_cst, memory_order_relaxed)) {
                task = NULL;
            }
            atomic_store_explicit(&deque->bottom, b + 1, memory_order_relaxed);
        }
        return task;
    } else {
        atomic_store_explicit(&deque->bottom, b + 1, memory_order_relaxed);
        return NULL;
    }
}

static expr_node_t* deque_steal(ws_deque_t* restrict deque) {
    if (UNLIKELY(!deque)) return NULL;
    int64_t t = atomic_load_explicit(&deque->top, memory_order_acquire);
    atomic_thread_fence(memory_order_seq_cst);
    int64_t b = atomic_load_explicit(&deque->bottom, memory_order_acquire);

    if (t < b) {
        expr_node_t* task = deque->tasks[(size_t)t & DEQUE_MASK];
        int64_t expected = t;
        if (atomic_compare_exchange_strong_explicit(&deque->top, &expected, t + 1, memory_order_seq_cst, memory_order_relaxed)) {
            return task;
        }
    }
    return NULL;
}

// Node Index Mapping
typedef struct {
    expr_node_t** node_map;
    size_t map_size;
    pthread_mutex_t map_lock;
} node_index_map_t;

static size_t get_node_index(node_index_map_t* map, expr_node_t* node) {
    if (UNLIKELY(!map || !node)) return 0;
    pthread_mutex_lock(&map->map_lock);
    for (size_t i = 0U; i < map->map_size; i++) {
        if (map->node_map[i] == node) {
            pthread_mutex_unlock(&map->map_lock);
            return i;
        }
    }
    pthread_mutex_unlock(&map->map_lock);
    return 0;
}

static void build_node_index_map(expr_node_t* node, expr_node_t** map, size_t* current_idx) {
    if (UNLIKELY(!node || !map || !current_idx)) return;
    build_node_index_map(node->left, map, current_idx);
    build_node_index_map(node->right, map, current_idx);
    map[(*current_idx)++] = node;
}

// Type definitions - FIXED ORDER
typedef struct {
    rtka_value_t value;
    float confidence;
    _Atomic(bool) ready;
} node_result_t;

typedef struct {
    int thread_id;
    ws_deque_t* local_deque;
    ws_deque_t** all_deques;
    int num_threads;
    node_result_t* results;
    node_index_map_t* index_map;
    _Atomic(bool)* should_terminate;  // Graceful termination
    _Atomic(int)* active_workers;     // Track active workers
} worker_context_t;

// Forward declaration
static bool evaluate_node_parallel(expr_node_t* node,
                                   node_result_t* results,
                                   size_t my_idx,
                                   node_index_map_t* index_map);

// Worker thread with graceful termination
static void* worker_thread(void* arg) {
    worker_context_t* ctx = (worker_context_t*)arg;
    if (UNLIKELY(!ctx)) return NULL;

    atomic_fetch_add(ctx->active_workers, 1);

    while (!atomic_load_explicit(ctx->should_terminate, memory_order_acquire)) {
        expr_node_t* node = deque_pop_bottom(ctx->local_deque);
        if (!node) {
            bool stole = false;
            for (int j = 0; j < ctx->num_threads; j++) {
                if (j == ctx->thread_id) continue;
                node = deque_steal(ctx->all_deques[j]);
                if (node) {
                    stole = true;
                    break;
                }
            }
            if (!stole) {
                // No work available, check if others are done
                if (atomic_load_explicit(ctx->active_workers, memory_order_acquire) == 1) {
                    // I'm the last active worker, signal termination
                    atomic_store_explicit(ctx->should_terminate, true, memory_order_release);
                    break;
                }
                // Sleep briefly
                atomic_fetch_sub(ctx->active_workers, 1);
                struct timespec ts = {.tv_sec = 0, .tv_nsec = 10000000L};  // 10ms
                nanosleep(&ts, NULL);
                atomic_fetch_add(ctx->active_workers, 1);
                continue;
            }
        }

        size_t my_idx = get_node_index(ctx->index_map, node);
        evaluate_node_parallel(node, ctx->results, my_idx, ctx->index_map);
    }

    atomic_fetch_sub(ctx->active_workers, 1);
    return NULL;
}

// Exponential backoff
static void wait_with_backoff(_Atomic(bool)* flag) {
    uint32_t spin_count = 0U;
    const uint32_t max_short_spins = 32U;
    const uint32_t max_spins = 1000U;

    while (!atomic_load_explicit(flag, memory_order_acquire)) {
        if (spin_count < max_short_spins) {
            for (uint32_t i = 0U; i < (1U << spin_count); i++) {
                __builtin_ia32_pause();
            }
            spin_count++;
        } else if (spin_count < max_spins) {
            sched_yield();
            spin_count++;
        } else {
            struct timespec ts = {.tv_sec = 0, .tv_nsec = 1000000L};
            nanosleep(&ts, NULL);
        }
    }
}

static bool evaluate_node_parallel(expr_node_t* node, node_result_t* results, size_t my_idx, node_index_map_t* index_map) {
      if (UNLIKELY(!node || !results)) return false;

      if (node->left) __builtin_prefetch(node->left, 0, 3);
      if (node->right) __builtin_prefetch(node->right, 0, 3);

      if (node->bits.op == OP_VALUE) {
        results[my_idx].value = apply_threshold_coercion(node->bits.value, node->confidence);
        results[my_idx].confidence = node->confidence;
        atomic_store_explicit(&results[my_idx].ready, true, memory_order_release);
        return true;
      }

      // Only look up and wait for children that exist
      rtka_value_t left_val = RTKA_UNKNOWN;
      rtka_value_t right_val = RTKA_UNKNOWN;
      float left_conf = 0.0f;
      float right_conf = 0.0f;

      if (node->left) {
        size_t left_idx = get_node_index(index_map, node->left);
        wait_with_backoff(&results[left_idx].ready);
        left_val = results[left_idx].value;
        left_conf = results[left_idx].confidence;
      }

      if (node->right) {
        size_t right_idx = get_node_index(index_map, node->right);
        wait_with_backoff(&results[right_idx].ready);
        right_val = results[right_idx].value;
        right_conf = results[right_idx].confidence;
      }

      switch (node->bits.op) {
        case OP_AND:
            results[my_idx].value = rtka_and(left_val, right_val);
            results[my_idx].confidence = conf_and(left_conf, right_conf);
            break;
        case OP_OR:
            results[my_idx].value = rtka_or(left_val, right_val);
            results[my_idx].confidence = conf_or(left_conf, right_conf);
            break;
        case OP_NOT:
            results[my_idx].value = rtka_not(left_val);
            results[my_idx].confidence = conf_not(left_conf);
            break;
        case OP_IMPLY:
            results[my_idx].value = rtka_imply(left_val, right_val);
            results[my_idx].confidence = conf_imply(left_conf, right_conf);
            break;
        case OP_EQUIV:
            results[my_idx].value = rtka_equiv(left_val, right_val);
            results[my_idx].confidence = conf_equiv(left_conf, right_conf);
            break;
        case OP_VALUE:
            results[my_idx].value = node->bits.value;
            results[my_idx].confidence = node->confidence;
            break;
        default:
            results[my_idx].value = RTKA_UNKNOWN;
            results[my_idx].confidence = 0.0f;
            atomic_store_explicit(&results[my_idx].ready, true, memory_order_release);
            return false;
    }

    results[my_idx].value = apply_threshold_coercion(results[my_idx].value, results[my_idx].confidence);
    atomic_store_explicit(&results[my_idx].ready, true, memory_order_release);
    return true;
}

static void enqueue_tree_postorder(ws_deque_t* deque, expr_node_t* node) {
    if (UNLIKELY(!node || !deque)) return;
    enqueue_tree_postorder(deque, node->left);
    enqueue_tree_postorder(deque, node->right);
    deque_push_bottom(deque, node);
}

rtka_value_t rtka_evaluate_parallel(expr_node_t* root, float threshold, int num_threads) {
  if (UNLIKELY(!root || num_threads < 1)) return RTKA_UNKNOWN;

  compute_tree_metadata(root, 0U);
  size_t num_nodes = root->bits.subtree_size;

  node_result_t* results = calloc(num_nodes, sizeof(node_result_t));
  expr_node_t** node_map = calloc(num_nodes, sizeof(expr_node_t*));
  node_index_map_t* index_map = calloc(1, sizeof(node_index_map_t));

  if (UNLIKELY(!results || !node_map || !index_map)) {
    free(results); free(node_map); free(index_map);
    return RTKA_UNKNOWN;
  }

  size_t idx = 0U;
  build_node_index_map(root, node_map, &idx);
  index_map->node_map = node_map;
  index_map->map_size = num_nodes;
  pthread_mutex_init(&index_map->map_lock, NULL);

  ws_deque_t** deques = calloc((size_t)num_threads, sizeof(ws_deque_t*));
  pthread_t* threads = calloc((size_t)num_threads, sizeof(pthread_t));
  worker_context_t* ctxs = calloc((size_t)num_threads, sizeof(worker_context_t));
  _Atomic(bool) should_terminate = false;
  _Atomic(int) active_workers = 0;

  if (UNLIKELY(!deques || !threads || !ctxs)) {
    pthread_mutex_destroy(&index_map->map_lock);
    free(results); free(node_map); free(index_map);
    free(deques); free(threads); free(ctxs);
    return RTKA_UNKNOWN;
  }

  // First: Create all deques
  int allocated_deques = 0;
  for (int t = 0; t < num_threads; t++) {
    deques[t] = deque_create(DEQUE_CAPACITY);
    if (UNLIKELY(!deques[t])) {
      for (int i = 0; i < allocated_deques; i++) {
        deque_destroy(deques[i]);
      }
      pthread_mutex_destroy(&index_map->map_lock);
      free(results); free(node_map); free(index_map);
      free(deques); free(threads); free(ctxs);
      return RTKA_UNKNOWN;
    }
    allocated_deques++;
  }

  // Second: Enqueue work BEFORE creating threads
  enqueue_tree_postorder(deques[0], root);

  // Third: Create worker threads
  for (int t = 0; t < num_threads; t++) {
    ctxs[t].thread_id = t;
    ctxs[t].local_deque = deques[t];
    ctxs[t].all_deques = deques;
    ctxs[t].num_threads = num_threads;
    ctxs[t].results = results;
    ctxs[t].index_map = index_map;
    ctxs[t].should_terminate = &should_terminate;
    ctxs[t].active_workers = &active_workers;

    int rc = pthread_create(&threads[t], NULL, worker_thread, &ctxs[t]);
    if (UNLIKELY(rc != 0)) {
      atomic_store_explicit(&should_terminate, true, memory_order_release);
      for (int i = 0; i < t; i++) {
        pthread_join(threads[i], NULL);
      }
      for (int i = 0; i < allocated_deques; i++) {
        deque_destroy(deques[i]);
      }
      pthread_mutex_destroy(&index_map->map_lock);
      free(results); free(node_map); free(index_map);
      free(deques); free(threads); free(ctxs);
      return RTKA_UNKNOWN;
    }
  }

  // Wait for completion with timeout
  struct timespec timeout = {.tv_sec = 5, .tv_nsec = 0};
  struct timespec start_time;
  clock_gettime(CLOCK_MONOTONIC, &start_time);

  while (!atomic_load_explicit(&results[num_nodes - 1].ready, memory_order_acquire)) {
    struct timespec now;
    clock_gettime(CLOCK_MONOTONIC, &now);
    if (now.tv_sec - start_time.tv_sec > timeout.tv_sec) {
      atomic_store_explicit(&should_terminate, true, memory_order_release);
      break;
    }
    struct timespec sleep_time = {.tv_sec = 0, .tv_nsec = 1000000L};
    nanosleep(&sleep_time, NULL);
  }

  atomic_store_explicit(&should_terminate, true, memory_order_release);

  for (int t = 0; t < num_threads; t++) {
    pthread_join(threads[t], NULL);
  }

  rtka_value_t result = results[num_nodes - 1].value;

  for (int t = 0; t < num_threads; t++) {
    deque_destroy(deques[t]);
  }
  pthread_mutex_destroy(&index_map->map_lock);
  free(deques);
  free(threads);
  free(ctxs);
  free(results);
  free(node_map);
  free(index_map);

  return result;
}

#endif // PARALLEL_ENABLED

// Testing Functions
static void test_operators(void) {
    assert(rtka_imply(RTKA_TRUE, RTKA_FALSE) == RTKA_FALSE);
    assert(rtka_imply(RTKA_FALSE, RTKA_FALSE) == RTKA_TRUE);
    assert(rtka_imply(RTKA_UNKNOWN, RTKA_TRUE) == RTKA_TRUE);
    assert(rtka_equiv(RTKA_TRUE, RTKA_TRUE) == RTKA_TRUE);
    assert(rtka_equiv(RTKA_TRUE, RTKA_FALSE) == RTKA_FALSE);
    assert(rtka_equiv(RTKA_UNKNOWN, RTKA_TRUE) == RTKA_UNKNOWN);
    printf("Operator tests passed\n");
}

static void test_tree_operations(void) {
    expr_node_t* root = calloc(1, sizeof(expr_node_t));
    expr_node_t* or_left = calloc(1, sizeof(expr_node_t));
    expr_node_t* or_right = calloc(1, sizeof(expr_node_t));
    expr_node_t* and_left = calloc(1, sizeof(expr_node_t));
    expr_node_t* and_right = calloc(1, sizeof(expr_node_t));
    expr_node_t* not_child = calloc(1, sizeof(expr_node_t));

    if (!root || !or_left || !or_right || !and_left || !and_right || !not_child) {
        free(root); free(or_left); free(or_right);
        free(and_left); free(and_right); free(not_child);
        return;
    }

    root->bits.op = OP_OR;
    root->left = or_left;
    root->right = or_right;

    or_left->bits.op = OP_AND;
    or_left->left = and_left;
    or_left->right = and_right;

    and_left->bits.op = OP_VALUE;
    and_left->bits.value = RTKA_TRUE;
    and_left->confidence = 0.9f;

    and_right->bits.op = OP_VALUE;
    and_right->bits.value = RTKA_UNKNOWN;
    and_right->confidence = 0.5f;

    or_right->bits.op = OP_NOT;
    or_right->left = not_child;

    not_child->bits.op = OP_VALUE;
    not_child->bits.value = RTKA_FALSE;
    not_child->confidence = 0.8f;

#ifdef PARALLEL_ENABLED
    rtka_value_t result = rtka_evaluate_parallel(root, 0.5f, 4);
#else
    rtka_value_t result = rtka_evaluate_scalar(root, 0.5f);
#endif

    assert(result == RTKA_TRUE);
    free(and_left); free(and_right); free(or_left);
    free(not_child); free(or_right); free(root);
    printf("Complex tree test passed\n");
}

static void test_parallel_tree(void) {
    expr_node_t* root = calloc(1, sizeof(expr_node_t));
    expr_node_t* left = calloc(1, sizeof(expr_node_t));
    expr_node_t* right = calloc(1, sizeof(expr_node_t));

    if (!root || !left || !right) {
        free(root); free(left); free(right);
        return;
    }

    root->bits.op = OP_AND;
    root->left = left;
    root->right = right;

    left->bits.op = OP_VALUE;
    left->bits.value = RTKA_TRUE;
    left->confidence = 0.9f;

    right->bits.op = OP_VALUE;
    right->bits.value = RTKA_UNKNOWN;
    right->confidence = 0.5f;

#ifdef PARALLEL_ENABLED
    rtka_value_t result = rtka_evaluate_parallel(root, 0.5f, 4);
#else
    rtka_value_t result = rtka_evaluate_scalar(root, 0.5f);
#endif
    assert(result == RTKA_UNKNOWN);

    free(left); free(right); free(root);
}

int main(void) {
    srand(42U);
    init_sigmoid_lut();
    init_var_lut();

    printf("RTKA-U Enhanced Demo v1.3\n");
    printf("=========================\n");

    test_operators();
    test_tree_operations();
    test_parallel_tree();

    uint32_t true_count = 0U, false_count = 0U, unknown_count = 0U;
    double avg_time = 0.0;
    uint32_t trials = 100000U;

    struct timespec start_all = {0}, end_all = {0};
    clock_gettime(CLOCK_MONOTONIC, &start_all);

    for (uint32_t trial = 0U; trial < trials; trial++) {
        sensor_input_t sensors[NUM_SENSORS] = {0};

        for (uint32_t i = 0U; i < NUM_SENSORS; i++) {
            double r = (double)rand() / (double)RAND_MAX;
            sensors[i].detection = (r < 0.35) ? RTKA_FALSE : (r < 0.65 ? RTKA_UNKNOWN : RTKA_TRUE);
            sensors[i].confidence = (float)(r * 0.8 + 0.2);
            sensors[i].variance = (float)(r * 0.2);
        }

        struct timespec start = {0}, end = {0};
        clock_gettime(CLOCK_MONOTONIC, &start);
        fusion_result_t res = fuse_sensors_adaptive(sensors, NUM_SENSORS);
        clock_gettime(CLOCK_MONOTONIC, &end);

        double time_ms = (double)(end.tv_sec - start.tv_sec) * 1000.0 +
                         (double)(end.tv_nsec - start.tv_nsec) / 1e6;
        avg_time += time_ms;

        switch (res.fused) {
            case RTKA_TRUE: true_count++; break;
            case RTKA_FALSE: false_count++; break;
            case RTKA_UNKNOWN: unknown_count++; break;
            default: break;
        }

        if (trial % 1000U == 0U) {
            printf("Trial %u: %s (conf: %.3f, time: %.3f ms)\n",
                   trial, res.fused == RTKA_TRUE ? "TRUE" : 
                   (res.fused == RTKA_FALSE ? "FALSE" : "UNKNOWN"),
                   res.confidence, time_ms);
        }
    }

    clock_gettime(CLOCK_MONOTONIC, &end_all);
    double total_time = (double)(end_all.tv_sec - start_all.tv_sec) * 1000.0 +
                        (double)(end_all.tv_nsec - start_all.tv_nsec) / 1e6;
    avg_time /= (double)trials;

    printf("\nStats: TRUE %u, FALSE %u, UNKNOWN %u | Avg %.3f ms | Total %.3f ms\n",
           true_count, false_count, unknown_count, avg_time, total_time);
    printf("UNKNOWN rate: %.2f%%\n", (double)unknown_count / (double)trials * 100.0);
#ifdef PARALLEL_ENABLED
    printf("Parallel: Enabled with fixes.\n");
#else
    printf("Scalar: Enabled.\n");
#endif

    return 0;
}
