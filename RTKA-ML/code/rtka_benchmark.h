/**
 * File: rtka_benchmark.h
 * Copyright (c) 2025 - H.Overman opsec.ee@pm.me
 * 
 * RTKA Performance Benchmarking
 */

#ifndef RTKA_BENCHMARK_H
#define RTKA_BENCHMARK_H

#include "rtka_types.h"
#include <time.h>

typedef struct {
    const char* name;
    uint64_t iterations;
    double total_time;
    double min_time;
    double max_time;
    double avg_time;
    uint64_t ops_per_sec;
} rtka_benchmark_result_t;

typedef void (*rtka_benchmark_fn)(void* data);

/* Run benchmark */
rtka_benchmark_result_t rtka_benchmark_run(const char* name, 
                                          rtka_benchmark_fn fn,
                                          void* data,
                                          uint64_t iterations);

/* Core operation benchmarks */
void rtka_benchmark_core_ops(void);
void rtka_benchmark_tensor_ops(void);
void rtka_benchmark_solver(void);
void rtka_benchmark_all(void);

/* Timing utilities */
double rtka_get_time(void);
void rtka_benchmark_print_results(rtka_benchmark_result_t* result);

#endif /* RTKA_BENCHMARK_H */
