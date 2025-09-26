/**
 * File: rtka_types.h
 * Copyright (c) 2025 H. Overman <opsec.ee@pm.me>
 *
 * PROPRIETARY AND CONFIDENTIAL
 * This file is proprietary to H. Overman and may not be reproduced,
 * distributed, or used without explicit written permission.
 *
 * For licensing inquiries: opsec.ee@pm.me
 *
 * RTKA Type System and Infrastructure
 * Comprehensive type definitions for all RTKA modules
 *
 * CHANGELOG:
 * v1.0.1 - Initial comprehensive type system
 * - Error handling via Result<T> pattern
 * - Module interface definitions
 * - Extended state types for specialized modules
 * - Performance measurement types
 */

#ifndef RTKA_TYPES_H
#define RTKA_TYPES_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>
#include <math.h>
#include "rtka_constants.h"

/* C11 feature detection */
#if defined(__STDC_VERSION__) && __STDC_VERSION__ >= 201112L
#define RTKA_C11_AVAILABLE 1
#include <stdatomic.h>
#include <threads.h>
#endif

/* C23 feature detection */
#if defined(__STDC_VERSION__) && __STDC_VERSION__ >= 202311L
#define RTKA_C23_AVAILABLE 1
#define RTKA_NULLPTR nullptr
#else
#define RTKA_NULLPTR NULL
#endif

/* Compiler attributes */
#ifdef __GNUC__
#define RTKA_ALIGNED(n) __attribute__((aligned(n)))
#define RTKA_PACKED __attribute__((packed))
#define RTKA_NODISCARD __attribute__((warn_unused_result))
#define RTKA_HOT __attribute__((hot))
#define RTKA_COLD __attribute__((cold))
#define RTKA_PURE __attribute__((pure))
#define RTKA_INLINE __attribute__((always_inline)) inline
#define RTKA_LIKELY(x) __builtin_expect(!!(x), 1)
#define RTKA_UNLIKELY(x) __builtin_expect(!!(x), 0)
#else
#define RTKA_ALIGNED(n)
#define RTKA_PACKED
#define RTKA_NODISCARD
#define RTKA_HOT
#define RTKA_COLD
#define RTKA_PURE
#define RTKA_INLINE inline
#define RTKA_LIKELY(x) (x)
#define RTKA_UNLIKELY(x) (x)
#endif

/* Core types from rtka_u_core */
typedef enum {
    RTKA_FALSE = -1,
    RTKA_UNKNOWN = 0,
    RTKA_TRUE = 1
} rtka_value_t;

typedef float rtka_confidence_t;
typedef double rtka_confidence_hp_t;

typedef struct {
    rtka_value_t value;
    rtka_confidence_t confidence;
} rtka_state_t;

/* Error codes */
typedef enum {
    RTKA_SUCCESS = 0,
    RTKA_ERROR_NULL_POINTER,
    RTKA_ERROR_INVALID_VALUE,
    RTKA_ERROR_INVALID_DIMENSION,
    RTKA_ERROR_OUT_OF_MEMORY,
    RTKA_ERROR_NOT_INITIALIZED,
    RTKA_ERROR_ALREADY_INITIALIZED,
    RTKA_ERROR_MODULE_INIT_FAILED,
    RTKA_ERROR_NOT_SUPPORTED,
    RTKA_ERROR_OVERFLOW,
    RTKA_ERROR_UNDERFLOW,
    RTKA_ERROR_TIMEOUT,
    RTKA_ERROR_PERMISSION_DENIED,
    RTKA_ERROR_IO_FAILURE
} rtka_error_t;

/* Result type system */
typedef struct {
    rtka_error_t error;
    union {
        rtka_state_t state;
        void* data;
    } value;
    const char* message;
} rtka_result_t;

typedef rtka_result_t rtka_result_rtka_state_t;

#define RTKA_IS_OK(result) ((result).error == RTKA_SUCCESS)
#define RTKA_OK(type, val) ((rtka_result_t){.error = RTKA_SUCCESS, .value.type = val, .message = NULL})
#define RTKA_ERROR(type, err, msg) ((rtka_result_t){.error = err, .message = msg})

/* High-precision state */
typedef struct {
    rtka_value_t value;
    rtka_confidence_hp_t confidence;
    uint32_t flags;
    uint32_t reserved;
} rtka_state_hp_t;

/* Extended state for modules */
typedef struct {
    rtka_state_t core;
    uint64_t timestamp;
    uint32_t source_id;
    uint32_t depth;
    uint32_t flags;
    void* metadata;
} rtka_state_ext_t;

