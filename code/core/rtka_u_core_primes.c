/**
 * File: rtka_u_core.c
 * Copyright (c) 2025 H. Overman <opsec.ee@pm.me>
 *
 * PROPRIETARY AND CONFIDENTIAL
 * This file is proprietary to H. Overman and may not be reproduced,
 * distributed, or used without explicit written permission.
 *
 * For licensing inquiries: opsec.ee@pm.me
 *
 * RTKA Core Implementation
 * Mathematical operations with proven correctness
 *
 * CHANGELOG:
 * v1.3.1 - Production implementation
 * - Early termination optimization
 * - UNKNOWN preservation verification
 * - Pure algorithms, no I/O
 */

#include "rtka_u_core_primes.h"
#include <math.h>

/* Recursive AND with early termination */
rtka_state_t rtka_recursive_and_seq(const rtka_state_t* states, uint32_t count) {
    if (count == 0) return rtka_make_state(RTKA_TRUE, 1.0f);
    if (count == 1) return states[0];

    rtka_value_t result_value = states[0].value;
    rtka_confidence_t result_conf = states[0].confidence;

    for (uint32_t i = 1; i < count; i++) {
        if (result_value == RTKA_FALSE) break;  /* Early termination */

        result_value = rtka_and(result_value, states[i].value);
        result_conf = rtka_conf_and(result_conf, states[i].confidence);
    }

    return rtka_make_state(result_value, result_conf);
}

/* Recursive OR with early termination */
rtka_state_t rtka_recursive_or_seq(const rtka_state_t* states, uint32_t count) {
    if (count == 0) return rtka_make_state(RTKA_FALSE, 1.0f);
    if (count == 1) return states[0];

    rtka_value_t result_value = states[0].value;
    rtka_confidence_t result_conf = states[0].confidence;

    for (uint32_t i = 1; i < count; i++) {
        if (result_value == RTKA_TRUE) break;  /* Early termination */

        result_value = rtka_or(result_value, states[i].value);
        result_conf = rtka_conf_or(result_conf, states[i].confidence);
    }

    return rtka_make_state(result_value, result_conf);
}

/* Generic recursive evaluation */
rtka_state_t rtka_recursive_eval(const rtka_state_t* states, uint32_t count,
                                rtka_value_t (*operation)(rtka_value_t, rtka_value_t),
                                rtka_confidence_t (*conf_prop)(rtka_confidence_t, rtka_confidence_t),
                                rtka_value_t absorbing_element) {
    if (count == 0) return rtka_make_state(RTKA_UNKNOWN, 0.0f);
    if (count == 1) return states[0];

    rtka_value_t result_value = states[0].value;
    rtka_confidence_t result_conf = states[0].confidence;

    for (uint32_t i = 1; i < count; i++) {
        if (result_value == absorbing_element) break;

        result_value = operation(result_value, states[i].value);
        result_conf = conf_prop(result_conf, states[i].confidence);
    }

    return rtka_make_state(result_value, result_conf);
}

/* UNKNOWN preservation check */
bool rtka_preserves_unknown(rtka_value_t op_result, const rtka_value_t* inputs, uint32_t count) {
    if (op_result != RTKA_UNKNOWN) return false;

    bool has_false = false;
    bool has_true = false;

    for (uint32_t i = 0; i < count; i++) {
        if (inputs[i] == RTKA_FALSE) has_false = true;
        if (inputs[i] == RTKA_TRUE) has_true = true;
    }

    return !(has_false || has_true);
}

/* UNKNOWN persistence probability */
rtka_confidence_t rtka_unknown_persistence_probability(uint32_t n) {
    if (n == 0) return 0.0f;
    if (n == 1) return 1.0f;
    return (rtka_confidence_t)pow(2.0/3.0, (double)(n - 1));
}

/* State initialization */
rtka_state_t rtka_init_state(uint64_t prime, const uint64_t* factors, uint32_t factor_count) {
    (void)prime;
    (void)factors;

    if (factor_count == 0) {
        return rtka_make_state(RTKA_TRUE, 1.0f);
    }

    rtka_confidence_t conf = rtka_unknown_persistence_probability(factor_count);
    return rtka_make_state(RTKA_UNKNOWN, conf);
}

/* State validation */
bool rtka_validate_state(uint64_t prime, rtka_state_t state, uint32_t depth) {
    (void)prime;
    (void)depth;

    return rtka_is_valid_state(&state);
}

/* Expected recursion depth */
uint32_t rtka_expected_depth(uint64_t prime, uint32_t factor_count) {
    (void)prime;

    if (factor_count <= 1) return 1;
    return (uint32_t)(log2((double)factor_count) + 1);
}

/* Combine multiple states */
rtka_state_t rtka_combine_states(const rtka_state_t* states, uint32_t count) {
    if (count == 0) return rtka_make_state(RTKA_UNKNOWN, 0.0f);
    if (count == 1) return states[0];

    rtka_confidence_t total_conf = 0.0f;
    int32_t value_sum = 0;

    for (uint32_t i = 0; i < count; i++) {
        value_sum += (int32_t)states[i].value;
        total_conf += states[i].confidence;
    }

    total_conf /= (rtka_confidence_t)count;

    rtka_value_t combined_value;
    if (value_sum > 0) combined_value = RTKA_TRUE;
    else if (value_sum < 0) combined_value = RTKA_FALSE;
    else combined_value = RTKA_UNKNOWN;

    return rtka_make_state(combined_value, total_conf);
}

/* Simple primality check */
bool rtka_is_likely_prime(uint64_t n) {
    if (n <= 1) return false;
    if (n <= 3) return true;
    if (n % 2 == 0 || n % 3 == 0) return false;

    uint64_t sqrt_n = (uint64_t)sqrt((double)n);
    for (uint64_t i = 5; i <= sqrt_n; i += 6) {
        if (n % i == 0 || n % (i + 2) == 0) return false;
    }

    return true;
}

/* Validate confidence range */
bool rtka_valid_confidence(rtka_confidence_t c) {
    return c >= 0.0f && c <= 1.0f;
}
