/**
 * File: rtka_u_core.h
 * Copyright (c) 2025 H. Overman <opsec.ee@pm.me>
 *
 * PROPRIETARY AND CONFIDENTIAL
 * This file is proprietary to H. Overman and may not be reproduced,
 * distributed, or used without explicit written permission.
 *
 * For licensing inquiries: opsec.ee@pm.me
 *
 * RTKA: Recursive Ternary Knowledge Algorithm
 * Universal core system providing fundamental ternary logic operations
 *
 * CHANGELOG:
 * v1.3.1 - Production-ready core with mathematical rigor
 * - Pure mathematical operations, no dependencies
 * - UNKNOWN Preservation Theorem implementation
 * - Early termination optimization
 */

#ifndef RTKA_U_CORE_H
#define RTKA_U_CORE_H

#include <stdint.h>
#include <stdbool.h>
#include <math.h>
#include "rtka_types.h"

/* Maximum factors for recursive operations */
#define MAX_FACTORS RTKA_MAX_FACTORS

/* Core Kleene operations */
RTKA_INLINE RTKA_PURE rtka_value_t rtka_and(rtka_value_t a, rtka_value_t b) {
    return (a < b) ? a : b;
}

RTKA_INLINE RTKA_PURE rtka_value_t rtka_or(rtka_value_t a, rtka_value_t b) {
    return (a > b) ? a : b;
}

RTKA_INLINE RTKA_PURE rtka_value_t rtka_not(rtka_value_t a) {
    return (rtka_value_t)(-a);
}

RTKA_INLINE RTKA_PURE rtka_value_t rtka_equiv(rtka_value_t a, rtka_value_t b) {
    return (a == b) ? RTKA_TRUE : ((a == RTKA_UNKNOWN || b == RTKA_UNKNOWN) ? RTKA_UNKNOWN : RTKA_FALSE);
}

RTKA_INLINE RTKA_PURE rtka_value_t rtka_imply(rtka_value_t a, rtka_value_t b) {
    return rtka_or(rtka_not(a), b);
}

/* Confidence propagation */
RTKA_INLINE RTKA_PURE rtka_confidence_t rtka_conf_and(rtka_confidence_t a, rtka_confidence_t b) {
    return a * b;
}

RTKA_INLINE RTKA_PURE rtka_confidence_t rtka_conf_or(rtka_confidence_t a, rtka_confidence_t b) {
    return a + b - (a * b);
}

RTKA_INLINE RTKA_PURE rtka_confidence_t rtka_conf_not(rtka_confidence_t a) {
    return a;
}

RTKA_INLINE RTKA_PURE rtka_confidence_t rtka_conf_equiv(rtka_confidence_t a, rtka_confidence_t b) {
    return (a * b) + ((1.0f - a) * (1.0f - b));
}

/* Recursive operations */
rtka_state_t rtka_recursive_and_seq(const rtka_state_t* states, uint32_t count);
rtka_state_t rtka_recursive_or_seq(const rtka_state_t* states, uint32_t count);
rtka_state_t rtka_recursive_eval(const rtka_state_t* states, uint32_t count,
                                 rtka_value_t (*operation)(rtka_value_t, rtka_value_t),
                                 rtka_confidence_t (*conf_prop)(rtka_confidence_t, rtka_confidence_t),
                                 rtka_value_t absorbing_element);

/* State management */
rtka_state_t rtka_init_state(uint64_t prime, const uint64_t* factors, uint32_t factor_count);
bool rtka_validate_state(uint64_t prime, rtka_state_t state, uint32_t depth);
uint32_t rtka_expected_depth(uint64_t prime, uint32_t factor_count);

/* Utility functions */
rtka_confidence_t rtka_unknown_persistence_probability(uint32_t n);
bool rtka_preserves_unknown(rtka_value_t op_result, const rtka_value_t* inputs, uint32_t count);
rtka_state_t rtka_combine_states(const rtka_state_t* states, uint32_t count);
bool rtka_is_likely_prime(uint64_t n);
bool rtka_valid_confidence(rtka_confidence_t c);

#endif /* RTKA_U_CORE_H */
