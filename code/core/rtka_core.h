/**
 * File: rtka_core.h
 * Copyright (c) 2025 - H.Overman opsec.ee@pm.me
 * Email: opsec.ee@pm.me
 * 
 * RTKA Core Operations - Extended with NAND/NOR
 * 
 * CHANGELOG:
 * 
 * v1.1.0 - 2025-10-11:
 * ADDED (lines ~80-145):
 *   - rtka_nand(): NAND operation for ternary values
 *   - rtka_nor(): NOR operation for ternary values
 *   - rtka_conf_nand(): Confidence propagation for NAND
 *   - rtka_conf_nor(): Confidence propagation for NOR
 *   - rtka_combine_nand(): State combination for NAND
 *   - rtka_combine_nor(): State combination for NOR
 *   - rtka_nand_batch(): Batch NAND operation
 *   - rtka_nor_batch(): Batch NOR operation
 *   - rtka_recursive_nand(): Recursive NAND with early termination
 *   - rtka_recursive_nor(): Recursive NOR with early termination
 * REASON: Complete the fundamental logic gate set with universal gates.
 *         NAND and NOR provide flexibility for circuit design and
 *         fault-tolerant systems while maintaining mathematical rigor.
 * 
 * This file extends the existing RTKA type system defined in rtka_types.h
 * and uses constants from rtka_constants.h.
 */

#ifndef RTKA_CORE_H
#define RTKA_CORE_H

#include "rtka_types.h"
#include "rtka_constants.h"

/* ============================================================================
 * CORE KLEENE OPERATIONS
 * Arithmetic encodings per mathematical framework in rtka-u.tex
 * ============================================================================ */

/**
 * Conjunction: a ∧ b = min(a, b)
 * Implements strong Kleene AND operation
 */
RTKA_NODISCARD RTKA_PURE RTKA_INLINE
rtka_value_t rtka_and(rtka_value_t a, rtka_value_t b) {
    return (a < b) ? a : b;
}

/**
 * Disjunction: a ∨ b = max(a, b)
 * Implements strong Kleene OR operation
 */
RTKA_NODISCARD RTKA_PURE RTKA_INLINE
rtka_value_t rtka_or(rtka_value_t a, rtka_value_t b) {
    return (a > b) ? a : b;
}

/**
 * Negation: ¬a = -a
 * Maps: FALSE → TRUE, UNKNOWN → UNKNOWN, TRUE → FALSE
 */
RTKA_NODISCARD RTKA_PURE RTKA_INLINE
rtka_value_t rtka_not(rtka_value_t a) {
    return (rtka_value_t)(-((int)a));
}

/**
 * NAND: a ⊼ b = ¬(a ∧ b)
 * Universal gate - can construct all other operations
 * Returns TRUE when at least one input is FALSE
 */
RTKA_NODISCARD RTKA_PURE RTKA_INLINE
rtka_value_t rtka_nand(rtka_value_t a, rtka_value_t b) {
    return rtka_not(rtka_and(a, b));
}

/**
 * NOR: a ⊽ b = ¬(a ∨ b)
 * Universal gate - can construct all other operations
 * Returns TRUE only when both inputs are FALSE
 */
RTKA_NODISCARD RTKA_PURE RTKA_INLINE
rtka_value_t rtka_nor(rtka_value_t a, rtka_value_t b) {
    return rtka_not(rtka_or(a, b));
}

/**
 * Implication: a → b = max(-a, b)
 * Derived from ¬a ∨ b
 */
RTKA_NODISCARD RTKA_PURE RTKA_INLINE
rtka_value_t rtka_implies(rtka_value_t a, rtka_value_t b) {
    return rtka_or(rtka_not(a), b);
}

/**
 * Equivalence: a ↔ b
 * Returns TRUE when both values are equal and certain
 */
