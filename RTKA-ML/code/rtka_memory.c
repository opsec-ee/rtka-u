/**
 * File: rtka_memory.c
 * Copyright (c) 2025 - H.Overman opsec.ee@pm.me
 * 
 * PROPRIETARY AND CONFIDENTIAL
 * This file is proprietary to H. Overman and may not be reproduced,
 * distributed, or used without explicit written permission.
 *
 * For licensing inquiries: opsec.ee@pm.me
 *
 * RTKA Memory Management Implementation
 */

#include "rtka_memory.h"
#include <stdlib.h>
#include <string.h>

/* Helper macro for alignment */
#define RTKA_ALIGN(size, align) (((size) + (align) - 1) & ~((align) - 1))

/* Thread-local memory for zero contention */
_Thread_local rtka_thread_memory_t rtka_tls_memory = {0};

/* Global pools for fallback */
static rtka_pool_t g_state_pool = {0};
static rtka_stack_t g_temp_stack = {0};

/* Initialize pool allocator */
static rtka_error_t pool_init(rtka_pool_t* pool, size_t block_size, size_t block_count) {
    pool->block_size = RTKA_ALIGN(block_size, RTKA_CACHE_LINE_SIZE);
    pool->block_count = block_count;
    pool->total_size = pool->block_size * block_count;
    
    pool->memory_base = aligned_alloc(RTKA_CACHE_LINE_SIZE, pool->total_size);
    if (!pool->memory_base) return RTKA_ERROR_OUT_OF_MEMORY;
    
    /* Build free list */
    uint8_t* base = (uint8_t*)pool->memory_base;
    pool->free_list = NULL;
    
    for (size_t i = 0; i < block_count; i++) {
        rtka_pool_block_t* block = (rtka_pool_block_t*)(base + i * pool->block_size);
        block->next = pool->free_list;
        pool->free_list = block;
    }
    
    atomic_store(&pool->allocated, 0);
    atomic_store(&pool->peak_usage, 0);
    pool->initialized = true;
    
    return RTKA_SUCCESS;
}

/* Initialize stack allocator */
static rtka_error_t stack_init(rtka_stack_t* stack, size_t size) {
    stack->total_size = RTKA_ALIGN(size, RTKA_CACHE_LINE_SIZE);
    stack->memory_base = aligned_alloc(RTKA_CACHE_LINE_SIZE, stack->total_size);
    if (!stack->memory_base) return RTKA_ERROR_OUT_OF_MEMORY;
    
    atomic_store(&stack->current_offset, 0);
    stack->alignment = RTKA_SIMD_ALIGNMENT;
    stack->initialized = true;
    
    return RTKA_SUCCESS;
}

/* Global initialization */
rtka_error_t rtka_memory_init(void) {
    return rtka_memory_init_custom(RTKA_STATE_POOL_SIZE, RTKA_TEMP_STACK_SIZE);
}

rtka_error_t rtka_memory_init_custom(size_t pool_size, size_t stack_size) {
    rtka_error_t err;
    
    /* Initialize global pools */
    err = pool_init(&g_state_pool, sizeof(rtka_state_t), pool_size);
    if (err != RTKA_SUCCESS) return err;
    
    err = stack_init(&g_temp_stack, stack_size);
    if (err != RTKA_SUCCESS) {
        free(g_state_pool.memory_base);
        return err;
    }
    
    /* Setup thread-local pointers */
    rtka_tls_memory.state_pool = &g_state_pool;
    rtka_tls_memory.temp_stack = &g_temp_stack;
    
    return RTKA_SUCCESS;
}

/* Pool allocation - lock-free fast path */
void* rtka_pool_alloc(rtka_pool_t* pool) {
    if (RTKA_UNLIKELY(!pool->initialized)) return NULL;
    
    rtka_pool_block_t* block = pool->free_list;
    if (RTKA_LIKELY(block != NULL)) {
        pool->free_list = block->next;
        
        uint32_t allocated = atomic_fetch_add(&pool->allocated, 1) + 1;
        uint32_t peak = atomic_load(&pool->peak_usage);
        if (allocated > peak) {
            atomic_store(&pool->peak_usage, allocated);
        }
        
        return block->data;
    }
    
    return NULL;  /* Pool exhausted */
}

void rtka_pool_free(rtka_pool_t* pool, void* ptr) {
    if (RTKA_UNLIKELY(!ptr || !pool->initialized)) return;
    
    rtka_pool_block_t* block = (rtka_pool_block_t*)((uint8_t*)ptr - offsetof(rtka_pool_block_t, data));
    block->next = pool->free_list;
    pool->free_list = block;
    
    atomic_fetch_sub(&pool->allocated, 1);
}

/* Stack allocation - for temporaries */
void* rtka_stack_alloc(rtka_stack_t* stack, size_t size) {
    if (RTKA_UNLIKELY(!stack->initialized)) return NULL;
    
    size = RTKA_ALIGN(size, stack->alignment);
    size_t offset = atomic_fetch_add(&stack->current_offset, size);
    
    if (RTKA_UNLIKELY(offset + size > stack->total_size)) {
        atomic_fetch_sub(&stack->current_offset, size);
        return NULL;  /* Stack overflow */
    }
    
    return (uint8_t*)stack->memory_base + offset;
}

void rtka_stack_reset(rtka_stack_t* stack) {
    atomic_store(&stack->current_offset, 0);
}

/* Vector allocation */
rtka_vector_t* rtka_alloc_vector(uint32_t size) {
    rtka_vector_t* vec = (rtka_vector_t*)rtka_pool_alloc(rtka_tls_memory.state_pool);
    if (!vec) return NULL;
    
    vec->values = (rtka_value_t*)rtka_stack_alloc(rtka_tls_memory.temp_stack, 
                                                   size * sizeof(rtka_value_t));
    vec->confidences = (rtka_confidence_t*)rtka_stack_alloc(rtka_tls_memory.temp_stack,
                                                             size * sizeof(rtka_confidence_t));
    
    if (!vec->values || !vec->confidences) {
        rtka_pool_free(rtka_tls_memory.state_pool, vec);
        return NULL;
    }
    
    vec->count = 0;
    vec->capacity = size;
    return vec;
}

void rtka_free_vector(rtka_vector_t* vec) {
    /* Stack memory is bulk-freed */
    rtka_pool_free(rtka_tls_memory.state_pool, vec);
}

/* Memory statistics */
rtka_memory_stats_t rtka_memory_get_stats(void) {
    rtka_memory_stats_t stats = {0};
    
    rtka_pool_t* pool = rtka_tls_memory.state_pool;
    if (pool) {
        stats.current_usage = atomic_load(&pool->allocated) * pool->block_size;
        stats.peak_usage = atomic_load(&pool->peak_usage) * pool->block_size;
        stats.total_allocated = pool->total_size;
        stats.allocation_count = atomic_load(&pool->allocated);
    }
    
    return stats;
}

/* Cleanup */
void rtka_memory_cleanup(void) {
    if (g_state_pool.memory_base) {
        free(g_state_pool.memory_base);
        memset(&g_state_pool, 0, sizeof(g_state_pool));
    }
    
    if (g_temp_stack.memory_base) {
        free(g_temp_stack.memory_base);
        memset(&g_temp_stack, 0, sizeof(g_temp_stack));
    }
}
