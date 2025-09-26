/**
 * File: rtka_memory.c
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
 * RTKA Memory Management Implementation
 *
 * CHANGELOG:
 * v1.0.1 - Initial implementation
 * - Runtime configurable sizes
 * - Opt-in threading model
 * - Error propagation via results
 */

#include "rtka_memory.h"
#include <stdlib.h>
#include <string.h>
#include <stddef.h>

/* Global allocators */
static rtka_allocator_t* g_default_state_allocator = NULL;
static rtka_allocator_t* g_default_temp_allocator = NULL;
static bool g_memory_initialized = false;

/* Pool allocator implementation */
rtka_allocator_t* rtka_create_pool_allocator(size_t block_size, size_t block_count, bool thread_safe) {
    if (block_size == 0U || block_count == 0U) return NULL;

    rtka_allocator_t* allocator = calloc(1U, sizeof(rtka_allocator_t));
    if (!allocator) return NULL;

    allocator->type = RTKA_ALLOC_POOL;
    allocator->name = "RTKA Pool Allocator";

    rtka_pool_allocator_t* pool = &allocator->impl.pool;

    size_t actual_block_size = RTKA_ALIGN_UP(block_size + sizeof(rtka_pool_block_t), 16U);
    size_t total_size = actual_block_size * block_count;

    pool->memory_base = aligned_alloc(RTKA_CACHE_LINE_SIZE, total_size);
    if (!pool->memory_base) {
        free(allocator);
        return NULL;
    }

    pool->total_size = total_size;
    pool->block_size = block_size;
    pool->block_count = block_count;
    pool->thread_safe = thread_safe;

    /* Initialize counters */
    pool->allocated_count = 0U;
    pool->peak_usage = 0U;

    /* Build free list */
    uint8_t* current = (uint8_t*)pool->memory_base;
    for (size_t i = 0U; i < block_count - 1U; i++) {
        rtka_pool_block_t* block = (rtka_pool_block_t*)current;
        block->next = (rtka_pool_block_t*)(current + actual_block_size);
        current += actual_block_size;
    }
    ((rtka_pool_block_t*)current)->next = NULL;
    pool->free_list = (rtka_pool_block_t*)pool->memory_base;

#ifdef RTKA_C11_AVAILABLE
    if (thread_safe) {
        mtx_init(&pool->pool_mutex, mtx_plain);
        atomic_init(&pool->allocated_count, 0U);
        atomic_init(&pool->peak_usage, 0U);
    }
#endif

    allocator->initialized = true;
    return allocator;
}

/* Stack allocator implementation */
rtka_allocator_t* rtka_create_stack_allocator(size_t stack_size, size_t alignment) {
    if (stack_size == 0U || alignment == 0U) return NULL;

    rtka_allocator_t* allocator = calloc(1U, sizeof(rtka_allocator_t));
    if (!allocator) return NULL;

    allocator->type = RTKA_ALLOC_STACK;
    allocator->name = "RTKA Stack Allocator";

    rtka_stack_allocator_t* stack = &allocator->impl.stack;

    stack->memory_base = aligned_alloc(alignment, stack_size);
    if (!stack->memory_base) {
        free(allocator);
        return NULL;
    }

    stack->total_size = stack_size;
    stack->current_offset = 0U;
    stack->alignment = alignment;

    allocator->initialized = true;
    return allocator;
}

/* Ring allocator implementation */
rtka_allocator_t* rtka_create_ring_allocator(size_t element_size, size_t element_count) {
    if (element_size == 0U || element_count == 0U) return NULL;

    rtka_allocator_t* allocator = calloc(1U, sizeof(rtka_allocator_t));
    if (!allocator) return NULL;

    allocator->type = RTKA_ALLOC_RING;
    allocator->name = "RTKA Ring Allocator";

    rtka_ring_allocator_t* ring = &allocator->impl.ring;

    size_t total_size = element_size * element_count;
    ring->memory_base = aligned_alloc(RTKA_CACHE_LINE_SIZE, total_size);
    if (!ring->memory_base) {
        free(allocator);
        return NULL;
    }

    ring->total_size = total_size;
    ring->element_size = element_size;
    ring->write_offset = 0U;
    ring->read_offset = 0U;
    ring->full = false;

    allocator->initialized = true;
    return allocator;
}

