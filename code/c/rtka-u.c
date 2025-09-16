/**
 * rtka-u.c
 * Recursive Ternary with Kleene Algorithm + UNKNOWN
 * Implementation file
 */

#include "rtka-u.h"
#include <math.h>

/* Core Kleene operations */
rtka_ternary_t rtka_and(rtka_ternary_t a, rtka_ternary_t b) {
    return (a < b) ? a : b;  /* min operation */
}

rtka_ternary_t rtka_or(rtka_ternary_t a, rtka_ternary_t b) {
    return (a > b) ? a : b;  /* max operation */
}

rtka_ternary_t rtka_not(rtka_ternary_t a) {
    return -a;  /* arithmetic negation */
}

rtka_ternary_t rtka_eqv(rtka_ternary_t a, rtka_ternary_t b) {
    return (rtka_ternary_t)(a * b);  /* multiplication */
}

/* Calculate confidence for AND operation */
static rtka_confidence_t confidence_and(const rtka_confidence_t* confidences, size_t count) {
    rtka_confidence_t result = 1.0;
    for (size_t i = 0; i < count; i++) {
        result *= confidences[i];
    }
    return result;
}

/* Calculate confidence for OR operation */
static rtka_confidence_t confidence_or(const rtka_confidence_t* confidences, size_t count) {
    rtka_confidence_t result = 1.0;
    for (size_t i = 0; i < count; i++) {
        result *= (1.0 - confidences[i]);
    }
    return 1.0 - result;
}

/* Recursive evaluation for AND */
rtka_result_t rtka_eval_and(const rtka_ternary_t* inputs, const rtka_confidence_t* confidences, size_t count) {
    rtka_result_t result = {RTKA_UNKNOWN, 1.0, 0, false};
    
    if (!inputs || count == 0) {
        result.confidence = 0.0;
        return result;
    }
    
    /* Check UNKNOWN level protection */
    size_t unknown_count = 0;
    for (size_t i = 0; i < count; i++) {
        if (inputs[i] == RTKA_UNKNOWN) {
            unknown_count++;
        }
    }
    
    if (unknown_count >= RTKA_MAX_UNKNOWN_LEVELS) {
        result.value = RTKA_FALSE;
        result.confidence = 0.0;
        return result;
    }
    
    /* Evaluate AND operation */
    rtka_ternary_t accumulator = inputs[0];
    
    for (size_t i = 1; i < count; i++) {
        result.operations_performed++;
        
        accumulator = rtka_and(accumulator, inputs[i]);
        
        /* Early termination on FALSE */
        if (accumulator == RTKA_FALSE) {
            result.early_terminated = true;
            break;
        }
    }
    
    result.value = accumulator;
    
    /* Calculate confidence if provided */
    if (confidences) {
        result.confidence = confidence_and(confidences, count);
    }
    
    return result;
}

/* Recursive evaluation for OR */
rtka_result_t rtka_eval_or(const rtka_ternary_t* inputs, const rtka_confidence_t* confidences, size_t count) {
    rtka_result_t result = {RTKA_UNKNOWN, 1.0, 0, false};
    
    if (!inputs || count == 0) {
        result.confidence = 0.0;
        return result;
    }
    
    /* Check UNKNOWN level protection */
    size_t unknown_count = 0;
    for (size_t i = 0; i < count; i++) {
        if (inputs[i] == RTKA_UNKNOWN) {
            unknown_count++;
        }
    }
    
    if (unknown_count >= RTKA_MAX_UNKNOWN_LEVELS) {
        result.value = RTKA_FALSE;
        result.confidence = 0.0;
        return result;
    }
    
    /* Evaluate OR operation */
    rtka_ternary_t accumulator = inputs[0];
    
    for (size_t i = 1; i < count; i++) {
        result.operations_performed++;
        
        accumulator = rtka_or(accumulator, inputs[i]);
        
        /* Early termination on TRUE */
        if (accumulator == RTKA_TRUE) {
            result.early_terminated = true;
            break;
        }
    }
    
    result.value = accumulator;
    
    /* Calculate confidence if provided */
    if (confidences) {
        result.confidence = confidence_or(confidences, count);
    }
    
    return result;
}

/* Convert ternary value to string */
const char* rtka_to_string(rtka_ternary_t value) {
    switch (value) {
        case RTKA_FALSE: return "FALSE";
        case RTKA_UNKNOWN: return "UNKNOWN";
        case RTKA_TRUE: return "TRUE";
        default: return "INVALID";
    }
}

/* Validate ternary value */
bool rtka_is_valid(rtka_ternary_t value) {
    return value == RTKA_FALSE || value == RTKA_UNKNOWN || value == RTKA_TRUE;
}
