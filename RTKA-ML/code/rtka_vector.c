/**
 * File: rtka_vector.c
 * Copyright (c) 2025 - H.Overman opsec.ee@pm.me
 * 
 * PROPRIETARY AND CONFIDENTIAL
 * This file is proprietary to H. Overman and may not be reproduced,
 * distributed, or used without explicit written permission.
 *
 * For licensing inquiries: opsec.ee@pm.me
 *
 * RTKA Vector Operations Implementation
 */

#include "rtka_vector.h"
#include "rtka_memory.h"
#include <string.h>

/* Generic vector AND */
void rtka_vector_and(const rtka_vector_t* RTKA_RESTRICT a,
                    const rtka_vector_t* RTKA_RESTRICT b,
                    rtka_vector_t* RTKA_RESTRICT result) {
    uint32_t count = a->count < b->count ? a->count : b->count;
    
    #ifdef __AVX2__
    if (count >= RTKA_SIMD_THRESHOLD) {
        rtka_vector_and_avx2(a->confidences, b->confidences, 
                           result->confidences, count);
        /* Handle values separately */
        for (uint32_t i = 0; i < count; i++) {
            result->values[i] = rtka_and(a->values[i], b->values[i]);
        }
        result->count = count;
        return;
    }
    #endif
    
    /* Scalar fallback with unrolling */
    uint32_t i = 0;
    for (; i + 3 < count; i += 4) {
        result->values[i] = rtka_and(a->values[i], b->values[i]);
        result->values[i+1] = rtka_and(a->values[i+1], b->values[i+1]);
        result->values[i+2] = rtka_and(a->values[i+2], b->values[i+2]);
        result->values[i+3] = rtka_and(a->values[i+3], b->values[i+3]);
        
        result->confidences[i] = rtka_conf_and(a->confidences[i], b->confidences[i]);
        result->confidences[i+1] = rtka_conf_and(a->confidences[i+1], b->confidences[i+1]);
        result->confidences[i+2] = rtka_conf_and(a->confidences[i+2], b->confidences[i+2]);
        result->confidences[i+3] = rtka_conf_and(a->confidences[i+3], b->confidences[i+3]);
    }
    
    for (; i < count; i++) {
        result->values[i] = rtka_and(a->values[i], b->values[i]);
        result->confidences[i] = rtka_conf_and(a->confidences[i], b->confidences[i]);
    }
    
    result->count = count;
}

/* Vector reduction AND */
rtka_state_t rtka_vector_reduce_and(const rtka_vector_t* vec) {
    if (vec->count == 0) return rtka_make_state(RTKA_TRUE, 1.0f);
    
    rtka_value_t val = vec->values[0];
    rtka_confidence_t conf = vec->confidences[0];
    
    for (uint32_t i = 1; i < vec->count; i++) {
        if (val == RTKA_FALSE) break;  /* Early termination */
        val = rtka_and(val, vec->values[i]);
        conf = rtka_conf_and(conf, vec->confidences[i]);
    }
    
    return rtka_make_state(val, conf);
}

/* Vector broadcast */
void rtka_vector_broadcast(rtka_vector_t* vec, rtka_state_t value) {
    /* Use memset for special cases */
    if (value.value == RTKA_FALSE && value.confidence == 0.0f) {
        memset(vec->values, RTKA_FALSE, vec->capacity * sizeof(rtka_value_t));
        memset(vec->confidences, 0, vec->capacity * sizeof(rtka_confidence_t));
        vec->count = vec->capacity;
        return;
    }
    
    /* General case with unrolling */
    uint32_t i = 0;
    for (; i + 7 < vec->capacity; i += 8) {
        vec->values[i] = vec->values[i+1] = vec->values[i+2] = vec->values[i+3] = 
        vec->values[i+4] = vec->values[i+5] = vec->values[i+6] = vec->values[i+7] = value.value;
        
        vec->confidences[i] = vec->confidences[i+1] = vec->confidences[i+2] = vec->confidences[i+3] =
        vec->confidences[i+4] = vec->confidences[i+5] = vec->confidences[i+6] = vec->confidences[i+7] = value.confidence;
    }
    
    for (; i < vec->capacity; i++) {
        vec->values[i] = value.value;
        vec->confidences[i] = value.confidence;
    }
    
    vec->count = vec->capacity;
}

#ifdef __AVX2__
/* AVX2 confidence multiplication */
void rtka_vector_and_avx2(const float* RTKA_RESTRICT conf_a,
                         const float* RTKA_RESTRICT conf_b,
                         float* RTKA_RESTRICT conf_result,
                         uint32_t count) {
    uint32_t simd_count = count & ~7u;  /* Round down to multiple of 8 */
    
    for (uint32_t i = 0; i < simd_count; i += 8) {
        __m256 a = _mm256_load_ps(&conf_a[i]);
        __m256 b = _mm256_load_ps(&conf_b[i]);
        __m256 result = _mm256_mul_ps(a, b);  /* AND confidence = multiply */
        _mm256_store_ps(&conf_result[i], result);
    }
    
    /* Handle remainder */
    for (uint32_t i = simd_count; i < count; i++) {
        conf_result[i] = conf_a[i] * conf_b[i];
    }
}

void rtka_vector_or_avx2(const float* RTKA_RESTRICT conf_a,
                        const float* RTKA_RESTRICT conf_b,
                        float* RTKA_RESTRICT conf_result,
                        uint32_t count) {
    uint32_t simd_count = count & ~7u;
    
    for (uint32_t i = 0; i < simd_count; i += 8) {
        __m256 a = _mm256_load_ps(&conf_a[i]);
        __m256 b = _mm256_load_ps(&conf_b[i]);
        __m256 ab = _mm256_mul_ps(a, b);
        __m256 sum = _mm256_add_ps(a, b);
        __m256 result = _mm256_sub_ps(sum, ab);  /* OR confidence = a + b - ab */
        _mm256_store_ps(&conf_result[i], result);
    }
    
    for (uint32_t i = simd_count; i < count; i++) {
        conf_result[i] = conf_a[i] + conf_b[i] - conf_a[i] * conf_b[i];
    }
}
#endif

/* SIMD detection */
bool rtka_simd_available(void) {
    #ifdef __AVX2__
    return true;
    #elif defined(__ARM_NEON)
    return true;
    #else
    return false;
    #endif
}

uint32_t rtka_simd_width(void) {
    #ifdef __AVX2__
    return 8;  /* 8 floats in AVX */
    #elif defined(__ARM_NEON)
    return 4;  /* 4 floats in NEON */
    #else
    return 1;
    #endif
}