RTKA_NODISCARD RTKA_PURE RTKA_INLINE
rtka_value_t rtka_equiv(rtka_value_t a, rtka_value_t b) {
    if (a == RTKA_UNKNOWN || b == RTKA_UNKNOWN) return RTKA_UNKNOWN;
    return (a == b) ? RTKA_TRUE : RTKA_FALSE;
}

/**
 * XOR: a ⊕ b
 * TRUE when operands differ
 */
RTKA_NODISCARD RTKA_PURE RTKA_INLINE
rtka_value_t rtka_xor(rtka_value_t a, rtka_value_t b) {
    if (a == RTKA_UNKNOWN || b == RTKA_UNKNOWN) return RTKA_UNKNOWN;
    return (a != b) ? RTKA_TRUE : RTKA_FALSE;
}

/* ============================================================================
 * CONFIDENCE PROPAGATION
 * Per mathematical framework in rtka-u.tex
 * ============================================================================ */

/**
 * Conjunction confidence: C∧ = c₁ × c₂
 * Multiplicative rule for AND operations
 */
RTKA_NODISCARD RTKA_PURE RTKA_INLINE
rtka_confidence_t rtka_conf_and(rtka_confidence_t c1, rtka_confidence_t c2) {
    return c1 * c2;
}

/**
 * Disjunction confidence: C∨ = 1 - (1-c₁)(1-c₂)
 * Inclusion-exclusion principle for OR operations
 */
RTKA_NODISCARD RTKA_PURE RTKA_INLINE
rtka_confidence_t rtka_conf_or(rtka_confidence_t c1, rtka_confidence_t c2) {
    return c1 + c2 - c1 * c2;
}

/**
 * Negation confidence: C¬ = c
 * Confidence preserved through negation
 */
RTKA_NODISCARD RTKA_PURE RTKA_INLINE
rtka_confidence_t rtka_conf_not(rtka_confidence_t c) {
    return c;
}

/**
 * NAND confidence: C⊼ = c₁ × c₂
 * Since NAND = NOT(AND) and NOT preserves confidence
 */
RTKA_NODISCARD RTKA_PURE RTKA_INLINE
rtka_confidence_t rtka_conf_nand(rtka_confidence_t c1, rtka_confidence_t c2) {
    return rtka_conf_and(c1, c2);
}

/**
 * NOR confidence: C⊽ = 1 - (1-c₁)(1-c₂)
 * Since NOR = NOT(OR) and NOT preserves confidence
 */
RTKA_NODISCARD RTKA_PURE RTKA_INLINE
rtka_confidence_t rtka_conf_nor(rtka_confidence_t c1, rtka_confidence_t c2) {
    return rtka_conf_or(c1, c2);
}

/**
 * Implication confidence
 */
RTKA_NODISCARD RTKA_PURE RTKA_INLINE
rtka_confidence_t rtka_conf_implies(rtka_confidence_t c1, rtka_confidence_t c2) {
    return rtka_conf_or(c1, c2);
}

/**
 * Equivalence confidence: average
 */
RTKA_NODISCARD RTKA_PURE RTKA_INLINE
rtka_confidence_t rtka_conf_equiv(rtka_confidence_t c1, rtka_confidence_t c2) {
    return (c1 + c2) * 0.5f;
}

/**
 * XOR confidence
 */
RTKA_NODISCARD RTKA_PURE RTKA_INLINE
rtka_confidence_t rtka_conf_xor(rtka_confidence_t c1, rtka_confidence_t c2) {
    return rtka_conf_and(c1, c2);
}

/* ============================================================================
 * STATE COMBINATION OPERATIONS
 * Combines value logic with confidence propagation
 * ============================================================================ */

RTKA_NODISCARD RTKA_PURE RTKA_INLINE
rtka_state_t rtka_combine_and(rtka_state_t a, rtka_state_t b) {
    return rtka_make_state(
        rtka_and(a.value, b.value),
        rtka_conf_and(a.confidence, b.confidence)
    );
}

