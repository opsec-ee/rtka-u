/**
 * File: rtka_core_bridge.c
 * Copyright (c) 2025 - H.Overman <opsec.ee@pm.me>
 * Email: opsec.ee@pm.me
 *
 * MIT License
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND.
 *
 * RTKA Core Bridge Implementation
 *
 * CHANGELOG:
 * v1.0.1 - Initial implementation
 * - Performance tracking
 * - Module interface
 * - Compatibility testing
 */

#include "rtka_memory.h"
#include "rtka_allocator.h"
#include "rtka_core_bridge.h"
#include <string.h>
#include <time.h>
#include <stdio.h>
#include <stddef.h>
#include <math.h>
#include <stdlib.h>

/* Performance tracking */
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
static void acquire_metrics_lock(void) {}
static void release_metrics_lock(void) {}
#endif

/* Module operations with tracking */
static rtka_state_t core_module_and(rtka_state_t lhs, rtka_state_t rhs) {
    clock_t start_time = clock();

    rtka_state_t result = {
        .value = rtka_and(lhs.value, rhs.value),
        .confidence = rtka_conf_and(lhs.confidence, rhs.confidence)
    };

    clock_t elapsed = clock() - start_time;

    acquire_metrics_lock();
    g_performance_metrics.operations_count++;
    g_performance_metrics.total_time_ns +=
    (uint64_t)(elapsed * 1000000000ULL / CLOCKS_PER_SEC);
    release_metrics_lock();

    return result;
}

static rtka_state_t core_module_or(rtka_state_t lhs, rtka_state_t rhs) {
    clock_t start_time = clock();

    rtka_state_t result = {
        .value = rtka_or(lhs.value, rhs.value),
        .confidence = rtka_conf_or(lhs.confidence, rhs.confidence)
    };

    clock_t elapsed = clock() - start_time;

    acquire_metrics_lock();
    g_performance_metrics.operations_count++;
    g_performance_metrics.total_time_ns +=
    (uint64_t)(elapsed * 1000000000ULL / CLOCKS_PER_SEC);
    release_metrics_lock();

    return result;
}

static rtka_state_t core_module_not(rtka_state_t operand, rtka_state_t unused) {
    (void)unused;

    rtka_state_t result = {
        .value = rtka_not(operand.value),
        .confidence = rtka_conf_not(operand.confidence)
    };

    acquire_metrics_lock();
    g_performance_metrics.operations_count++;
    release_metrics_lock();

    return result;
}

static rtka_state_t core_module_equiv(rtka_state_t lhs, rtka_state_t rhs) {
    rtka_state_t result = {
        .value = rtka_equiv(lhs.value, rhs.value),
        .confidence = rtka_conf_equiv(lhs.confidence, rhs.confidence)
    };

    acquire_metrics_lock();
    g_performance_metrics.operations_count++;
    release_metrics_lock();

    return result;
}

/* Batch operations */
static void core_batch_and(const rtka_vector_t* lhs, const rtka_vector_t* rhs, rtka_vector_t* result) {
    if (!lhs || !rhs || !result) return;

    uint32_t count = RTKA_MIN(lhs->count, rhs->count);
    if (count > RTKA_MAX_VECTOR_SIZE) count = RTKA_MAX_VECTOR_SIZE;

    result->count = count;

    for (uint32_t i = 0U; i < count; i++) {
        result->values[i] = rtka_and(lhs->values[i], rhs->values[i]);
        result->confidences[i] = rtka_conf_and(lhs->confidences[i], rhs->confidences[i]);
    }

    acquire_metrics_lock();
    g_performance_metrics.operations_count += count;
    release_metrics_lock();
}

static void core_batch_or(const rtka_vector_t* lhs, const rtka_vector_t* rhs, rtka_vector_t* result) {
    if (!lhs || !rhs || !result) return;

    uint32_t count = RTKA_MIN(lhs->count, rhs->count);
    if (count > RTKA_MAX_VECTOR_SIZE) count = RTKA_MAX_VECTOR_SIZE;

    result->count = count;

    for (uint32_t i = 0U; i < count; i++) {
        result->values[i] = rtka_or(lhs->values[i], rhs->values[i]);
        result->confidences[i] = rtka_conf_or(lhs->confidences[i], rhs->confidences[i]);
    }

    acquire_metrics_lock();
    g_performance_metrics.operations_count += count;
    release_metrics_lock();
}

