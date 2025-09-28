/**
 * File: rtka_u_core.h
 * Copyright (c) 2025 - H.Overman opsec.ee@pm.me
 * 
 * RTKA Universal Core Operations
 *
 * PROPRIETARY AND CONFIDENTIAL
 * This file is proprietary to H. Overman and may not be reproduced,
 * distributed, or used without explicit written permission.
 *
 * For licensing inquiries: opsec.ee@pm.me
 *
 * CHANGELOG:
 * v1.0.1 - Added missing recursive function declarations
 *          Added missing confidence operations (rtka_conf_not, rtka_conf_equiv)
 *          Lines added: 48-52, 68-72
 */

#ifndef RTKA_U_CORE_H
#define RTKA_U_CORE_H

#include "rtka_types.h"

/* Core Kleene ternary operations */
static inline rtka_value_t rtka_and(rtka_value_t a, rtka_value_t b) {
    if (a == RTKA_FALSE || b == RTKA_FALSE) return RTKA_FALSE;
    if (a == RTKA_UNKNOWN || b == RTKA_UNKNOWN) return RTKA_UNKNOWN;
    return RTKA_TRUE;
}

static inline rtka_value_t rtka_or(rtka_value_t a, rtka_value_t b) {
    if (a == RTKA_TRUE || b == RTKA_TRUE) return RTKA_TRUE;
    if (a == RTKA_UNKNOWN || b == RTKA_UNKNOWN) return RTKA_UNKNOWN;
    return RTKA_FALSE;
}

static inline rtka_value_t rtka_not(rtka_value_t a) {
    return -a;  /* -1 → 1, 0 → 0, 1 → -1 */
}

static inline rtka_value_t rtka_implies(rtka_value_t a, rtka_value_t b) {
    return rtka_or(rtka_not(a), b);
}

static inline rtka_value_t rtka_equiv(rtka_value_t a, rtka_value_t b) {
    if (a == RTKA_UNKNOWN || b == RTKA_UNKNOWN) return RTKA_UNKNOWN;
    return (a == b) ? RTKA_TRUE : RTKA_FALSE;
}

/* Confidence operations */
static inline rtka_confidence_t rtka_conf_and(rtka_confidence_t a, rtka_confidence_t b) {
    return a * b;
}

static inline rtka_confidence_t rtka_conf_or(rtka_confidence_t a, rtka_confidence_t b) {
    return a + b - (a * b);
}

static inline rtka_confidence_t rtka_conf_not(rtka_confidence_t a) {
    return a; /* Confidence preserved through negation */
}

static inline rtka_confidence_t rtka_conf_equiv(rtka_confidence_t a, rtka_confidence_t b) {
    return (a + b) / 2.0f; /* Average confidence for equivalence */
}

/* State combinations */
static inline rtka_state_t rtka_combine_and(rtka_state_t a, rtka_state_t b) {
    return (rtka_state_t){
        .value = rtka_and(a.value, b.value),
        .confidence = rtka_conf_and(a.confidence, b.confidence)
    };
}

static inline rtka_state_t rtka_combine_or(rtka_state_t a, rtka_state_t b) {
    return (rtka_state_t){
        .value = rtka_or(a.value, b.value),
        .confidence = rtka_conf_or(a.confidence, b.confidence)
    };
}

/* Batch operations - defined in rtka_u_core.c */
void rtka_and_batch(const rtka_state_t* a, const rtka_state_t* b, 
                   rtka_state_t* result, uint32_t count);
void rtka_or_batch(const rtka_state_t* a, const rtka_state_t* b,
                  rtka_state_t* result, uint32_t count);

/* Recursive sequence operations - defined in rtka_u_core.c */
rtka_state_t rtka_recursive_and_seq(const rtka_state_t* states, uint32_t count);
rtka_state_t rtka_recursive_or_seq(const rtka_state_t* states, uint32_t count);

#endif /* RTKA_U_CORE_H */
