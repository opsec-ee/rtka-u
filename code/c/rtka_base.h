/**
 * File: rtka_base.h
 * Author: H. Overman
 * Date: 2025-01-14
 * Copyright (c) 2025 - H. Overman opsec.ee@pm.me
 * https://orcid.org/0009-0007-9737-762X
 * 
 * RTKA-U Universal Core - Base Header
 * 
 * This header defines the fundamental types and operations for the
 * Recursive Ternary Knowledge Algorithm universal system. All RTKA
 * modules depend on these definitions for mathematical consistency.
 * 
 * Licensed under CC BY-NC-SA 4.0 for research and non-commercial use.
 */

#ifndef RTKA_BASE_H
#define RTKA_BASE_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/* ============================================================================
 * VERSION INFORMATION
 * ============================================================================ */

#define RTKA_U_VERSION_MAJOR 0
#define RTKA_U_VERSION_MINOR 1
#define RTKA_U_VERSION_PATCH 0

#define RTKA_U_VERSION ((RTKA_U_VERSION_MAJOR << 16) | \
                       (RTKA_U_VERSION_MINOR << 8) | \
                       RTKA_U_VERSION_PATCH)

/* ============================================================================
 * COMPILER OPTIMIZATION HINTS
 * ============================================================================ */

#ifdef __GNUC__
    #define RTKA_ALWAYS_INLINE __attribute__((always_inline)) inline
    #define RTKA_LIKELY(x) __builtin_expect(!!(x), 1)
    #define RTKA_UNLIKELY(x) __builtin_expect(!!(x), 0)
    #define RTKA_RESTRICT __restrict
#else
    #define RTKA_ALWAYS_INLINE inline
    #define RTKA_LIKELY(x) (x)
    #define RTKA_UNLIKELY(x) (x)
    #define RTKA_RESTRICT
#endif

/* ============================================================================
 * CORE TYPE DEFINITIONS
 * ============================================================================ */

/**
 * Fundamental ternary value enumeration implementing Kleene's strong
 * three-valued logic with arithmetic encoding for efficient computation.
 * 
 * The encoding enables arithmetic operations:
 * - Negation: -x
 * - Conjunction (AND): min(x, y)
 * - Disjunction (OR): max(x, y)
 */
typedef enum {
    RTKA_FALSE = -1,    /* Definitive false state */
    RTKA_UNKNOWN = 0,   /* Uncertain/maybe state (preserves through operations) */
    RTKA_TRUE = 1       /* Definitive true state */
} rtka_value_t;

/**
 * Confidence type for uncertainty quantification.
 * Values must remain within [0.0, 1.0] bounds.
 */
typedef float rtka_confidence_t;

/**
 * Error codes returned by RTKA operations.
 * Modules extend this enumeration with domain-specific codes.
 */
typedef enum {
    RTKA_SUCCESS = 0,
    RTKA_ERROR_NULL_PARAMETER = -1,
    RTKA_ERROR_INVALID_VALUE = -2,
    RTKA_ERROR_CONFIDENCE_BOUNDS = -3,
    RTKA_ERROR_OVERFLOW = -4,
    RTKA_ERROR_UNIMPLEMENTED = -99
} rtka_error_t;

/* ============================================================================
 * FUNDAMENTAL TERNARY OPERATIONS
 * ============================================================================ */

/**
 * Ternary AND operation (conjunction).
 * Implements Kleene's strong conjunction as minimum function.
 * 
 * Truth table:
 *        FALSE  UNKNOWN  TRUE
 * FALSE   FALSE   FALSE   FALSE
 * UNKNOWN FALSE  UNKNOWN UNKNOWN
 * TRUE    FALSE  UNKNOWN  TRUE
 */
RTKA_ALWAYS_INLINE rtka_value_t rtka_and(rtka_value_t a, rtka_value_t b) {
    return (a < b) ? a : b;
}

/**
 * Ternary OR operation (disjunction).
 * Implements Kleene's strong disjunction as maximum function.
 * 
 * Truth table:
 *        FALSE  UNKNOWN  TRUE
 * FALSE  FALSE  UNKNOWN  TRUE
 * UNKNOWN UNKNOWN UNKNOWN TRUE
 * TRUE    TRUE    TRUE   TRUE
 */
RTKA_ALWAYS_INLINE rtka_value_t rtka_or(rtka_value_t a, rtka_value_t b) {
    return (a > b) ? a : b;
}

/**
 * Ternary NOT operation (negation).
 * Implements logical negation through arithmetic sign reversal.
 * 
 * Mapping:
 * FALSE -> TRUE
 * UNKNOWN -> UNKNOWN
 * TRUE -> FALSE
 */
RTKA_ALWAYS_INLINE rtka_value_t rtka_not(rtka_value_t a) {
    return (rtka_value_t)(-a);
}

/**
 * Ternary IMPLICATION operation.
 * Derived operation: a → b ≡ ¬a ∨ b
 */
RTKA_ALWAYS_INLINE rtka_value_t rtka_imply(rtka_value_t a, rtka_value_t b) {
    return rtka_or(rtka_not(a), b);
}

/**
 * Ternary EQUIVALENCE operation.
 * Returns TRUE if values are identical, UNKNOWN if either is UNKNOWN,
 * FALSE if values differ and neither is UNKNOWN.
 */
RTKA_ALWAYS_INLINE rtka_value_t rtka_equiv(rtka_value_t a, rtka_value_t b) {
    if (a == b) return RTKA_TRUE;
    if (a == RTKA_UNKNOWN || b == RTKA_UNKNOWN) return RTKA_UNKNOWN;
    return RTKA_FALSE;
}