/* Validation */
static bool core_validate_state(const rtka_state_t* state, void* context) {
    (void)context;
    return rtka_is_valid_state(state);
}

/* Serialization */
static size_t core_serialize_state(const void* data, void* buffer, size_t buffer_size) {
    if (!data || !buffer) return 0U;

    const rtka_state_t* state = (const rtka_state_t*)data;
    size_t required_size = sizeof(rtka_state_t);

    if (buffer_size < required_size) return required_size;

    memcpy(buffer, state, required_size);
    return required_size;
}

static size_t core_deserialize_state(const void* buffer, void* data, size_t buffer_size) {
    if (!buffer || !data) return 0U;

    size_t required_size = sizeof(rtka_state_t);
    if (buffer_size < required_size) return required_size;

    memcpy(data, buffer, required_size);
    return required_size;
}

/* Module lifecycle */
static bool core_module_init(void) {
    memset(&g_performance_metrics, 0, sizeof(g_performance_metrics));
    g_bridge_initialized = true;
    return true;
}

static void core_module_cleanup(void) {
    g_bridge_initialized = false;
    memset(&g_performance_metrics, 0, sizeof(g_performance_metrics));
}

/* Module descriptor */
const rtka_module_t rtka_core_module = {
    .module_name = "rtka-core",
    .version_major = 1U,
    .version_minor = 3U,
    .version_patch = 0U,

    .and_fn = core_module_and,
    .or_fn = core_module_or,
    .not_fn = core_module_not,
    .equiv_fn = core_module_equiv,

    .batch_and_fn = core_batch_and,
    .batch_or_fn = core_batch_or,

    .conf_and_fn = rtka_conf_and,
    .conf_or_fn = rtka_conf_or,

    .validate_fn = core_validate_state,
    .serialize_fn = core_serialize_state,
    .deserialize_fn = core_deserialize_state,

    .init_fn = core_module_init,
    .cleanup_fn = core_module_cleanup,

    .module_data = &g_performance_metrics,
    .data_size = sizeof(g_performance_metrics)
};

/* Vector operations - Fixed to use allocator system */
rtka_result_rtka_vector_t rtka_vector_and(const rtka_vector_t* lhs, const rtka_vector_t* rhs) {
    if (!lhs || !rhs) {
        return RTKA_ERROR(data, RTKA_ERROR_NULL_POINTER, "Null vector operand");
    }

    if (lhs->count != rhs->count) {
        return RTKA_ERROR(data, RTKA_ERROR_INVALID_DIMENSION, "Vector size mismatch");
    }

    rtka_allocator_t* allocator = rtka_get_default_state_allocator();
    if (!allocator) {
        return RTKA_ERROR(data, RTKA_ERROR_NOT_INITIALIZED, "Memory system not initialized");
    }

    rtka_vector_t* result = rtka_alloc_vector(allocator, RTKA_MAX_VECTOR_SIZE);
    if (!result) {
        return RTKA_ERROR(data, RTKA_ERROR_OUT_OF_MEMORY, "Cannot allocate result");
    }

    core_batch_and(lhs, rhs, result);

    return RTKA_OK(data, result);
}

rtka_result_rtka_vector_t rtka_vector_or(const rtka_vector_t* lhs, const rtka_vector_t* rhs) {
    if (!lhs || !rhs) {
        return RTKA_ERROR(data, RTKA_ERROR_NULL_POINTER, "Null vector operand");
    }

    if (lhs->count != rhs->count) {
        return RTKA_ERROR(data, RTKA_ERROR_INVALID_DIMENSION, "Vector size mismatch");
    }

    rtka_allocator_t* allocator = rtka_get_default_state_allocator();
    if (!allocator) {
        return RTKA_ERROR(data, RTKA_ERROR_NOT_INITIALIZED, "Memory system not initialized");
    }

    rtka_vector_t* result = rtka_alloc_vector(allocator, RTKA_MAX_VECTOR_SIZE);
    if (!result) {
        return RTKA_ERROR(data, RTKA_ERROR_OUT_OF_MEMORY, "Cannot allocate result");
    }

    core_batch_or(lhs, rhs, result);

    return RTKA_OK(data, result);
}