RTKA_NODISCARD RTKA_PURE RTKA_INLINE
rtka_state_t rtka_combine_or(rtka_state_t a, rtka_state_t b) {
    return rtka_make_state(
        rtka_or(a.value, b.value),
        rtka_conf_or(a.confidence, b.confidence)
    );
}

RTKA_NODISCARD RTKA_PURE RTKA_INLINE
rtka_state_t rtka_combine_not(rtka_state_t a) {
    return rtka_make_state(
        rtka_not(a.value),
        rtka_conf_not(a.confidence)
    );
}

RTKA_NODISCARD RTKA_PURE RTKA_INLINE
rtka_state_t rtka_combine_nand(rtka_state_t a, rtka_state_t b) {
    return rtka_make_state(
        rtka_nand(a.value, b.value),
        rtka_conf_nand(a.confidence, b.confidence)
    );
}

RTKA_NODISCARD RTKA_PURE RTKA_INLINE
rtka_state_t rtka_combine_nor(rtka_state_t a, rtka_state_t b) {
    return rtka_make_state(
        rtka_nor(a.value, b.value),
        rtka_conf_nor(a.confidence, b.confidence)
    );
}

RTKA_NODISCARD RTKA_PURE RTKA_INLINE
rtka_state_t rtka_combine_implies(rtka_state_t a, rtka_state_t b) {
    return rtka_make_state(
        rtka_implies(a.value, b.value),
        rtka_conf_implies(a.confidence, b.confidence)
    );
}

RTKA_NODISCARD RTKA_PURE RTKA_INLINE
rtka_state_t rtka_combine_equiv(rtka_state_t a, rtka_state_t b) {
    return rtka_make_state(
        rtka_equiv(a.value, b.value),
        rtka_conf_equiv(a.confidence, b.confidence)
    );
}

RTKA_NODISCARD RTKA_PURE RTKA_INLINE
rtka_state_t rtka_combine_xor(rtka_state_t a, rtka_state_t b) {
    return rtka_make_state(
        rtka_xor(a.value, b.value),
        rtka_conf_xor(a.confidence, b.confidence)
    );
}

/* ============================================================================
 * ADVANCED CONFIDENCE FUNCTIONS
 * For sensor fusion and multi-source aggregation
 * ============================================================================ */

/**
 * Geometric mean of confidences
 * Used in sensor fusion: ∛(c₁ × c₂ × ... × cₙ)
 */
RTKA_NODISCARD
rtka_confidence_t rtka_geometric_mean(const rtka_confidence_t* confs, uint32_t n);

/**
 * N-ary AND confidence: ∏cᵢ
 */
RTKA_NODISCARD
rtka_confidence_t rtka_conf_and_n(const rtka_confidence_t* confs, uint32_t n);

/**
 * N-ary OR confidence: 1 - ∏(1-cᵢ)
 */
RTKA_NODISCARD
rtka_confidence_t rtka_conf_or_n(const rtka_confidence_t* confs, uint32_t n);

/**
 * Weighted confidence fusion
 */
RTKA_NODISCARD
rtka_confidence_t rtka_conf_weighted(
    const rtka_confidence_t* confs,
    const rtka_confidence_t* weights,
    uint32_t n
);

/* ============================================================================
 * RECURSIVE SEQUENCE OPERATIONS
 * With early termination optimization per mathematical framework
 * ============================================================================ */

/**
 * Recursive AND over sequence
 * Early terminates on FALSE (40-60% performance improvement)
 */
RTKA_NODISCARD
rtka_state_t rtka_recursive_and(const rtka_state_t* states, uint32_t count);

/**
 * Recursive OR over sequence
 * Early terminates on TRUE (40-60% performance improvement)
 */
RTKA_NODISCARD
rtka_state_t rtka_recursive_or(const rtka_state_t* states, uint32_t count);

/**
 * Recursive NAND over sequence
 * Returns: NOT(AND(states[0], ..., states[n-1]))
 */
RTKA_NODISCARD
rtka_state_t rtka_recursive_nand(const rtka_state_t* states, uint32_t count);