/* Destroy allocator */
void rtka_destroy_allocator(rtka_allocator_t* allocator) {
    if (!allocator) return;

    switch (allocator->type) {
        case RTKA_ALLOC_POOL:
#ifdef RTKA_C11_AVAILABLE
            if (allocator->impl.pool.thread_safe) {
                mtx_destroy(&allocator->impl.pool.pool_mutex);
            }
#endif
            free(allocator->impl.pool.memory_base);
            break;
        case RTKA_ALLOC_STACK:
            free(allocator->impl.stack.memory_base);
            break;
        case RTKA_ALLOC_RING:
            free(allocator->impl.ring.memory_base);
            break;
        default:
            break;
    }

    free(allocator);
}

/* Memory allocation */
void* rtka_memory_alloc(rtka_allocator_t* allocator, size_t size) {
    if (!allocator || !allocator->initialized || size == 0U) return NULL;

    void* ptr = NULL;

    switch (allocator->type) {
        case RTKA_ALLOC_POOL: {
            rtka_pool_allocator_t* pool = &allocator->impl.pool;
            if (size > pool->block_size) return NULL;

#ifdef RTKA_C11_AVAILABLE
            if (pool->thread_safe) {
                mtx_lock(&pool->pool_mutex);
            }
#endif

            if (pool->free_list) {
                rtka_pool_block_t* block = pool->free_list;
                pool->free_list = block->next;
                ptr = block->data;

                pool->allocated_count++;
                if (pool->allocated_count > pool->peak_usage) {
                    pool->peak_usage = pool->allocated_count;
                }
            }

#ifdef RTKA_C11_AVAILABLE
            if (pool->thread_safe) {
                mtx_unlock(&pool->pool_mutex);
            }
#endif
            break;
        }

        case RTKA_ALLOC_STACK: {
            rtka_stack_allocator_t* stack = &allocator->impl.stack;
            size_t aligned_size = RTKA_ALIGN_UP(size, stack->alignment);

            if (stack->current_offset + aligned_size <= stack->total_size) {
                ptr = (uint8_t*)stack->memory_base + stack->current_offset;
                stack->current_offset += aligned_size;
            }
            break;
        }

        case RTKA_ALLOC_RING: {
            rtka_ring_allocator_t* ring = &allocator->impl.ring;
            if (size > ring->element_size) return NULL;

            if (!ring->full) {
                ptr = (uint8_t*)ring->memory_base + ring->write_offset;
                ring->write_offset += ring->element_size;

                if (ring->write_offset >= ring->total_size) {
                    ring->write_offset = 0U;
                    ring->full = (ring->write_offset == ring->read_offset);
                }
            }
            break;
        }

        default:
            break;
    }

    if (ptr) {
        allocator->stats.allocation_count++;
        allocator->stats.current_usage += size;
        allocator->stats.total_allocated += size;

        if (allocator->stats.current_usage > allocator->stats.peak_usage) {
            allocator->stats.peak_usage = allocator->stats.current_usage;
        }
    }

    return ptr;
}

/* Memory deallocation */
void rtka_memory_free(rtka_allocator_t* allocator, void* ptr) {
    if (!allocator || !ptr) return;

    if (allocator->type == RTKA_ALLOC_POOL) {
        rtka_pool_allocator_t* pool = &allocator->impl.pool;

#ifdef RTKA_C11_AVAILABLE
        if (pool->thread_safe) {
            mtx_lock(&pool->pool_mutex);
        }
#endif

        rtka_pool_block_t* block = (rtka_pool_block_t*)((uint8_t*)ptr - offsetof(rtka_pool_block_t, data));
        block->next = pool->free_list;
        pool->free_list = block;
        pool->allocated_count--;

#ifdef RTKA_C11_AVAILABLE
        if (pool->thread_safe) {
            mtx_unlock(&pool->pool_mutex);
        }
#endif

        allocator->stats.free_count++;
    }
}

/* Reset allocator */
void rtka_memory_reset(rtka_allocator_t* allocator) {
    if (!allocator) return;

    switch (allocator->type) {
        case RTKA_ALLOC_STACK:
            allocator->impl.stack.current_offset = 0U;
            break;
        case RTKA_ALLOC_RING:
            allocator->impl.ring.write_offset = 0U;
            allocator->impl.ring.read_offset = 0U;
            allocator->impl.ring.full = false;
            break;
        default:
            break;
    }
}

/* RTKA-specific functions */
rtka_state_t* rtka_alloc_state_array(rtka_allocator_t* allocator, uint32_t count) {
    if (!allocator || count == 0U || count > MAX_FACTORS) return NULL;

    size_t total_size = sizeof(rtka_state_t) * count;
    return (rtka_state_t*)rtka_memory_alloc(allocator, total_size);
}

