/**
 * File: rtka_types.h
 * Copyright (c) 2025 - H.Overman opsec.ee@pm.me
 * 
 * PROPRIETARY AND CONFIDENTIAL
 * This file is proprietary to H. Overman and may not be reproduced,
 * distributed, or used without explicit written permission.
 *
 * For licensing inquiries: opsec.ee@pm.me
 *
 * RTKA Core Type Definitions
 * Foundation for entire system - zero dependencies
 */

#ifndef RTKA_TYPES_H
#define RTKA_TYPES_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

/* Compiler optimizations */
#ifdef __GNUC__
    #define RTKA_INLINE __attribute__((always_inline)) inline
    #define RTKA_PURE __attribute__((pure))
    #define RTKA_CONST __attribute__((const))
    #define RTKA_NODISCARD __attribute__((warn_unused_result))
    #define RTKA_ALIGNED(x) __attribute__((aligned(x)))
    #define RTKA_RESTRICT __restrict__
    #define RTKA_LIKELY(x) __builtin_expect(!!(x), 1)
    #define RTKA_UNLIKELY(x) __builtin_expect(!!(x), 0)
#else
    #define RTKA_INLINE inline
    #define RTKA_PURE
    #define RTKA_CONST
    #define RTKA_NODISCARD
    #define RTKA_ALIGNED(x)
    #define RTKA_RESTRICT
    #define RTKA_LIKELY(x) (x)
    #define RTKA_UNLIKELY(x) (x)
#endif

/* Core ternary values - optimal encoding */
typedef enum {
    RTKA_FALSE = -1,
    RTKA_UNKNOWN = 0,  
    RTKA_TRUE = 1
} rtka_value_t;

/* Confidence type - float for cache efficiency */
typedef float rtka_confidence_t;
typedef double rtka_confidence_hp_t;

/* Core state - 8 bytes aligned */
typedef struct RTKA_ALIGNED(8) {
    rtka_value_t value;
    rtka_confidence_t confidence;
} rtka_state_t;

/* Extended state with metadata - cache line aligned */
typedef struct RTKA_ALIGNED(64) {
    rtka_state_t core;
    uint64_t timestamp;
    uint32_t source_id;
    uint32_t depth;
    uint32_t flags;
    void* metadata;
    uint8_t padding[24];  /* Pad to cache line */
} rtka_state_ext_t;

/* Vector for SIMD operations */
typedef struct RTKA_ALIGNED(64) {
    rtka_value_t* RTKA_RESTRICT values;
    rtka_confidence_t* RTKA_RESTRICT confidences;
    uint32_t count;
    uint32_t capacity;
} rtka_vector_t;

/* Error codes */
typedef enum {
    RTKA_SUCCESS = 0,
    RTKA_ERROR_NULL_POINTER = 1,
    RTKA_ERROR_INVALID_VALUE = 2,
    RTKA_ERROR_OUT_OF_MEMORY = 3,
    RTKA_ERROR_NOT_INITIALIZED = 4,
    RTKA_ERROR_OVERFLOW = 5,
    RTKA_ERROR_MAX
} rtka_error_t;

/* Allocator interface for zero-cost abstraction */
typedef struct {
    void* (*alloc)(size_t size, void* context);
    void (*free)(void* ptr, void* context);
    void* context;
} rtka_allocator_t;

/* Module interface for extensibility */
typedef struct {
    const char* module_name;
    uint32_t version;
    
    /* Core operations as function pointers */
    rtka_state_t (*and_fn)(rtka_state_t, rtka_state_t);
    rtka_state_t (*or_fn)(rtka_state_t, rtka_state_t);
    rtka_state_t (*not_fn)(rtka_state_t);
    
    /* Module data */
    void* module_data;
} rtka_module_t;

/* Inline helpers for optimal code generation */
RTKA_INLINE RTKA_CONST bool rtka_is_valid_value(rtka_value_t v) {
    return v >= RTKA_FALSE && v <= RTKA_TRUE;
}

RTKA_INLINE RTKA_CONST bool rtka_is_valid_confidence(rtka_confidence_t c) {
    return c >= 0.0f && c <= 1.0f;
}

RTKA_INLINE RTKA_PURE bool rtka_is_valid_state(const rtka_state_t* s) {
    return RTKA_LIKELY(s != NULL) && 
           rtka_is_valid_value(s->value) && 
           rtka_is_valid_confidence(s->confidence);
}

/* State creation - compiler can inline and optimize */
RTKA_INLINE RTKA_CONST rtka_state_t rtka_make_state(rtka_value_t v, rtka_confidence_t c) {
    return (rtka_state_t){.value = v, .confidence = c};
}

#endif /* RTKA_TYPES_H */