/* ============================================================================
 * CONFIDENCE PROPAGATION OPERATIONS
 * ============================================================================ */

/**
 * Confidence propagation for AND operation.
 * Uses multiplicative combination reflecting joint probability.
 */
RTKA_ALWAYS_INLINE rtka_confidence_t conf_and(rtka_confidence_t a, rtka_confidence_t b) {
    return a * b;
}

/**
 * Confidence propagation for OR operation.
 * Uses inclusion-exclusion principle: P(A ∨ B) = P(A) + P(B) - P(A ∧ B)
 */
RTKA_ALWAYS_INLINE rtka_confidence_t conf_or(rtka_confidence_t a, rtka_confidence_t b) {
    return 1.0f - (1.0f - a) * (1.0f - b);
}

/**
 * Confidence propagation for NOT operation.
 * Confidence remains unchanged through negation.
 */
RTKA_ALWAYS_INLINE rtka_confidence_t conf_not(rtka_confidence_t a) {
    return a;
}

/**
 * Confidence propagation for IMPLICATION operation.
 * Derived from NOT and OR operations.
 */
RTKA_ALWAYS_INLINE rtka_confidence_t conf_imply(rtka_confidence_t a, rtka_confidence_t b) {
    return conf_or(1.0f - a, b);
}

/**
 * Confidence propagation for EQUIVALENCE operation.
 * Product of confidences when values match.
 */
RTKA_ALWAYS_INLINE rtka_confidence_t conf_equiv(rtka_confidence_t a, rtka_confidence_t b) {
    return a * b;
}

/* ============================================================================
 * UTILITY FUNCTIONS
 * ============================================================================ */

/**
 * Validate confidence value is within bounds.
 * Returns clamped value within [0.0, 1.0].
 */
RTKA_ALWAYS_INLINE rtka_confidence_t rtka_clamp_confidence(rtka_confidence_t conf) {
    if (conf < 0.0f) return 0.0f;
    if (conf > 1.0f) return 1.0f;
    return conf;
}

/**
 * Check if value is valid ternary state.
 */
RTKA_ALWAYS_INLINE bool rtka_is_valid(rtka_value_t value) {
    return (value == RTKA_FALSE || value == RTKA_UNKNOWN || value == RTKA_TRUE);
}

/**
 * Convert ternary value to string representation.
 * Returns static string pointer (do not free).
 */
RTKA_ALWAYS_INLINE const char* rtka_value_to_string(rtka_value_t value) {
    switch(value) {
        case RTKA_FALSE: return "FALSE";
        case RTKA_UNKNOWN: return "UNKNOWN";
        case RTKA_TRUE: return "TRUE";
        default: return "INVALID";
    }
}

/* ============================================================================
 * RECURSIVE TERNARY STRUCTURES
 * ============================================================================ */

/**
 * Forward declaration for recursive ternary node.
 * Modules extend this structure for domain-specific applications.
 */
typedef struct rtka_node rtka_node_t;

/**
 * Recursive ternary node structure.
 * When value is UNKNOWN, can recursively reference additional nodes
 * for progressive uncertainty refinement.
 */
struct rtka_node {
    rtka_value_t value;
    rtka_confidence_t confidence;
    
    /* Recursive refinement when UNKNOWN */
    rtka_node_t* if_false;  /* Explore FALSE possibility */
    rtka_node_t* if_unknown; /* Remain UNKNOWN (recurse) */
    rtka_node_t* if_true;    /* Explore TRUE possibility */
    
    /* Probability distribution for branches */
    rtka_confidence_t prob_false;
    rtka_confidence_t prob_unknown;
    rtka_confidence_t prob_true;
    
    /* Recursion depth tracking */
    uint32_t depth;
};

/* ============================================================================
 * MEMORY ALIGNMENT DEFINITIONS
 * ============================================================================ */

/**
 * Standard cache line size for alignment.
 * Modules should align performance-critical structures to this boundary.
 */
#define RTKA_CACHE_LINE_SIZE 64U

/**
 * Memory alignment macro for structure declarations.
 * Usage: RTKA_ALIGN(64) typedef struct { ... } aligned_struct_t;
 */
#ifdef __GNUC__
    #define RTKA_ALIGN(n) __attribute__((aligned(n)))
#elif defined(_MSC_VER)
    #define RTKA_ALIGN(n) __declspec(align(n))
#else
    #define RTKA_ALIGN(n)
#endif

/* ============================================================================
 * MODULE INTEGRATION HOOKS 
 * ============================================================================ */

/**
 * Module initialization callback type.
 * Modules register initialization functions with the core.
 */
typedef rtka_error_t (*rtka_module_init_fn)(void);

/**
 * Module cleanup callback type.
 * Modules register cleanup functions for orderly shutdown.
 */
typedef void (*rtka_module_cleanup_fn)(void);

/**
 * Module descriptor structure.
 * Modules provide this structure for registration with the core.
 */
typedef struct {
    const char* name;           /* Module identifier */
    uint32_t version;          /* Module version */
    uint32_t required_core;    /* Minimum core version required */
    rtka_module_init_fn init;     /* Initialization function */
    rtka_module_cleanup_fn cleanup; /* Cleanup function */
} rtka_module_descriptor_t;

#ifdef __cplusplus
}
#endif

#endif /* RTKA_BASE_H */