/**
 * Recursive NOR over sequence
 * Returns: NOT(OR(states[0], ..., states[n-1]))
 */
RTKA_NODISCARD
rtka_state_t rtka_recursive_nor(const rtka_state_t* states, uint32_t count);

/* ============================================================================
 * BATCH OPERATIONS
 * Optimized for cache efficiency
 * ============================================================================ */

/**
 * Batch AND: result[i] = a[i] AND b[i]
 */
void rtka_and_batch(
    const rtka_state_t* a,
    const rtka_state_t* b,
    rtka_state_t* result,
    uint32_t count
);

/**
 * Batch OR: result[i] = a[i] OR b[i]
 */
void rtka_or_batch(
    const rtka_state_t* a,
    const rtka_state_t* b,
    rtka_state_t* result,
    uint32_t count
);

/**
 * Batch NAND: result[i] = a[i] NAND b[i]
 */
void rtka_nand_batch(
    const rtka_state_t* a,
    const rtka_state_t* b,
    rtka_state_t* result,
    uint32_t count
);

/**
 * Batch NOR: result[i] = a[i] NOR b[i]
 */
void rtka_nor_batch(
    const rtka_state_t* a,
    const rtka_state_t* b,
    rtka_state_t* result,
    uint32_t count
);

/**
 * Batch NOT: result[i] = NOT a[i]
 */
void rtka_not_batch(
    const rtka_state_t* a,
    rtka_state_t* result,
    uint32_t count
);

/* ============================================================================
 * STATE COUNTING AND STATISTICS
 * ============================================================================ */

typedef struct {
    uint32_t true_count;
    uint32_t false_count;
    uint32_t unknown_count;
    rtka_confidence_t avg_confidence;
    rtka_confidence_t min_confidence;
    rtka_confidence_t max_confidence;
} rtka_state_counts_t;

RTKA_NODISCARD
rtka_state_counts_t rtka_count_states(const rtka_state_t* states, uint32_t count);

/* ============================================================================
 * MATHEMATICAL FRAMEWORK SUPPORT
 * ============================================================================ */

/**
 * UNKNOWN persistence probability
 * P(UNKNOWN | n) = (2/3)^(n-1)
 */
RTKA_NODISCARD RTKA_INLINE
double rtka_unknown_persistence(uint32_t n) {
    if (n == 0) return 1.0;
    return pow(2.0/3.0, (double)(n - 1));
}

/* ============================================================================
 * UTILITY FUNCTIONS
 * ============================================================================ */

/**
 * Compare states (for sorting)
 */
RTKA_NODISCARD RTKA_PURE RTKA_INLINE
int rtka_compare_states(const rtka_state_t* a, const rtka_state_t* b) {
    if (a->value != b->value) return (int)a->value - (int)b->value;
    if (a->confidence < b->confidence) return -1;
    if (a->confidence > b->confidence) return 1;
    return 0;
}

/**
 * State equality (with epsilon for confidence)
 */
RTKA_NODISCARD RTKA_PURE RTKA_INLINE
bool rtka_states_equal(const rtka_state_t* a, const rtka_state_t* b) {
    return a->value == b->value && 
           fabsf(a->confidence - b->confidence) < RTKA_CONFIDENCE_EPSILON;
}

/**
 * String representations
 */
const char* rtka_value_to_string(rtka_value_t value);
const char* rtka_error_to_string(rtka_error_t error);

/**
 * Print state (for debugging)
 */
void rtka_print_state(const rtka_state_t* state);

/* ============================================================================
 * LIBRARY INITIALIZATION
 * ============================================================================ */

/**
 * Initialize core library
 * Call once before using library
 */
RTKA_NODISCARD
rtka_error_t rtka_core_init(void);

/**
 * Cleanup core library
 */
void rtka_core_cleanup(void);

/**
 * Check if library is initialized
 */
bool rtka_core_is_initialized(void);

#endif /* RTKA_CORE_H */
