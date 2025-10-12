/**
 * File: rtka_allocator.h
 * Copyright (c) 2025 H. Overman <opsec.ee@pm.me>
 *
 * PROPRIETARY AND CONFIDENTIAL
 * This file is proprietary to H. Overman and may not be reproduced,
 * distributed, or used without explicit written permission.
 *
 * For licensing inquiries: opsec.ee@pm.me
 *
 * RTKA Allocator Interface
 * Type-safe allocation primitives for RTKA operations
 *
 * CHANGELOG:
 * v1.0.1 - Initial allocator interface
 * - Type-safe wrappers
 * - Alignment guarantees
 */

#ifndef RTKA_ALLOCATOR_H
#define RTKA_ALLOCATOR_H

#include "rtka_types.h"

/* Forward declaration */
typedef struct rtka_allocator rtka_allocator_t;

/* Allocation functions */
typedef void* (*rtka_alloc_fn_t)(rtka_allocator_t*, size_t);
typedef void (*rtka_free_fn_t)(rtka_allocator_t*, void*);
typedef void (*rtka_reset_fn_t)(rtka_allocator_t*);

/* Allocator interface */
struct rtka_allocator {
    rtka_allocator_type_t type;
    const char* name;
    rtka_alloc_fn_t alloc_fn;
    rtka_free_fn_t free_fn;
    rtka_reset_fn_t reset_fn;
    void* impl;
    rtka_memory_stats_t stats;
    bool initialized;
};

/* Basic allocation */
RTKA_NODISCARD RTKA_INLINE
void* rtka_alloc(rtka_allocator_t* allocator, size_t size) {
    if (RTKA_UNLIKELY(!allocator || !allocator->alloc_fn)) {
        return RTKA_NULLPTR;
    }
    return allocator->alloc_fn(allocator, size);
}

RTKA_INLINE
void rtka_free(rtka_allocator_t* allocator, void* ptr) {
    if (RTKA_LIKELY(allocator && allocator->free_fn && ptr)) {
        allocator->free_fn(allocator, ptr);
    }
}

RTKA_INLINE
void rtka_reset(rtka_allocator_t* allocator) {
    if (RTKA_LIKELY(allocator && allocator->reset_fn)) {
        allocator->reset_fn(allocator);
    }
}

/* Typed allocation */
RTKA_NODISCARD RTKA_INLINE
rtka_state_t* rtka_alloc_state(rtka_allocator_t* allocator, rtka_value_t value, rtka_confidence_t confidence) {
    rtka_state_t* state = (rtka_state_t*)rtka_alloc(allocator, sizeof(rtka_state_t));
    if (RTKA_LIKELY(state)) {
        state->value = value;
        state->confidence = confidence;
    }
    return state;
}

RTKA_NODISCARD RTKA_INLINE
rtka_state_t* rtka_alloc_state_array(rtka_allocator_t* allocator, uint32_t count) {
    if (RTKA_UNLIKELY(count == 0U || count > RTKA_MAX_FACTORS)) {
        return RTKA_NULLPTR;
    }

    size_t total_size = sizeof(rtka_state_t) * count;
    return (rtka_state_t*)rtka_alloc(allocator, total_size);
}

RTKA_NODISCARD RTKA_INLINE
rtka_vector_t* rtka_alloc_vector(rtka_allocator_t* allocator, uint32_t capacity) {
    if (RTKA_UNLIKELY(capacity == 0U || capacity > RTKA_MAX_VECTOR_SIZE)) {
        return RTKA_NULLPTR;
    }

    rtka_vector_t* vector = (rtka_vector_t*)rtka_alloc(allocator, sizeof(rtka_vector_t));
    if (RTKA_LIKELY(vector)) {
        vector->count = 0U;
        for (uint32_t i = 0U; i < 3U; i++) {
            vector->reserved[i] = 0U;
        }
    }
    return vector;
}

/* Macros for convenience */
#define RTKA_ALLOC_TYPE(allocator, type) \
((type*)rtka_alloc((allocator), sizeof(type)))

#define RTKA_ALLOC_ARRAY(allocator, type, count) \
((type*)rtka_alloc((allocator), sizeof(type) * (count)))

#define RTKA_FREE_SAFE(allocator, ptr) do { \
if ((ptr)) { \
    rtka_free((allocator), (ptr)); \
    (ptr) = RTKA_NULLPTR; \
} \
} while(0)

/* Global allocator management */
RTKA_NODISCARD
rtka_error_t rtka_allocators_init(void);

void rtka_allocators_cleanup(void);

RTKA_NODISCARD
rtka_allocator_t* rtka_get_default_allocator(void);

rtka_error_t rtka_set_threading_mode(bool enable_threading);

#endif /* RTKA_ALLOCATOR_H */
