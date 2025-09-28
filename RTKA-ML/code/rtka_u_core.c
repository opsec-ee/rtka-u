/**
 * File: rtka_u_core.c
 * Copyright (c) 2025 - H.Overman opsec.ee@pm.me
 * 
 * PROPRIETARY AND CONFIDENTIAL
 * This file is proprietary to H. Overman and may not be reproduced,
 * distributed, or used without explicit written permission.
 *
 * For licensing inquiries: opsec.ee@pm.me
 *
 * RTKA Core Implementation - Optimized
 */

#include "rtka_u_core.h"
#include <math.h>
#include <string.h>

/* Lookup tables for O(1) ternary operations */
#if RTKA_USE_LOOKUP_TABLES

const rtka_value_t rtka_and_table[3][3] = {
    /* [FALSE] */ {RTKA_FALSE, RTKA_FALSE, RTKA_FALSE},
    /* [UNKNOWN] */ {RTKA_FALSE, RTKA_UNKNOWN, RTKA_UNKNOWN},
    /* [TRUE] */ {RTKA_FALSE, RTKA_UNKNOWN, RTKA_TRUE}
};

const rtka_value_t rtka_or_table[3][3] = {
    /* [FALSE] */ {RTKA_FALSE, RTKA_UNKNOWN, RTKA_TRUE},
    /* [UNKNOWN] */ {RTKA_UNKNOWN, RTKA_UNKNOWN, RTKA_TRUE},
    /* [TRUE] */ {RTKA_TRUE, RTKA_TRUE, RTKA_TRUE}
};

/* Precomputed confidence values for common operations */
const rtka_confidence_t rtka_conf_table[256] = {
    /* Initialize with common confidence combinations */
    [0] = 0.0f, [255] = 1.0f, [128] = 0.5f,
    [85] = 0.333333f, [170] = 0.666667f,
    [64] = 0.25f, [192] = 0.75f
    /* Compiler will zero-init the rest */
};

#endif

/* Optimized batch operations using loop unrolling */
void rtka_and_batch(const rtka_state_t* RTKA_RESTRICT a, 
                   const rtka_state_t* RTKA_RESTRICT b,
                   rtka_state_t* RTKA_RESTRICT result,
                   uint32_t count) {
    uint32_t i = 0;
    
    /* Unroll by 4 for better pipelining */
    for (; i + 3 < count; i += 4) {
        result[i] = rtka_combine_and(a[i], b[i]);
        result[i+1] = rtka_combine_and(a[i+1], b[i+1]);
        result[i+2] = rtka_combine_and(a[i+2], b[i+2]);
        result[i+3] = rtka_combine_and(a[i+3], b[i+3]);
    }
    
    /* Handle remaining */
    for (; i < count; i++) {
        result[i] = rtka_combine_and(a[i], b[i]);
    }
}

void rtka_or_batch(const rtka_state_t* RTKA_RESTRICT a,
                  const rtka_state_t* RTKA_RESTRICT b,
                  rtka_state_t* RTKA_RESTRICT result,
                  uint32_t count) {
    uint32_t i = 0;
    
    /* Unroll by 4 */
    for (; i + 3 < count; i += 4) {
        result[i] = rtka_combine_or(a[i], b[i]);
        result[i+1] = rtka_combine_or(a[i+1], b[i+1]);
        result[i+2] = rtka_combine_or(a[i+2], b[i+2]);
        result[i+3] = rtka_combine_or(a[i+3], b[i+3]);
    }
    
    for (; i < count; i++) {
        result[i] = rtka_combine_or(a[i], b[i]);
    }
}

/* Recursive AND with early termination */
rtka_state_t rtka_recursive_and_seq(const rtka_state_t* states, uint32_t count) {
    if (RTKA_UNLIKELY(count == 0)) 
        return rtka_make_state(RTKA_TRUE, 1.0f);
    if (count == 1) 
        return states[0];

    rtka_value_t result_value = states[0].value;
    rtka_confidence_t result_conf = states[0].confidence;

    for (uint32_t i = 1; i < count; i++) {
        #if RTKA_EARLY_TERMINATION
        if (RTKA_UNLIKELY(result_value == RTKA_FALSE)) 
            break;  /* Early exit on FALSE */
        #endif
        
        result_value = rtka_and(result_value, states[i].value);
        result_conf = rtka_conf_and(result_conf, states[i].confidence);
    }

    return rtka_make_state(result_value, result_conf);
}

/* Recursive OR with early termination */
rtka_state_t rtka_recursive_or_seq(const rtka_state_t* states, uint32_t count) {
    if (RTKA_UNLIKELY(count == 0)) 
        return rtka_make_state(RTKA_FALSE, 1.0f);
    if (count == 1) 
        return states[0];

    rtka_value_t result_value = states[0].value;
    rtka_confidence_t result_conf = states[0].confidence;

    for (uint32_t i = 1; i < count; i++) {
        #if RTKA_EARLY_TERMINATION
        if (RTKA_UNLIKELY(result_value == RTKA_TRUE)) 
            break;  /* Early exit on TRUE */
        #endif
        
        result_value = rtka_or(result_value, states[i].value);
        result_conf = rtka_conf_or(result_conf, states[i].confidence);
    }

    return rtka_make_state(result_value, result_conf);
}

/* UNKNOWN preservation check */
bool rtka_preserves_unknown(rtka_value_t result, const rtka_value_t* inputs, uint32_t count) {
    if (result != RTKA_UNKNOWN) return false;
    
    #if RTKA_UNKNOWN_PRESERVATION
    bool has_unknown = false;
    for (uint32_t i = 0; i < count; i++) {
        if (inputs[i] == RTKA_UNKNOWN) {
            has_unknown = true;
            break;
        }
    }
    return has_unknown;
    #else
    (void)inputs;  /* Suppress warning */
    (void)count;
    return true;
    #endif
}

/* UNKNOWN persistence probability calculation */
rtka_confidence_t rtka_unknown_persistence_probability(uint32_t n) {
    if (n == 0) return 0.0f;
    if (n == 1) return 1.0f;
    
    /* Use fast approximation for large n */
    if (n > 16) {
        return (rtka_confidence_t)(0.666667f / n);
    }
    
    /* Exact calculation for small n */
    return (rtka_confidence_t)pow(2.0/3.0, (double)(n - 1));
}
