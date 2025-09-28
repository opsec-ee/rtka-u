/**
 * File: rtka_vector.h
 * Copyright (c) 2025 - H.Overman opsec.ee@pm.me
 * 
 * PROPRIETARY AND CONFIDENTIAL
 * This file is proprietary to H. Overman and may not be reproduced,
 * distributed, or used without explicit written permission.
 *
 * For licensing inquiries: opsec.ee@pm.me
 *
 * RTKA Vector Operations - SIMD Optimized
 */

#ifndef RTKA_VECTOR_H
#define RTKA_VECTOR_H

#include "rtka_types.h"
#include "rtka_u_core.h"

#ifdef __AVX2__
#include <immintrin.h>
#endif

#ifdef __ARM_NEON
#include <arm_neon.h>
#endif

/* Vector operation flags */
#define RTKA_VEC_ALIGNED   (1U << 0)
#define RTKA_VEC_CONTIGUOUS (1U << 1)
#define RTKA_VEC_SIMD_READY (1U << 2)

/* SIMD-optimized vector operations */
void rtka_vector_and(const rtka_vector_t* RTKA_RESTRICT a,
                    const rtka_vector_t* RTKA_RESTRICT b,
                    rtka_vector_t* RTKA_RESTRICT result);

void rtka_vector_or(const rtka_vector_t* RTKA_RESTRICT a,
                   const rtka_vector_t* RTKA_RESTRICT b,
                   rtka_vector_t* RTKA_RESTRICT result);

void rtka_vector_not(const rtka_vector_t* RTKA_RESTRICT input,
                    rtka_vector_t* RTKA_RESTRICT result);

/* Reduction operations */
rtka_state_t rtka_vector_reduce_and(const rtka_vector_t* vec);
rtka_state_t rtka_vector_reduce_or(const rtka_vector_t* vec);

/* Broadcast operations */
void rtka_vector_broadcast(rtka_vector_t* vec, rtka_state_t value);
void rtka_vector_fill_range(rtka_vector_t* vec, uint32_t start, uint32_t end, rtka_state_t value);

/* Element-wise confidence operations */
void rtka_vector_conf_multiply(rtka_vector_t* vec, rtka_confidence_t scalar);
void rtka_vector_conf_normalize(rtka_vector_t* vec);

/* SIMD detection and dispatch */
bool rtka_simd_available(void);
uint32_t rtka_simd_width(void);

#ifdef __AVX2__
/* AVX2 implementations */
void rtka_vector_and_avx2(const float* RTKA_RESTRICT conf_a,
                         const float* RTKA_RESTRICT conf_b,
                         float* RTKA_RESTRICT conf_result,
                         uint32_t count);

void rtka_vector_or_avx2(const float* RTKA_RESTRICT conf_a,
                        const float* RTKA_RESTRICT conf_b,
                        float* RTKA_RESTRICT conf_result,
                        uint32_t count);
#endif

#ifdef __ARM_NEON
/* NEON implementations */
void rtka_vector_and_neon(const float* RTKA_RESTRICT conf_a,
                         const float* RTKA_RESTRICT conf_b,
                         float* RTKA_RESTRICT conf_result,
                         uint32_t count);
#endif

/* Parallel operations for large vectors */
void rtka_vector_parallel_and(const rtka_vector_t* RTKA_RESTRICT a,
                             const rtka_vector_t* RTKA_RESTRICT b,
                             rtka_vector_t* RTKA_RESTRICT result,
                             uint32_t num_threads);

#endif /* RTKA_VECTOR_H */
