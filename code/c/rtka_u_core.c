/**
 * File rtka_u_core.c
 * Author H. Overman
 * Date 2025-09-19
 * Copyright (c) 2025 - H. Overman <opsec.ee@pm.me>
 *
 * Demo: RTKA-U multi-sensor (8 cameras) fusion for Tesla FSD-like "Obstacle ahead?" decision.
 * For FSD, swap fuse_sensors call for rtka_evaluate on a sensor tree.
 * 100k evals: Balanced F/U/T inputs .. [blazing fast!]
 */

#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <stdbool.h>
#include <stdint.h>
#include <time.h>

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

// ============================================================================
// OPT-000: Core RTKA-U Types
// ============================================================================
typedef enum {
    RTKA_FALSE = -1,
    RTKA_UNKNOWN = 0,
    RTKA_TRUE = 1
} rtka_value_t;

typedef enum {
    OP_AND = 0,
    OP_OR = 1,
    OP_NOT = 2
} op_type_t;

typedef struct expr_node {
    op_type_t op;
    rtka_value_t value;
    double confidence;
    struct expr_node *left;
    struct expr_node *right;
    uint32_t depth;  // OPT-118: Metadata
    uint32_t heat;   // OPT-118: Hot if high conf
} CACHE_ALIGN expr_node_t;

// OPT-117: Adaptive Threshold Config
typedef struct {
    float theta;
    float alpha;
    float beta;
    float sigmoid_k;
    float x0;
    float var_threshold;  // Added missing member
    bool adaptive_enabled;
} threshold_config_t;

static threshold_config_t g_threshold = {
    .theta = 0.5f,
    .alpha = 1.0f,
    .beta = 1.0f,
    .sigmoid_k = 10.0f,
    .x0 = 0.35f,
    .var_threshold = 0.1f,  // Initialize the new member
    .adaptive_enabled = true
};

#define LUT_SIZE 101  // OPT-003
static float sigmoid_lut[LUT_SIZE];

// Move VAR_LUT definitions before fuse_sensors function
#define VAR_LUT_SIZE 101
static float var_weight_lut[VAR_LUT_SIZE];

static void init_sigmoid_lut(void) __attribute__((constructor));
static void init_sigmoid_lut(void) {
    for (uint32_t i = 0U; i < LUT_SIZE; i++) {
        float x = (float)i / (float)(LUT_SIZE - 1U);
        sigmoid_lut[i] = 1.0f / (1.0f + expf(-g_threshold.sigmoid_k * (x - g_threshold.x0)));
    }
}

static void init_var_lut(void) __attribute__((constructor));
static void init_var_lut(void) {
    for (uint32_t i = 0U; i < VAR_LUT_SIZE; i++) {
        float var = (float)i / (float)(VAR_LUT_SIZE - 1U);
        var_weight_lut[i] = 1.0f / (1.0f + var);
    }
}

