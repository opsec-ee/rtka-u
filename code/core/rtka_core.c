/**
 * File: rtka_core.c
 * Copyright (c) 2025 - H.Overman opsec.ee@pm.me
 * Email: opsec.ee@pm.me
 * 
 * RTKA Core Library Implementation
 */

#include "rtka_core.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

/* ============================================================================
 * GLOBAL STATE
 * ============================================================================ */

static bool g_rtka_initialized = false;

/* ============================================================================
 * ADVANCED CONFIDENCE FUNCTIONS
 * ============================================================================ */

rtka_confidence_t rtka_geometric_mean(const rtka_confidence_t* confs, uint32_t n) {
    if (n == 0) return 1.0f;
    if (n == 1) return confs[0];
    
    double product = 1.0;
    for (uint32_t i = 0; i < n; i++) {
        product *= confs[i];
    }
    return (rtka_confidence_t)pow(product, 1.0 / (double)n);
}

rtka_confidence_t rtka_conf_and_n(const rtka_confidence_t* confs, uint32_t n) {
    if (n == 0) return 1.0f;
    
    rtka_confidence_t result = confs[0];
    for (uint32_t i = 1; i < n; i++) {
        result *= confs[i];
    }
    return result;
}

rtka_confidence_t rtka_conf_or_n(const rtka_confidence_t* confs, uint32_t n) {
    if (n == 0) return 0.0f;
    
    double inv_product = 1.0;
    for (uint32_t i = 0; i < n; i++) {
        inv_product *= (1.0 - confs[i]);
    }
    return (rtka_confidence_t)(1.0 - inv_product);
}

rtka_confidence_t rtka_conf_weighted(
    const rtka_confidence_t* confs,
    const rtka_confidence_t* weights,
    uint32_t n
) {
    if (n == 0) return 0.0f;
    
    double sum = 0.0;
    double weight_sum = 0.0;
    
    for (uint32_t i = 0; i < n; i++) {
        sum += confs[i] * weights[i];
        weight_sum += weights[i];
    }
    
    return (weight_sum > 0.0) ? (rtka_confidence_t)(sum / weight_sum) : 0.0f;
}

/* ============================================================================
 * RECURSIVE SEQUENCE OPERATIONS
 * ============================================================================ */

rtka_state_t rtka_recursive_and(const rtka_state_t* states, uint32_t count) {
    if (RTKA_UNLIKELY(count == 0)) {
        return rtka_make_state(RTKA_TRUE, 1.0f);
    }
    if (count == 1) {
        return states[0];
    }
    
    rtka_value_t result_value = states[0].value;
    rtka_confidence_t result_conf = states[0].confidence;
    
    for (uint32_t i = 1; i < count; i++) {
        /* Early termination on FALSE */
        if (RTKA_UNLIKELY(result_value == RTKA_FALSE)) {
            break;
        }
        
        result_value = rtka_and(result_value, states[i].value);
        result_conf = rtka_conf_and(result_conf, states[i].confidence);
    }
    
    return rtka_make_state(result_value, result_conf);
}

rtka_state_t rtka_recursive_or(const rtka_state_t* states, uint32_t count) {
    if (RTKA_UNLIKELY(count == 0)) {
        return rtka_make_state(RTKA_FALSE, 1.0f);
    }
    if (count == 1) {
        return states[0];
    }
    
    rtka_value_t result_value = states[0].value;
    rtka_confidence_t result_conf = states[0].confidence;
    
    for (uint32_t i = 1; i < count; i++) {
        /* Early termination on TRUE */
        if (RTKA_UNLIKELY(result_value == RTKA_TRUE)) {
            break;
        }
        
        result_value = rtka_or(result_value, states[i].value);
        result_conf = rtka_conf_or(result_conf, states[i].confidence);
    }
    
    return rtka_make_state(result_value, result_conf);
}

rtka_state_t rtka_recursive_nand(const rtka_state_t* states, uint32_t count) {
    rtka_state_t and_result = rtka_recursive_and(states, count);
    return rtka_combine_not(and_result);
}

rtka_state_t rtka_recursive_nor(const rtka_state_t* states, uint32_t count) {
    rtka_state_t or_result = rtka_recursive_or(states, count);
    return rtka_combine_not(or_result);
}

/* ============================================================================
 * BATCH OPERATIONS
 * ============================================================================ */

void rtka_and_batch(
    const rtka_state_t* a,
    const rtka_state_t* b,
    rtka_state_t* result,
    uint32_t count
) {
    uint32_t i = 0;
    
    /* Unroll by 4 for better instruction-level parallelism */
    for (; i + 3 < count; i += 4) {
        result[i] = rtka_combine_and(a[i], b[i]);
        result[i+1] = rtka_combine_and(a[i+1], b[i+1]);
        result[i+2] = rtka_combine_and(a[i+2], b[i+2]);
        result[i+3] = rtka_combine_and(a[i+3], b[i+3]);
    }
    
    for (; i < count; i++) {
        result[i] = rtka_combine_and(a[i], b[i]);
    }
}

