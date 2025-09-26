/**
 * File: rtka_core_bridge.h
 * Copyright (c) 2025 - H.Overman <opsec.ee@pm.me>
 * Email: opsec.ee@pm.me
 * 
 * RTKA Core Bridge - Unified Interface
 * Bridges the proven rtka_u_core implementation with the comprehensive rtka_types system
 * Maintains backward compatibility while enabling advanced features
 *
 * CHANGELOG:
 * v1.0.0 - Initial bridge implementation
 * - Preserves rtka_u_core mathematical correctness
 * - Extends with rtka_types advanced features
 * - Maintains full backward compatibility
 * - Enables gradual migration path
 * 
 * DESIGN RATIONALE:
 * The rtka_u_core.h/.c files are mathematically sound and well-implemented.
 * Rather than replace this proven code, we extend it with the advanced features
 * from rtka_types.h, creating a unified system that preserves existing correctness
 * while enabling new capabilities for specialized modules.
 */

#ifndef RTKA_CORE_BRIDGE_H
#define RTKA_CORE_BRIDGE_H

/* Include the proven core implementation */
#include "rtka_u_core.h"

/* Include the comprehensive type system */
#include "rtka_types.h"

/* ============================================================================
 * COMPATIBILITY LAYER - Maps existing implementation to new types
 * ============================================================================ */

/* Core types are identical - rtka_u_core.h already defines what we need */
/* rtka_value_t, rtka_confidence_t, rtka_state_t are compatible */

/* Alias existing functions to match rtka_types.h expectations */
#define rtka_core_and rtka_and
#define rtka_core_or rtka_or
#define rtka_core_not rtka_not
#define rtka_core_equiv rtka_equiv
#define rtka_core_imply rtka_imply

#define rtka_core_conf_and rtka_conf_and
#define rtka_core_conf_or rtka_conf_or
#define rtka_core_conf_not rtka_conf_not
#define rtka_core_conf_equiv rtka_conf_equiv

/* ============================================================================
 * ENHANCED OPERATIONS - Build on existing core for advanced features
 * ============================================================================ */

/**
 * Enhanced state operations using rtka_u_core as foundation
 */
RTKA_NODISCARD RTKA_INLINE
rtka_result_rtka_state_t rtka_safe_and(rtka_state_t lhs, rtka_state_t rhs) {
    if (!rtka_is_valid_state(&lhs)) {
        return RTKA_ERROR(rtka_state, RTKA_ERROR_INVALID_VALUE, "Invalid left operand");
    }
    if (!rtka_is_valid_state(&rhs)) {
        return RTKA_ERROR(rtka_state, RTKA_ERROR_INVALID_VALUE, "Invalid right operand");
    }
    
    rtka_state_t result = {
        .value = rtka_and(lhs.value, rhs.value),
        .confidence = rtka_conf_and(lhs.confidence, rhs.confidence)
    };
    
    return RTKA_OK(rtka_state, result);
}

RTKA_NODISCARD RTKA_INLINE
rtka_result_rtka_state_t rtka_safe_or(rtka_state_t lhs, rtka_state_t rhs) {
    if (!rtka_is_valid_state(&lhs)) {
        return RTKA_ERROR(rtka_state, RTKA_ERROR_INVALID_VALUE, "Invalid left operand");
    }
    if (!rtka_is_valid_state(&rhs)) {
        return RTKA_ERROR(rtka_state, RTKA_ERROR_INVALID_VALUE, "Invalid right operand");
    }
    
    rtka_state_t result = {
        .value = rtka_or(lhs.value, rhs.value),
        .confidence = rtka_conf_or(lhs.confidence, rhs.confidence)
    };
    
    return RTKA_OK(rtka_state, result);
}

