/**
 * File: rtka_core_bridge.c
 * Copyright (c) 2025 - H.Overman <opsec.ee@pm.me>
 * Email: opsec.ee@pm.me
 * 
 * RTKA Core Bridge Implementation
 * Implements the bridge between rtka_u_core and rtka_types systems
 * Provides module interface and enhanced features while preserving proven algorithms
 */

#include "rtka_core_bridge.h"
#include <string.h>
#include <time.h>
#include <assert.h>

/* ============================================================================
 * GLOBAL STATE AND PERFORMANCE TRACKING
 * ============================================================================ */

static rtka_performance_t g_performance_metrics = {0};
static bool g_bridge_initialized = false;

#ifdef RTKA_C11_AVAILABLE
static _Atomic bool g_metrics_lock = false;

static void acquire_metrics_lock(void) {
    bool expected = false;
    while (!atomic_compare_exchange_weak(&g_metrics_lock, &expected, true)) {
        expected = false;
    }
}

static void release_metrics_lock(void) {
    atomic_store(&g_metrics_lock, false);
}
#else
static void acquire_metrics_lock(void) { /* No-op without C11 */ }
static void release_metrics_lock(void) { /* No-op without C11 */ }
#endif

/* ============================================================================
 * MODULE INTERFACE IMPLEMENTATION
 * ============================================================================ */

/**
 * Core module AND operation with performance tracking
 */
static rtka_state_t core_module_and(rtka_state_t lhs, rtka_state_t rhs) {
    acquire_metrics_lock();
    g_performance_metrics.operations_count++;
    release_metrics_lock();
    
    clock_t start_time = clock();
    
    rtka_state_t result = {
        .value = rtka_and(lhs.value, rhs.value),
        .confidence = rtka_conf_and(lhs.confidence, rhs.confidence)
    };
    
    clock_t end_time = clock();
    
    acquire_metrics_lock();
    g_performance_metrics.computation_time += (double)(end_time - start_time) / CLOCKS_PER_SEC;
    g_performance_metrics.average_confidence = 
        (g_performance_metrics.average_confidence * (g_performance_metrics.operations_count - 1U) + result.confidence) 
        / g_performance_metrics.operations_count;
    release_metrics_lock();
    
    return result;
}

/**
 * Core module OR operation with performance tracking
 */
static rtka_state_t core_module_or(rtka_state_t lhs, rtka_state_t rhs) {
    acquire_metrics_lock();
    g_performance_metrics.operations_count++;
    release_metrics_lock();
    
    clock_t start_time = clock();
    
    rtka_state_t result = {
        .value = rtka_or(lhs.value, rhs.value),
        .confidence = rtka_conf_or(lhs.confidence, rhs.confidence)
    };
    
    clock_t end_time = clock();
    
    acquire_metrics_lock();
    g_performance_metrics.computation_time += (double)(end_time - start_time) / CLOCKS_PER_SEC;
    g_performance_metrics.average_confidence = 
        (g_performance_metrics.average_confidence * (g_performance_metrics.operations_count - 1U) + result.confidence) 
        / g_performance_metrics.operations_count;
    release_metrics_lock();
    
    return result;
}

/**
 * Core module NOT operation with performance tracking
 */
static rtka_state_t core_module_not(rtka_state_t operand, rtka_state_t unused) {
    (void)unused; /* Suppress unused parameter warning */
    
    acquire_metrics_lock();
    g_performance_metrics.operations_count++;
    release_metrics_lock();
    
    clock_t start_time = clock();
    
    rtka_state_t result = {
        .value = rtka_not(operand.value),
        .confidence = rtka_conf_not(operand.confidence)
    };
    
    clock_t end_time = clock();
    
    acquire_metrics_lock();
    g_performance_metrics.computation_time += (double)(end_time - start_time) / CLOCKS_PER_SEC;
    g_performance_metrics.average_confidence = 
        (g_performance_metrics.average_confidence * (g_performance_metrics.operations_count - 1U) + result.confidence) 
        / g_performance_metrics.operations_count;
    release_metrics_lock();
    
    return result;
}

/**
 * Core module EQUIV operation with performance tracking
 */
static rtka_state_t core_module_equiv(rtka_state_t lhs, rtka_state_t rhs) {
    acquire_metrics_lock();
    g_performance_metrics.operations_count++;
    release_metrics_lock();
    
    clock_t start_time = clock();
    
    rtka_state_t result = {
        .value = rtka_equiv(lhs.value, rhs.value),
        .confidence = rtka_conf_equiv(lhs.confidence, rhs.confidence)
    };
    
    clock_t end_time = clock();
    
    acquire_metrics_lock();
    g_performance_metrics.computation_time += (double)(end_time - start_time) / CLOCKS_PER_SEC;
    g_performance_metrics.average_confidence = 
        (g_performance_metrics.average_confidence * (g_performance_metrics.operations_count - 1U) + result.confidence) 
        / g_performance_metrics.operations_count;
    release_metrics_lock();
    
    return result;
}

