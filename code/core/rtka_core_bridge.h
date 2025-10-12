
/**
 * File: rtka_core_bridge.h
 * Copyright (c) 2025 - H.Overman <opsec.ee@pm.me>
 * Email: opsec.ee@pm.me
 *
 * MIT License
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND.
 *
 * RTKA Core Bridge - MIT Licensed Interface
 * Bridges to proprietary rtka_core with comprehensive type system
 *
 * CHANGELOG:
 * v1.0.1 - Initial bridge implementation
 * - Zero-cost abstraction
 * - Module interface support
 * - Performance tracking
 */

#ifndef RTKA_CORE_BRIDGE_H
#define RTKA_CORE_BRIDGE_H

#include "rtka_core.h"
#include "rtka_types.h"

/* Compatibility aliases */
#define rtka_core_and rtka_and
#define rtka_core_or rtka_or
#define rtka_core_not rtka_not
#define rtka_core_equiv rtka_equiv
#define rtka_core_implies rtka_implies

#define rtka_core_conf_and rtka_conf_and
#define rtka_core_conf_or rtka_conf_or
#define rtka_core_conf_not rtka_conf_not
#define rtka_core_conf_equiv rtka_conf_equiv

/* Enhanced operations with error handling */
RTKA_NODISCARD RTKA_INLINE
rtka_result_rtka_state_t rtka_safe_and(rtka_state_t lhs, rtka_state_t rhs) {
    if (!rtka_is_valid_state(&lhs)) {
        return RTKA_ERROR(state, RTKA_ERROR_INVALID_VALUE, "Invalid left operand");
    }
    if (!rtka_is_valid_state(&rhs)) {
        return RTKA_ERROR(state, RTKA_ERROR_INVALID_VALUE, "Invalid right operand");
    }

    rtka_state_t result = rtka_combine_and(lhs, rhs);

    return RTKA_OK(state, result);
}

RTKA_NODISCARD RTKA_INLINE
rtka_result_rtka_state_t rtka_safe_or(rtka_state_t lhs, rtka_state_t rhs) {
    if (!rtka_is_valid_state(&lhs)) {
        return RTKA_ERROR(state, RTKA_ERROR_INVALID_VALUE, "Invalid left operand");
    }
    if (!rtka_is_valid_state(&rhs)) {
        return RTKA_ERROR(state, RTKA_ERROR_INVALID_VALUE, "Invalid right operand");
    }

    rtka_state_t result = rtka_combine_or(lhs, rhs);

    return RTKA_OK(state, result);
}

RTKA_NODISCARD RTKA_INLINE
rtka_result_rtka_state_t rtka_safe_not(rtka_state_t operand) {
    if (!rtka_is_valid_state(&operand)) {
        return RTKA_ERROR(state, RTKA_ERROR_INVALID_VALUE, "Invalid operand");
    }

    rtka_state_t result = rtka_combine_not(operand);

    return RTKA_OK(state, result);
}

/* Vector operations */
RTKA_NODISCARD
rtka_result_rtka_vector_t rtka_vector_and(const rtka_vector_t* lhs, const rtka_vector_t* rhs);

RTKA_NODISCARD
rtka_result_rtka_vector_t rtka_vector_or(const rtka_vector_t* lhs, const rtka_vector_t* rhs);

/* Recursive operations with safety */
RTKA_NODISCARD
rtka_result_rtka_state_t rtka_recursive_and_safe(const rtka_state_t* states, uint32_t count);

RTKA_NODISCARD
rtka_result_rtka_state_t rtka_recursive_or_safe(const rtka_state_t* states, uint32_t count);

/* State conversion */
RTKA_NODISCARD RTKA_INLINE
rtka_state_hp_t rtka_state_to_hp(const rtka_state_t* state) {
    rtka_state_hp_t hp_state = {
        .value = state->value,
        .confidence = (rtka_confidence_hp_t)state->confidence,
        .flags = RTKA_FLAG_VALIDATED,
        .reserved = 0U
    };
    return hp_state;
}

RTKA_NODISCARD RTKA_INLINE
rtka_state_t rtka_state_from_hp(const rtka_state_hp_t* hp_state) {
    rtka_state_t basic_state = {
        .value = hp_state->value,
        .confidence = (rtka_confidence_t)hp_state->confidence
    };
    return basic_state;
}

RTKA_NODISCARD RTKA_INLINE
rtka_state_ext_t rtka_create_extended_state(const rtka_state_t* core_state,
                                           uint32_t source_id,
                                           void* metadata) {
    rtka_state_ext_t ext_state = {
        .core = *core_state,
        .timestamp = 0U,
        .source_id = source_id,
        .depth = 0U,
        .flags = RTKA_FLAG_VALIDATED,
        .metadata = metadata
    };
    return ext_state;
}

/* Module interface */
extern const rtka_module_t rtka_core_module;

RTKA_NODISCARD
rtka_error_t rtka_core_bridge_init(void);

void rtka_core_bridge_cleanup(void);

RTKA_NODISCARD
rtka_performance_t rtka_core_get_performance_metrics(void);

void rtka_core_reset_performance_metrics(void);

/* Compatibility testing */
RTKA_NODISCARD
bool rtka_bridge_compatibility_test(void);

void rtka_bridge_performance_comparison(void);

#endif /* RTKA_CORE_BRIDGE_H */