/* Recursive operations with safety */
rtka_result_rtka_state_t rtka_recursive_and_safe(const rtka_state_t* states, uint32_t count) {
    if (!states && count > 0U) {
        return RTKA_ERROR(state, RTKA_ERROR_NULL_POINTER, "Null states array");
    }

    if (count > RTKA_MAX_FACTORS) {
        return RTKA_ERROR(state, RTKA_ERROR_INVALID_DIMENSION, "Too many states");
    }

    for (uint32_t i = 0U; i < count; i++) {
        if (!rtka_is_valid_state(&states[i])) {
            return RTKA_ERROR(state, RTKA_ERROR_INVALID_VALUE, "Invalid state in array");
        }
    }

    rtka_state_t result = rtka_recursive_and_seq(states, count);

    return RTKA_OK(state, result);
}

rtka_result_rtka_state_t rtka_recursive_or_safe(const rtka_state_t* states, uint32_t count) {
    if (!states && count > 0U) {
        return RTKA_ERROR(state, RTKA_ERROR_NULL_POINTER, "Null states array");
    }

    if (count > RTKA_MAX_FACTORS) {
        return RTKA_ERROR(state, RTKA_ERROR_INVALID_DIMENSION, "Too many states");
    }

    for (uint32_t i = 0U; i < count; i++) {
        if (!rtka_is_valid_state(&states[i])) {
            return RTKA_ERROR(state, RTKA_ERROR_INVALID_VALUE, "Invalid state in array");
        }
    }

    rtka_state_t result = rtka_recursive_or_seq(states, count);

    return RTKA_OK(state, result);
}

/* Bridge interface */
rtka_error_t rtka_core_bridge_init(void) {
    if (g_bridge_initialized) {
        return RTKA_SUCCESS;
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

/* Compatibility testing */
bool rtka_bridge_compatibility_test(void) {
    const rtka_state_t test_cases[] = {
        {RTKA_FALSE, 0.8f},
        {RTKA_UNKNOWN, 0.5f},
        {RTKA_TRUE, 0.9f}
    };

    const size_t num_test_cases = RTKA_ARRAY_SIZE(test_cases);

    for (size_t i = 0U; i < num_test_cases; i++) {
        for (size_t j = 0U; j < num_test_cases; j++) {
            rtka_state_t rtka_u_result = {
                .value = rtka_and(test_cases[i].value, test_cases[j].value),
                .confidence = rtka_conf_and(test_cases[i].confidence, test_cases[j].confidence)
            };

            rtka_state_t bridge_result = core_module_and(test_cases[i], test_cases[j]);

            if (rtka_u_result.value != bridge_result.value) return false;
            if (fabsf(rtka_u_result.confidence - bridge_result.confidence) > RTKA_CONFIDENCE_EPSILON) return false;
        }
    }

    return true;
}

void rtka_bridge_performance_comparison(void) {
    const uint32_t num_iterations = 100000U;
    const rtka_state_t test_state_a = {RTKA_TRUE, 0.9f};
    const rtka_state_t test_state_b = {RTKA_UNKNOWN, 0.5f};

    clock_t start_time = clock();

    for (uint32_t i = 0U; i < num_iterations; i++) {
        (void)rtka_and(test_state_a.value, test_state_b.value);
        (void)rtka_conf_and(test_state_a.confidence, test_state_b.confidence);
    }

    clock_t u_core_time = clock() - start_time;

    start_time = clock();

    for (uint32_t i = 0U; i < num_iterations; i++) {
        (void)core_module_and(test_state_a, test_state_b);
    }

    clock_t bridge_time = clock() - start_time;

    double u_core_seconds = (double)u_core_time / CLOCKS_PER_SEC;
    double bridge_seconds = (double)bridge_time / CLOCKS_PER_SEC;
    double overhead_percent = ((bridge_seconds - u_core_seconds) / u_core_seconds) * 100.0;

    printf("Performance Comparison:\n");
    printf("Direct: %.6f seconds\n", u_core_seconds);
    printf("Bridge: %.6f seconds\n", bridge_seconds);
    printf("Overhead: %.2f%%\n", overhead_percent);
}