/**
 * Batch AND operation placeholder (no direct vectorization yet)
 */
static void core_batch_and(rtka_vector_t* result, const rtka_vector_t* lhs, const rtka_vector_t* rhs) {
    if (!result || !lhs || !rhs) return;
    
    uint32_t count = (lhs->count < rhs->count) ? lhs->count : rhs->count;
    if (count > RTKA_VECTOR_SIZE) count = RTKA_VECTOR_SIZE;
    
    result->count = count;
    
    for (uint32_t i = 0U; i < count; i++) {
        result->values[i] = rtka_and(lhs->values[i], rhs->values[i]);
        result->confidences[i] = rtka_conf_and(lhs->confidences[i], rhs->confidences[i]);
    }
    
    acquire_metrics_lock();
    g_performance_metrics.operations_count += count;
    release_metrics_lock();
}

/**
 * Batch OR operation placeholder (no direct vectorization yet)
 */
static void core_batch_or(rtka_vector_t* result, const rtka_vector_t* lhs, const rtka_vector_t* rhs) {
    if (!result || !lhs || !rhs) return;
    
    uint32_t count = (lhs->count < rhs->count) ? lhs->count : rhs->count;
    if (count > RTKA_VECTOR_SIZE) count = RTKA_VECTOR_SIZE;
    
    result->count = count;
    
    for (uint32_t i = 0U; i < count; i++) {
        result->values[i] = rtka_or(lhs->values[i], rhs->values[i]);
        result->confidences[i] = rtka_conf_or(lhs->confidences[i], rhs->confidences[i]);
    }
    
    acquire_metrics_lock();
    g_performance_metrics.operations_count += count;
    release_metrics_lock();
}

/**
 * Basic input validation for states
 */
static bool core_validate_state(const rtka_state_t* state, void* context) {
    (void)context; /* Unused parameter */
    return rtka_is_valid_state(state);
}

/**
 * Basic serialization (simplified implementation)
 */
static size_t core_serialize_state(const void* data, void* buffer, size_t buffer_size) {
    if (!data || !buffer) return 0U;
    
    const rtka_state_t* state = (const rtka_state_t*)data;
    size_t required_size = sizeof(rtka_state_t);
    
    if (buffer_size < required_size) return required_size; /* Return required size */
    
    memcpy(buffer, state, required_size);
    return required_size;
}

/**
 * Basic deserialization (simplified implementation)
 */
static size_t core_deserialize_state(const void* buffer, void* data, size_t buffer_size) {
    if (!buffer || !data) return 0U;
    
    size_t required_size = sizeof(rtka_state_t);
    if (buffer_size < required_size) return required_size;
    
    memcpy(data, buffer, required_size);
    return required_size;
}

/**
 * Module initialization
 */
static bool core_module_init(void) {
    memset(&g_performance_metrics, 0, sizeof(g_performance_metrics));
    g_bridge_initialized = true;
    return true;
}

/**
 * Module cleanup
 */
static void core_module_cleanup(void) {
    g_bridge_initialized = false;
    memset(&g_performance_metrics, 0, sizeof(g_performance_metrics));
}

/* ============================================================================
 * MODULE DESCRIPTOR
 * ============================================================================ */

const rtka_module_t rtka_core_module = {
    .module_name = "rtka-core",
    .version_major = 1U,
    .version_minor = 3U,
    .version_patch = 0U,
    
    /* Core operations */
    .and_fn = core_module_and,
    .or_fn = core_module_or,
    .not_fn = core_module_not,
    .equiv_fn = core_module_equiv,
    
    /* Batch operations */
    .batch_and_fn = core_batch_and,
    .batch_or_fn = core_batch_or,
    
    /* Confidence propagation (use existing functions) */
    .conf_and_fn = rtka_conf_and,
    .conf_or_fn = rtka_conf_or,
    
    /* Module-specific functions */
    .validate_fn = core_validate_state,
    .serialize_fn = core_serialize_state,
    .deserialize_fn = core_deserialize_state,
    
    /* Lifecycle functions */
    .init_fn = core_module_init,
    .cleanup_fn = core_module_cleanup,
    
    /* Module data */
    .module_data = &g_performance_metrics,
    .data_size = sizeof(g_performance_metrics)
};

/* ============================================================================
 * BRIDGE INTERFACE IMPLEMENTATION
 * ============================================================================ */

rtka_error_t rtka_core_bridge_init(void) {
    if (g_bridge_initialized) {
        return RTKA_SUCCESS; /* Already initialized */
    }
    
    return rtka_core_module.init_fn() ? RTKA_SUCCESS : RTKA_ERROR_MODULE_INIT_FAILED;
}

void rtka_core_bridge_cleanup(void) {
    if (g_bridge_initialized) {
        rtka_core_module.cleanup_fn();
    }
}

rtka_performance_t rtka_core_get_performance_metrics(void) {
    acquire_metrics_lock();
    rtka_performance_t metrics_copy = g_performance_metrics;
    release_metrics_lock();
    
    return metrics_copy;
}