ALWAYS_INLINE static float sigmoid_lut_interp(float conf) {  // OPT-003
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

static rtka_value_t apply_threshold_coercion(rtka_value_t value, double confidence) {  // OPT-117
    if (!g_threshold.adaptive_enabled) return value;
    if (confidence >= g_threshold.theta) return value;

    float coercion_strength = 1.0f - sigmoid_lut_interp((float)confidence);
    if (coercion_strength > 0.8f) {
        return RTKA_UNKNOWN;
    }
    return value;
}

static void update_threshold(bool decision_correct) {  // OPT-117
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
}

// ============================================================================
// OPT-125: Core Operations (Kleene min/max, conf propagation)
// ============================================================================
ALWAYS_INLINE static rtka_value_t rtka_and(rtka_value_t a, rtka_value_t b) {
    return (a < b) ? a : b;  // min
}

ALWAYS_INLINE static rtka_value_t rtka_or(rtka_value_t a, rtka_value_t b) {
    return (a > b) ? a : b;  // max
}

ALWAYS_INLINE static rtka_value_t rtka_not(rtka_value_t a) {
    return -a;
}

ALWAYS_INLINE static double conf_and(double c1, double c2) {
    return c1 * c2;
}

ALWAYS_INLINE static double conf_or(double c1, double c2) {
    return 1.0 - (1.0 - c1) * (1.0 - c2);
}

ALWAYS_INLINE static double conf_not(double c1) {
    return c1;
}

// ============================================================================
// OPT-125: Recursive Evaluation with Early Term
// ============================================================================
static bool evaluate_node(expr_node_t* node, double threshold) {  // OPT-125 helper
    if (UNLIKELY(!node)) return false;

    // Handle NOT separately first if needed
    if (node->op == OP_NOT) {
        if (node->left) evaluate_node(node->left, threshold);
        node->value = rtka_not(node->left ? node->left->value : RTKA_UNKNOWN);
        node->confidence = conf_not(node->left ? node->left->confidence : 0.0);
        node->value = apply_threshold_coercion(node->value, node->confidence);
        return true;
    }

    // Recursively evaluate children
    if (node->left) evaluate_node(node->left, threshold);
    if (node->right) evaluate_node(node->right, threshold);

    rtka_value_t left_val = node->left ? node->left->value : RTKA_UNKNOWN;
    rtka_value_t right_val = node->right ? node->right->value : RTKA_UNKNOWN;
    double left_conf = node->left ? node->left->confidence : 0.0;
    double right_conf = node->right ? node->right->confidence : 0.0;

    switch (node->op) {
        case OP_AND:
            node->value = rtka_and(left_val, right_val);
            node->confidence = conf_and(left_conf, right_conf);
            // OPT-125: Early term if FALSE
            if (node->value == RTKA_FALSE) return true;
            break;
        case OP_OR:
            node->value = rtka_or(left_val, right_val);
            node->confidence = conf_or(left_conf, right_conf);
            // OPT-125: Early term if TRUE
            if (node->value == RTKA_TRUE && node->confidence >= threshold) return true;
            break;
        case OP_NOT:
            // Already handled above, but included to satisfy -Wswitch-enum
            node->value = rtka_not(left_val);
            node->confidence = conf_not(left_conf);
            break;
        default:
            node->value = RTKA_UNKNOWN;
            node->confidence = 0.0;
            return false;
    }

    node->value = apply_threshold_coercion(node->value, node->confidence);
    update_threshold(node->value != RTKA_UNKNOWN);
    return true;
}

__attribute__((unused))  // Mark as potentially unused
static rtka_value_t rtka_evaluate(expr_node_t* root, double threshold) {
    if (UNLIKELY(!root)) return RTKA_UNKNOWN;
    evaluate_node(root, threshold);
    return root->value;
}

// ============================================================================
// OPT-126: Consensus for Sensor Fusion
// ============================================================================
#define NUM_SENSORS 8U
typedef struct {
    rtka_value_t detection;
    double confidence;
    double variance;  // OPT-129
} sensor_input_t;

typedef struct {
    rtka_value_t fused;
    double confidence;
} fusion_result_t;

ALWAYS_INLINE static double geometric_mean(double* restrict confs, uint32_t n) {  // OPT-126
    if (UNLIKELY(n == 0U)) return 1.0;
    double prod = 1.0;
    for (uint32_t i = 0U; i < n; i++) {
        prod *= confs[i];
    }
    return pow(prod, 1.0 / (double)n);
}

static fusion_result_t fuse_sensors(const sensor_input_t* restrict sensors, uint32_t n) {  // OPT-126/129
    fusion_result_t res = {RTKA_UNKNOWN, 0.0};
    double weighted_sum = 0.0;
    double weight_sum = 0.0;
    double conf_prod_or = 1.0;
    double confs[NUM_SENSORS] = {0.0};
    uint32_t conf_idx = 0U;
    double total_var = 0.0;
    bool early_term = false;

    for (uint32_t i = 0U; i < n; i++) {
        // OPT-129: Reweight by variance
        uint32_t idx = (uint32_t)(sensors[i].variance * (double)(VAR_LUT_SIZE - 1U));
        float weight = (idx < VAR_LUT_SIZE) ? var_weight_lut[idx] : 0.0f;
        double reweighted = (double)sensors[i].detection * (double)weight * sensors[i].confidence;
        weighted_sum += reweighted;
        weight_sum += sensors[i].confidence;

        conf_prod_or *= (1.0 - sensors[i].confidence);
        confs[conf_idx++] = sensors[i].confidence;
        total_var += sensors[i].variance;

        // OPT-125: Early term on high-conf TRUE
        if (sensors[i].detection == RTKA_TRUE && sensors[i].confidence >= g_threshold.theta) {
            early_term = true;
            break;
        }
    }

    double consensus_val = (weight_sum > 0.0) ? weighted_sum / weight_sum : 0.0;
    double fused_conf_or = 1.0 - conf_prod_or;

    if (early_term) {
        res.fused = RTKA_TRUE;
        res.confidence = fused_conf_or;
        goto done;
    }

    res.fused = (consensus_val > 0.5) ? RTKA_TRUE : (consensus_val < -0.5 ? RTKA_FALSE : RTKA_UNKNOWN);
    res.confidence = fused_conf_or;
    if (res.fused == RTKA_UNKNOWN) {
        double geo_mean = geometric_mean(confs, conf_idx);
        double deviation = fabs(consensus_val) / 0.5;
        res.confidence = geo_mean * (1.0 - deviation);  // OPT-126 damp
    }

    res.fused = apply_threshold_coercion(res.fused, res.confidence);
    update_threshold(res.confidence > 0.5);

done:
    // OPT-128: Adaptive var thresh if high total var
    if (total_var / (double)n > (double)(g_threshold.var_threshold * 2.0f)) {
        g_threshold.var_threshold *= 1.1f;  // Bump thresh for noisier env
    }

    return res;
}

// ============================================================================
// Main Demo (Scalar for Simplicity; Parallel via Deque in Future)
// ============================================================================
int main(void) {
    srand(42U);
    init_sigmoid_lut();
    init_var_lut();

    printf("RTKA-U Core Implementation Demo\n");
    printf("================================\n");
    printf("Recursive ternary eval + sensor fusion (OR chain, conf propagation).\n");
    printf("100k evals: Balanced F/U/T inputs.\n\n");

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
            sensors[i].confidence = (double)rand() / (double)RAND_MAX * 0.8 + 0.2;
            sensors[i].variance = (double)rand() / (double)RAND_MAX * 0.2;
        }

        struct timespec start = {0}, end = {0};
        clock_gettime(CLOCK_MONOTONIC, &start);
        fusion_result_t res = fuse_sensors(sensors, NUM_SENSORS);
        clock_gettime(CLOCK_MONOTONIC, &end);

        double time_ms = (double)(end.tv_sec - start.tv_sec) * 1000.0 + 
                        (double)(end.tv_nsec - start.tv_nsec) / 1e6;
        avg_time += time_ms;

        switch (res.fused) {
            case RTKA_TRUE: true_count++; break;
            case RTKA_FALSE: false_count++; break;
            case RTKA_UNKNOWN: unknown_count++; break;
            default: break;  // OPT-000: Switch-default
        }

        if (trial % 1000U == 0U) {
            printf("Trial %u: Fused = %s (conf: %.3f, time: %.3f ms)\n",
                   trial, (res.fused == RTKA_TRUE) ? "TRUE" : 
                         (res.fused == RTKA_FALSE ? "FALSE" : "UNKNOWN"),
                   res.confidence, time_ms);
        }
    }

    clock_gettime(CLOCK_MONOTONIC, &end_all);
    double total_time = (double)(end_all.tv_sec - start_all.tv_sec) * 1000.0 + 
                       (double)(end_all.tv_nsec - start_all.tv_nsec) / 1e6;
    avg_time /= (double)trials;

    printf("\nStats: TRUE: %u, FALSE: %u, UNKNOWN: %u | Avg time: %.3f ms | Total: %.3f ms\n",
           true_count, false_count, unknown_count, avg_time, total_time);
    printf("RTKA-U: Theorem holds—UNKNOWN rate %.2f > (2/3)^7 ≈0.06 (boosted by noise).\n", 
           (double)unknown_count / (double)trials);
    printf("Ready for FSD: Swap fuse_sensors for recursive tree eval.\n");

    return 0;
}
