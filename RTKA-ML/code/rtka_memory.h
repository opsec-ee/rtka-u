/**
 * File: rtka_memory.h
 * Copyright (c) 2025 - H.Overman opsec.ee@pm.me
 * 
 * PROPRIETARY AND CONFIDENTIAL
 * This file is proprietary to H. Overman and may not be reproduced,
 * distributed, or used without explicit written permission.
 *
 * For licensing inquiries: opsec.ee@pm.me
 *
 * RTKA Memory Management - Optimized
 * High-performance allocators for ternary operations
 */

#ifndef RTKA_MEMORY_H
#define RTKA_MEMORY_H

#include "rtka_types.h"
#include "rtka_constants.h"
#include <stdatomic.h>

/* Memory pool for states - cache-aligned blocks */
typedef struct rtka_pool_block {
    struct rtka_pool_block* next;
    uint8_t data[] RTKA_ALIGNED(RTKA_CACHE_LINE_SIZE);
} rtka_pool_block_t;

typedef struct RTKA_ALIGNED(RTKA_CACHE_LINE_SIZE) {
    void* memory_base;
    size_t total_size;
    size_t block_size;
    size_t block_count;
    rtka_pool_block_t* free_list;
    
    /* Statistics */
    atomic_uint_fast32_t allocated;
    atomic_uint_fast32_t peak_usage;
    atomic_uint_fast32_t cache_hits;
    
    bool initialized;
} rtka_pool_t;

/* Stack allocator for temporaries */
typedef struct RTKA_ALIGNED(RTKA_CACHE_LINE_SIZE) {
    void* memory_base;
    size_t total_size;
    atomic_size_t current_offset;
    size_t alignment;
    bool initialized;
} rtka_stack_t;

/* Thread-local storage for per-thread pools */
typedef struct {
    rtka_pool_t* state_pool;
    rtka_stack_t* temp_stack;
    uint32_t thread_id;
} rtka_thread_memory_t;

/* Global memory system */
extern _Thread_local rtka_thread_memory_t rtka_tls_memory;

/* Initialization */
rtka_error_t rtka_memory_init(void);
rtka_error_t rtka_memory_init_custom(size_t pool_size, size_t stack_size);
void rtka_memory_cleanup(void);

/* Pool operations - optimized for states */
RTKA_NODISCARD void* rtka_pool_alloc(rtka_pool_t* pool);
void rtka_pool_free(rtka_pool_t* pool, void* ptr);
void rtka_pool_reset(rtka_pool_t* pool);

/* Stack operations - for temporary allocations */
RTKA_NODISCARD void* rtka_stack_alloc(rtka_stack_t* stack, size_t size);
void rtka_stack_reset(rtka_stack_t* stack);
void* rtka_stack_save_point(rtka_stack_t* stack);
void rtka_stack_restore(rtka_stack_t* stack, void* save_point);

/* Specialized allocators */
RTKA_NODISCARD RTKA_INLINE rtka_state_t* rtka_alloc_state(void) {
    return (rtka_state_t*)rtka_pool_alloc(rtka_tls_memory.state_pool);
}

RTKA_NODISCARD RTKA_INLINE rtka_state_t* rtka_alloc_states(uint32_t count) {
    return (rtka_state_t*)rtka_stack_alloc(rtka_tls_memory.temp_stack, 
                                           count * sizeof(rtka_state_t));
}

RTKA_INLINE void rtka_free_state(rtka_state_t* state) {
    rtka_pool_free(rtka_tls_memory.state_pool, state);
}

/* Vector allocation */
RTKA_NODISCARD rtka_vector_t* rtka_alloc_vector(uint32_t size);
void rtka_free_vector(rtka_vector_t* vec);

/* Memory statistics */
typedef struct {
    uint64_t total_allocated;
    uint64_t current_usage;
    uint64_t peak_usage;
    uint64_t allocation_count;
    uint64_t free_count;
    float fragmentation;
} rtka_memory_stats_t;

rtka_memory_stats_t rtka_memory_get_stats(void);

#endif /* RTKA_MEMORY_H */