RTKA_NODISCARD RTKA_INLINE
rtka_result_rtka_state_t rtka_safe_not(rtka_state_t operand) {
    if (!rtka_is_valid_state(&operand)) {
        return RTKA_ERROR(rtka_state, RTKA_ERROR_INVALID_VALUE, "Invalid operand");
    }
    
    rtka_state_t result = {
        .value = rtka_not(operand.value),
        .confidence = rtka_conf_not(operand.confidence)
    };
    
    return RTKA_OK(rtka_state, result);
}

/* ============================================================================
 * VECTOR OPERATIONS - Extend core for SIMD processing
 * ============================================================================ */

/**
 * Batch AND operation using proven rtka_u_core functions
 */
RTKA_NODISCARD
rtka_result_rtka_vector_t rtka_vector_and(const rtka_vector_t* lhs, const rtka_vector_t* rhs) {
    if (!lhs || !rhs) {
        return RTKA_ERROR(rtka_vector, RTKA_ERROR_NULL_POINTER, "Null vector operand");
    }
    
    if (lhs->count != rhs->count) {
        return RTKA_ERROR(rtka_vector, RTKA_ERROR_INVALID_DIMENSION, "Vector size mismatch");
    }
    
    rtka_vector_t result = {0};
    result.count = lhs->count;
    
    for (uint32_t i = 0U; i < lhs->count && i < RTKA_VECTOR_SIZE; i++) {
        result.values[i] = rtka_and(lhs->values[i], rhs->values[i]);
        result.confidences[i] = rtka_conf_and(lhs->confidences[i], rhs->confidences[i]);
    }
    
    return RTKA_OK(rtka_vector, result);
}

/**
 * Batch OR operation using proven rtka_u_core functions
 */
RTKA_NODISCARD
rtka_result_rtka_vector_t rtka_vector_or(const rtka_vector_t* lhs, const rtka_vector_t* rhs) {
    if (!lhs || !rhs) {
        return RTKA_ERROR(rtka_vector, RTKA_ERROR_NULL_POINTER, "Null vector operand");
    }
    
    if (lhs->count != rhs->count) {
        return RTKA_ERROR(rtka_vector, RTKA_ERROR_INVALID_DIMENSION, "Vector size mismatch");
    }
    
    rtka_vector_t result = {0};
    result.count = lhs->count;
    
    for (uint32_t i = 0U; i < lhs->count && i < RTKA_VECTOR_SIZE; i++) {
        result.values[i] = rtka_or(lhs->values[i], rhs->values[i]);
        result.confidences[i] = rtka_conf_or(lhs->confidences[i], rhs->confidences[i]);
    }
    
    return RTKA_OK(rtka_vector, result);
}

/* ============================================================================
 * RECURSIVE OPERATIONS - Bridge to existing proven implementations
 * ============================================================================ */

/**
 * Enhanced recursive AND using rtka_u_core proven algorithm
 */
RTKA_NODISCARD
rtka_result_rtka_state_t rtka_recursive_and_safe(const rtka_state_t* states, uint32_t count) {
    if (!states && count > 0U) {
        return RTKA_ERROR(rtka_state, RTKA_ERROR_NULL_POINTER, "Null states array");
    }
    
    if (count > RTKA_MAX_FACTORS) {
        return RTKA_ERROR(rtka_state, RTKA_ERROR_INVALID_DIMENSION, "Too many states for recursive operation");
    }
    
    /* Validate all input states */
    for (uint32_t i = 0U; i < count; i++) {
        if (!rtka_is_valid_state(&states[i])) {
            return RTKA_ERROR(rtka_state, RTKA_ERROR_INVALID_VALUE, "Invalid state in array");
        }
    }
    
    /* Use proven rtka_u_core implementation */
    rtka_state_t result = rtka_recursive_and_seq(states, count);
    
    return RTKA_OK(rtka_state, result);
}

/**
 * Enhanced recursive OR using rtka_u_core proven algorithm
 */