rtka_state_t rtka_recursive_and_managed(rtka_allocator_t* temp_allocator,
                                       const rtka_state_t* states,
                                       uint32_t count) {
    (void)temp_allocator;
    if (!states || count == 0U) return rtka_make_state(RTKA_UNKNOWN, 0.0f);

    return rtka_recursive_and_seq(states, count);
}

rtka_state_t rtka_recursive_or_managed(rtka_allocator_t* temp_allocator,
                                      const rtka_state_t* states,
                                      uint32_t count) {
    (void)temp_allocator;
    if (!states || count == 0U) return rtka_make_state(RTKA_UNKNOWN, 0.0f);

    return rtka_recursive_or_seq(states, count);
}

rtka_state_t rtka_combine_states_managed(rtka_allocator_t* temp_allocator,
                                        const rtka_state_t* states,
                                        uint32_t count) {
    (void)temp_allocator;
    if (!states || count == 0U) return rtka_make_state(RTKA_UNKNOWN, 0.0f);

    return rtka_combine_states(states, count);
}

/* Global management */
rtka_error_t rtka_memory_init_with_config(size_t pool_blocks, size_t stack_size) {
    if (g_memory_initialized) return RTKA_SUCCESS;

    size_t blocks = (pool_blocks > 0) ? pool_blocks : RTKA_DEFAULT_POOL_BLOCKS;
    size_t stack = (stack_size > 0) ? stack_size : RTKA_DEFAULT_STACK_SIZE;

    g_default_state_allocator = rtka_create_pool_allocator(
        sizeof(rtka_state_t), blocks, false);
    if (!g_default_state_allocator) return RTKA_ERROR_OUT_OF_MEMORY;

    g_default_temp_allocator = rtka_create_stack_allocator(stack, 8U);
    if (!g_default_temp_allocator) {
        rtka_destroy_allocator(g_default_state_allocator);
        return RTKA_ERROR_OUT_OF_MEMORY;
    }

    g_memory_initialized = true;
    return RTKA_SUCCESS;
}

rtka_error_t rtka_memory_init(void) {
    return rtka_memory_init_with_config(0, 0);
}

void rtka_memory_cleanup(void) {
    if (!g_memory_initialized) return;

    rtka_destroy_allocator(g_default_state_allocator);
    rtka_destroy_allocator(g_default_temp_allocator);

    g_default_state_allocator = NULL;
    g_default_temp_allocator = NULL;
    g_memory_initialized = false;
}

rtka_allocator_t* rtka_get_default_state_allocator(void) {
    return g_default_state_allocator;
}

rtka_allocator_t* rtka_get_default_temp_allocator(void) {
    return g_default_temp_allocator;
}

rtka_error_t rtka_set_default_allocators(rtka_allocator_t* state_allocator,
                                        rtka_allocator_t* temp_allocator) {
    if (!state_allocator || !temp_allocator) return RTKA_ERROR_NULL_POINTER;

    g_default_state_allocator = state_allocator;
    g_default_temp_allocator = temp_allocator;
    return RTKA_SUCCESS;
}

/* Threading control */
rtka_error_t rtka_enable_threading(rtka_allocator_t* allocator) {
    if (!allocator || !allocator->initialized)
        return RTKA_ERROR_NULL_POINTER;

#ifdef RTKA_C11_AVAILABLE
    if (allocator->type == RTKA_ALLOC_POOL) {
        rtka_pool_allocator_t* pool = &allocator->impl.pool;
        if (!pool->thread_safe) {
            mtx_init(&pool->pool_mutex, mtx_plain);
            atomic_init(&pool->allocated_count, pool->allocated_count);
            atomic_init(&pool->peak_usage, pool->peak_usage);
            pool->thread_safe = true;
        }
    }
    return RTKA_SUCCESS;
#else
    return RTKA_ERROR_NOT_SUPPORTED;
#endif
}

/* Statistics */
rtka_memory_stats_t rtka_get_memory_stats(const rtka_allocator_t* allocator) {
    if (!allocator) {
        rtka_memory_stats_t empty = {0};
        return empty;
    }

    return allocator->stats;
}

bool rtka_check_memory_leaks(const rtka_allocator_t* allocator) {
    if (!allocator) return false;

    return allocator->stats.allocation_count != allocator->stats.free_count;
}

size_t rtka_get_total_memory_usage(void) {
    size_t total = 0U;

    if (g_default_state_allocator) {
        total += g_default_state_allocator->stats.current_usage;
    }

    if (g_default_temp_allocator) {
        total += g_default_temp_allocator->stats.current_usage;
    }

    return total;
}