/* State flags */
#define RTKA_FLAG_VALIDATED     (1U << 0U)
#define RTKA_FLAG_CACHED        (1U << 1U)
#define RTKA_FLAG_DIRTY         (1U << 2U)
#define RTKA_FLAG_PERSISTENT    (1U << 3U)

/* Vector type for SIMD */
typedef struct RTKA_ALIGNED(64) {
    rtka_value_t values[RTKA_MAX_VECTOR_SIZE];
    rtka_confidence_t confidences[RTKA_MAX_VECTOR_SIZE];
    uint32_t count;
    uint32_t reserved[3];
} rtka_vector_t;

typedef rtka_result_t rtka_result_rtka_vector_t;

/* Memory pool */
typedef struct {
    void* memory_base;
    size_t total_size;
    size_t allocated_size;
    size_t alignment;
    bool is_locked;
} rtka_memory_pool_t;

/* Performance metrics */
typedef struct {
    uint64_t operations_count;
    uint64_t cache_hits;
    uint64_t cache_misses;
    uint64_t total_time_ns;
    uint32_t recursion_depth_max;
    uint32_t early_terminations;
} rtka_performance_t;

/* Allocator types */
typedef enum {
    RTKA_ALLOC_SYSTEM,
    RTKA_ALLOC_POOL,
    RTKA_ALLOC_STACK,
    RTKA_ALLOC_RING
} rtka_allocator_type_t;

/* Memory statistics */
typedef struct {
    size_t total_allocated;
    size_t current_usage;
    size_t peak_usage;
    uint32_t allocation_count;
    uint32_t free_count;
} rtka_memory_stats_t;

/* Module interface function types */
typedef rtka_state_t (*rtka_operation_fn_t)(rtka_state_t, rtka_state_t);
typedef void (*rtka_batch_operation_fn_t)(const rtka_vector_t*, const rtka_vector_t*, rtka_vector_t*);
typedef rtka_confidence_t (*rtka_confidence_propagation_fn_t)(rtka_confidence_t, rtka_confidence_t);
typedef bool (*rtka_validation_fn_t)(const rtka_state_t*, void*);
typedef size_t (*rtka_serialize_fn_t)(const void*, void*, size_t);

/* Module descriptor */
typedef struct {
    const char* module_name;
    uint32_t version_major;
    uint32_t version_minor;
    uint32_t version_patch;

    rtka_operation_fn_t and_fn;
    rtka_operation_fn_t or_fn;
    rtka_operation_fn_t not_fn;
    rtka_operation_fn_t equiv_fn;

    rtka_batch_operation_fn_t batch_and_fn;
    rtka_batch_operation_fn_t batch_or_fn;

    rtka_confidence_propagation_fn_t conf_and_fn;
    rtka_confidence_propagation_fn_t conf_or_fn;

    rtka_validation_fn_t validate_fn;
    rtka_serialize_fn_t serialize_fn;
    rtka_serialize_fn_t deserialize_fn;

    bool (*init_fn)(void);
    void (*cleanup_fn)(void);

    void* module_data;
    size_t data_size;
} rtka_module_t;

/* Utility macros */
#define RTKA_MIN(a, b) ((a) < (b) ? (a) : (b))
#define RTKA_MAX(a, b) ((a) > (b) ? (a) : (b))
#define RTKA_ARRAY_SIZE(arr) (sizeof(arr) / sizeof((arr)[0U]))
#define RTKA_ALIGN_UP(n, alignment) (((n) + (alignment) - 1U) & ~((alignment) - 1U))

/* Validation functions */
RTKA_NODISCARD RTKA_PURE RTKA_INLINE
bool rtka_is_valid_value(rtka_value_t value) {
    return value >= RTKA_FALSE && value <= RTKA_TRUE;
}

RTKA_NODISCARD RTKA_PURE RTKA_INLINE
bool rtka_is_valid_confidence(rtka_confidence_t confidence) {
    return confidence >= 0.0f && confidence <= 1.0f &&
    !isnan(confidence) && !isinf(confidence);
}

RTKA_NODISCARD RTKA_PURE RTKA_INLINE
bool rtka_is_valid_state(const rtka_state_t* state) {
    return state != RTKA_NULLPTR &&
    rtka_is_valid_value(state->value) &&
    rtka_is_valid_confidence(state->confidence);
}

RTKA_INLINE rtka_state_t rtka_make_state(rtka_value_t value, rtka_confidence_t confidence) {
    rtka_state_t state = {value, confidence};
    return state;
}

#endif /* RTKA_TYPES_H */