RTKA_NODISCARD
rtka_result_rtka_state_t rtka_recursive_or_safe(const rtka_state_t* states, uint32_t count) {
    if (!states && count > 0U) {
        return RTKA_ERROR(rtka_state, RTKA_ERROR_NULL_POINTER, "Null states array");
    }
    
    if (count > RTKA_MAX_FACTORS) {
        return RTKA_ERROR(rtka_state, RTKA_ERROR_INVALID_DIMENSION, "Too many states for recursive operation");
    }
    
    /* Validate all input states */
    for (uint32_t i = 0U; i < count; i++) {
        if (!rtka_is_valid_state(&states[i])) {
            return RTKA_ERROR(rtka_state, RTKA_ERROR_INVALID_VALUE, "Invalid state in array");
        }
    }
    
    /* Use proven rtka_u_core implementation */
    rtka_state_t result = rtka_recursive_or_seq(states, count);
    
    return RTKA_OK(rtka_state, result);
}

/* ============================================================================
 * STATE CONVERSION - Bridge between type systems
 * ============================================================================ */

/**
 * Convert basic state to high-precision state
 */
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

/**
 * Convert high-precision state to basic state (with precision loss warning)
 */
RTKA_NODISCARD RTKA_INLINE
rtka_state_t rtka_state_from_hp(const rtka_state_hp_t* hp_state) {
    rtka_state_t basic_state = {
        .value = hp_state->value,
        .confidence = (rtka_confidence_t)hp_state->confidence  /* Precision loss here */
    };
    return basic_state;
}

/**
 * Create extended state with metadata
 */
RTKA_NODISCARD RTKA_INLINE
rtka_state_ext_t rtka_create_extended_state(const rtka_state_t* core_state, 
                                           uint32_t source_id, 
                                           void* metadata) {
    rtka_state_ext_t ext_state = {
        .core = *core_state,
        .timestamp = 0U,  /* Set by caller if needed */
        .source_id = source_id,
        .depth = 0U,
        .flags = RTKA_FLAG_VALIDATED,
        .metadata = metadata
    };
    return ext_state;
}

/* ============================================================================
 * MODULE INTERFACE IMPLEMENTATION - Universal module support
 * ============================================================================ */

/**
 * Standard RTKA core module implementing the universal interface
 * This allows rtka_u_core to be treated as a module by specialized systems
 */
extern const rtka_module_t rtka_core_module;

/**
 * Initialize the core module bridge
 */
RTKA_NODISCARD
rtka_error_t rtka_core_bridge_init(void);

/**
 * Cleanup the core module bridge  
 */
void rtka_core_bridge_cleanup(void);

/**
 * Get performance metrics from core operations
 */
RTKA_NODISCARD
rtka_performance_t rtka_core_get_performance_metrics(void);

/**
 * Reset performance counters
 */
void rtka_core_reset_performance_metrics(void);

/* ============================================================================
 * COMPATIBILITY TESTING FUNCTIONS
 * ============================================================================ */

/**
 * Validate that rtka_u_core and rtka_types produce identical results
 */
RTKA_NODISCARD
bool rtka_bridge_compatibility_test(void);

/**
 * Benchmark performance differences between implementations
 */
void rtka_bridge_performance_comparison(void);

/* ============================================================================
 * MIGRATION UTILITIES
 * ============================================================================ */

/**
 * Check if code is using legacy rtka_u_core directly
 */
#define RTKA_LEGACY_WARNING(func) \
    _Pragma("GCC warning \"Consider migrating to rtka_core_bridge for enhanced features\"")

/* Legacy function wrappers with migration warnings */
#ifdef RTKA_ENABLE_MIGRATION_WARNINGS
    #define rtka_and(a, b) (RTKA_LEGACY_WARNING(rtka_and), rtka_and(a, b))
    #define rtka_or(a, b) (RTKA_LEGACY_WARNING(rtka_or), rtka_or(a, b))
#endif

#endif /* RTKA_CORE_BRIDGE_H */
