/**
 * File: rtka_memory.h
 * Copyright (c) 2025 - H.Overman <opsec.ee@pm.me>
 * Email: opsec.ee@pm.me
 *
 * RTKA Memory Management System
 * High-performance allocators optimized for ternary operations
 */

#ifndef RTKA_MEMORY_H
#define RTKA_MEMORY_H

#include "rtka_types.h"
#include "rtka_core.h"

#ifdef RTKA_C11_AVAILABLE
#include <stdatomic.h>
#include <threads.h>
#endif

/* Pool allocator */
typedef struct rtka_pool_block rtka_pool_block_t;
struct rtka_pool_block {
    rtka_pool_block_t* next;
    uint8_t data[];
};

typedef struct {
    void* memory_base;
    size_t total_size;
    size_t block_size;
    size_t block_count;
    rtka_pool_block_t* free_list;

    #ifdef RTKA_C11_AVAILABLE
    atomic_uint_fast32_t allocated_count;
    atomic_uint_fast32_t peak_usage;
    mtx_t pool_mutex;
    #else
    uint32_t allocated_count;
    uint32_t peak_usage;
    #endif

    bool initialized;
    bool thread_safe;
} rtka_pool_allocator_t;

/* Stack allocator */
typedef struct {
    void* memory_base;
    size_t total_size;
    size_t current_offset;
    size_t alignment;
    bool initialized;
} rtka_stack_allocator_t;

/* Ring buffer allocator */
typedef struct {
    void* memory_base;
    size_t total_size;
    size_t write_offset;
    size_t read_offset;
    size_t element_size;
    bool initialized;
    bool full;
} rtka_ring_allocator_t;

/* Generic allocator */
typedef struct {
    rtka_allocator_type_t type;
    const char* name;
    bool initialized;

    union {
        rtka_pool_allocator_t pool;
        rtka_stack_allocator_t stack;
        rtka_ring_allocator_t ring;
    } impl;

    rtka_memory_stats_t stats;
} rtka_allocator_t;

/* Core memory functions */
RTKA_NODISCARD
rtka_error_t rtka_memory_init(void);

RTKA_NODISCARD
rtka_error_t rtka_memory_init_with_config(size_t pool_blocks, size_t stack_size);

void rtka_memory_cleanup(void);

RTKA_NODISCARD
rtka_allocator_t* rtka_create_pool_allocator(size_t block_size, size_t block_count, bool thread_safe);

RTKA_NODISCARD
rtka_allocator_t* rtka_create_stack_allocator(size_t stack_size, size_t alignment);

RTKA_NODISCARD
rtka_allocator_t* rtka_create_ring_allocator(size_t element_size, size_t element_count);

void rtka_destroy_allocator(rtka_allocator_t* allocator);

/* Allocation functions */
RTKA_NODISCARD
void* rtka_memory_alloc(rtka_allocator_t* allocator, size_t size);

void rtka_memory_free(rtka_allocator_t* allocator, void* ptr);

void rtka_memory_reset(rtka_allocator_t* allocator);

/* RTKA-specific allocations */
RTKA_NODISCARD
rtka_state_t* rtka_alloc_state_array(rtka_allocator_t* allocator, uint32_t count);

/* Global allocator access */
RTKA_NODISCARD
rtka_allocator_t* rtka_get_default_state_allocator(void);

RTKA_NODISCARD
rtka_allocator_t* rtka_get_default_temp_allocator(void);

rtka_error_t rtka_set_default_allocators(rtka_allocator_t* state_allocator,
                                         rtka_allocator_t* temp_allocator);

/* Threading control */
rtka_error_t rtka_enable_threading(rtka_allocator_t* allocator);

/* Statistics */
RTKA_NODISCARD
rtka_memory_stats_t rtka_get_memory_stats(const rtka_allocator_t* allocator);

RTKA_NODISCARD
bool rtka_check_memory_leaks(const rtka_allocator_t* allocator);

RTKA_NODISCARD
size_t rtka_get_total_memory_usage(void);

#endif /* RTKA_MEMORY_H */