void rtka_core_reset_performance_metrics(void) {
    acquire_metrics_lock();
    memset(&g_performance_metrics, 0, sizeof(g_performance_metrics));
    release_metrics_lock();
}

/* ============================================================================
 * COMPATIBILITY AND TESTING FUNCTIONS
 * ============================================================================ */

bool rtka_bridge_compatibility_test(void) {
    /* Test all core operations for consistency */
    
    const rtka_state_t test_cases[] = {
        {RTKA_FALSE, 0.8f},
        {RTKA_UNKNOWN, 0.5f},
        {RTKA_TRUE, 0.9f}
    };
    
    const size_t num_test_cases = sizeof(test_cases) / sizeof(test_cases[0]);
    
    /* Test binary operations */
    for (size_t i = 0U; i < num_test_cases; i++) {
        for (size_t j = 0U; j < num_test_cases; j++) {
            /* Test AND operation consistency */
            rtka_state_t rtka_u_result = {
                .value = rtka_and(test_cases[i].value, test_cases[j].value),
                .confidence = rtka_conf_and(test_cases[i].confidence, test_cases[j].confidence)
            };
            
            rtka_state_t bridge_result = core_module_and(test_cases[i], test_cases[j]);
            
            if (rtka_u_result.value != bridge_result.value) return false;
            if (fabsf(rtka_u_result.confidence - bridge_result.confidence) > RTKA_CONFIDENCE_EPSILON) return false;
            
            /* Test OR operation consistency */
            rtka_u_result.value = rtka_or(test_cases[i].value, test_cases[j].value);
            rtka_u_result.confidence = rtka_conf_or(test_cases[i].confidence, test_cases[j].confidence);
            
            bridge_result = core_module_or(test_cases[i], test_cases[j]);
            
            if (rtka_u_result.value != bridge_result.value) return false;
            if (fabsf(rtka_u_result.confidence - bridge_result.confidence) > RTKA_CONFIDENCE_EPSILON) return false;
        }
    }
    
    /* Test unary operations */
    for (size_t i = 0U; i < num_test_cases; i++) {
        rtka_state_t rtka_u_result = {
            .value = rtka_not(test_cases[i].value),
            .confidence = rtka_conf_not(test_cases[i].confidence)
        };
        
        rtka_state_t bridge_result = core_module_not(test_cases[i], test_cases[i]);
        
        if (rtka_u_result.value != bridge_result.value) return false;
        if (fabsf(rtka_u_result.confidence - bridge_result.confidence) > RTKA_CONFIDENCE_EPSILON) return false;
    }
    
    /* Test recursive operations */
    rtka_state_t recursive_states[] = {
        {RTKA_TRUE, 0.9f},
        {RTKA_UNKNOWN, 0.5f},
        {RTKA_FALSE, 0.8f}
    };
    
    rtka_state_t rtka_u_recursive = rtka_recursive_and_seq(recursive_states, 3U);
    rtka_result_rtka_state_t bridge_recursive = rtka_recursive_and_safe(recursive_states, 3U);
    
    if (!RTKA_IS_OK(bridge_recursive)) return false;
    
    if (rtka_u_recursive.value != bridge_recursive.value.value) return false;
    if (fabsf(rtka_u_recursive.confidence - bridge_recursive.value.confidence) > RTKA_CONFIDENCE_EPSILON) return false;
    
    return true;
}

void rtka_bridge_performance_comparison(void) {
    const uint32_t num_iterations = 100000U;
    const rtka_state_t test_state_a = {RTKA_TRUE, 0.9f};
    const rtka_state_t test_state_b = {RTKA_UNKNOWN, 0.5f};
    
    /* Benchmark rtka_u_core direct calls */
    clock_t start_time = clock();
    
    for (uint32_t i = 0U; i < num_iterations; i++) {
        (void)rtka_and(test_state_a.value, test_state_b.value);
        (void)rtka_conf_and(test_state_a.confidence, test_state_b.confidence);
    }
    
    clock_t u_core_time = clock() - start_time;
    
    /* Benchmark bridge calls */
    start_time = clock();
    
    for (uint32_t i = 0U; i < num_iterations; i++) {
        (void)core_module_and(test_state_a, test_state_b);
    }
    
    clock_t bridge_time = clock() - start_time;
    
    /* Report results */
    double u_core_seconds = (double)u_core_time / CLOCKS_PER_SEC;
    double bridge_seconds = (double)bridge_time / CLOCKS_PER_SEC;
    double overhead_percent = ((bridge_seconds - u_core_seconds) / u_core_seconds) * 100.0;
    
    printf("Performance Comparison Results:\n");
    printf("rtka_u_core direct: %.6f seconds\n", u_core_seconds);
    printf("Bridge interface:   %.6f seconds\n", bridge_seconds);
    printf("Overhead:           %.2f%%\n", overhead_percent);
    printf("Operations/second:  %.0f (direct), %.0f (bridge)\n", 
           num_iterations / u_core_seconds, num_iterations / bridge_seconds);
}