void rtka_or_batch(
    const rtka_state_t* a,
    const rtka_state_t* b,
    rtka_state_t* result,
    uint32_t count
) {
    uint32_t i = 0;
    
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

void rtka_nand_batch(
    const rtka_state_t* a,
    const rtka_state_t* b,
    rtka_state_t* result,
    uint32_t count
) {
    uint32_t i = 0;
    
    for (; i + 3 < count; i += 4) {
        result[i] = rtka_combine_nand(a[i], b[i]);
        result[i+1] = rtka_combine_nand(a[i+1], b[i+1]);
        result[i+2] = rtka_combine_nand(a[i+2], b[i+2]);
        result[i+3] = rtka_combine_nand(a[i+3], b[i+3]);
    }
    
    for (; i < count; i++) {
        result[i] = rtka_combine_nand(a[i], b[i]);
    }
}

void rtka_nor_batch(
    const rtka_state_t* a,
    const rtka_state_t* b,
    rtka_state_t* result,
    uint32_t count
) {
    uint32_t i = 0;
    
    for (; i + 3 < count; i += 4) {
        result[i] = rtka_combine_nor(a[i], b[i]);
        result[i+1] = rtka_combine_nor(a[i+1], b[i+1]);
        result[i+2] = rtka_combine_nor(a[i+2], b[i+2]);
        result[i+3] = rtka_combine_nor(a[i+3], b[i+3]);
    }
    
    for (; i < count; i++) {
        result[i] = rtka_combine_nor(a[i], b[i]);
    }
}

void rtka_not_batch(
    const rtka_state_t* a,
    rtka_state_t* result,
    uint32_t count
) {
    uint32_t i = 0;
    
    for (; i + 3 < count; i += 4) {
        result[i] = rtka_combine_not(a[i]);
        result[i+1] = rtka_combine_not(a[i+1]);
        result[i+2] = rtka_combine_not(a[i+2]);
        result[i+3] = rtka_combine_not(a[i+3]);
    }
    
    for (; i < count; i++) {
        result[i] = rtka_combine_not(a[i]);
    }
}

/* ============================================================================
 * STATE COUNTING AND STATISTICS
 * ============================================================================ */

rtka_state_counts_t rtka_count_states(const rtka_state_t* states, uint32_t count) {
    rtka_state_counts_t counts = {0, 0, 0, 0.0f, 1.0f, 0.0f};
    
    if (count == 0) return counts;
    
    double conf_sum = 0.0;
    counts.min_confidence = states[0].confidence;
    counts.max_confidence = states[0].confidence;
    
    for (uint32_t i = 0; i < count; i++) {
        switch (states[i].value) {
            case RTKA_TRUE:
                counts.true_count++;
                break;
            case RTKA_FALSE:
                counts.false_count++;
                break;
            case RTKA_UNKNOWN:
                counts.unknown_count++;
                break;
        }
        
        conf_sum += states[i].confidence;
        
        if (states[i].confidence < counts.min_confidence) {
            counts.min_confidence = states[i].confidence;
        }
        if (states[i].confidence > counts.max_confidence) {
            counts.max_confidence = states[i].confidence;
        }
    }
    
    counts.avg_confidence = (rtka_confidence_t)(conf_sum / (double)count);
    return counts;
}

/* ============================================================================
 * UTILITY FUNCTIONS
 * ============================================================================ */

const char* rtka_value_to_string(rtka_value_t value) {
    switch (value) {
        case RTKA_FALSE: return "FALSE";
        case RTKA_UNKNOWN: return "UNKNOWN";
        case RTKA_TRUE: return "TRUE";
        default: return "INVALID";
    }
}

const char* rtka_error_to_string(rtka_error_t error) {
    switch (error) {
        case RTKA_SUCCESS: return "SUCCESS";
        case RTKA_ERROR_NULL_POINTER: return "NULL_POINTER";
        case RTKA_ERROR_INVALID_VALUE: return "INVALID_VALUE";
        case RTKA_ERROR_INVALID_DIMENSION: return "INVALID_DIMENSION";
        case RTKA_ERROR_OUT_OF_MEMORY: return "OUT_OF_MEMORY";
        case RTKA_ERROR_NOT_INITIALIZED: return "NOT_INITIALIZED";
        case RTKA_ERROR_ALREADY_INITIALIZED: return "ALREADY_INITIALIZED";
        case RTKA_ERROR_MODULE_INIT_FAILED: return "MODULE_INIT_FAILED";
        case RTKA_ERROR_NOT_SUPPORTED: return "NOT_SUPPORTED";
        case RTKA_ERROR_OVERFLOW: return "OVERFLOW";
        case RTKA_ERROR_UNDERFLOW: return "UNDERFLOW";
        case RTKA_ERROR_TIMEOUT: return "TIMEOUT";
        case RTKA_ERROR_PERMISSION_DENIED: return "PERMISSION_DENIED";
        case RTKA_ERROR_IO_FAILURE: return "IO_FAILURE";
        default: return "UNKNOWN_ERROR";
    }
}

void rtka_print_state(const rtka_state_t* state) {
    if (!state) {
        printf("NULL state\n");
        return;
    }
    printf("State{value=%s, confidence=%.3f}\n",
           rtka_value_to_string(state->value),
           state->confidence);
}

/* ============================================================================
 * LIBRARY INITIALIZATION
 * ============================================================================ */

rtka_error_t rtka_core_init(void) {
    if (g_rtka_initialized) {
        return RTKA_ERROR_ALREADY_INITIALIZED;
    }
    
    g_rtka_initialized = true;
    return RTKA_SUCCESS;
}

void rtka_core_cleanup(void) {
    g_rtka_initialized = false;
}

bool rtka_core_is_initialized(void) {
    return g_rtka_initialized;
}
