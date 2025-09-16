/**
 * RTKA-U: Recursive Ternary with Kleene Algorithm + UNKNOWN
 * 
 * This implementation follows the mathematical specification for a ternary logic
 * system with uncertainty preservation and confidence propagation.
 * 
 * Mathematical Domain: T = {-1, 0, 1} representing {FALSE, UNKNOWN, TRUE}
 * Operations: ¬a = -a, a ∧ b = min(a,b), a ∨ b = max(a,b), a ↔ b = a × b
 *
 * Author: H.Overman <opsec.ee@pm.ee>
 */

#ifndef RTKA_U_H
#define RTKA_U_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>
#include <math.h>

// C23 compatibility macros
#if __STDC_VERSION__ >= 202311L
    #define RTKA_U_TYPEOF typeof
#else
    #define RTKA_U_TYPEOF __typeof__
#endif

// Function declarations - use static inline for header-defined functions
#define RTKA_U_INLINE static inline

// Ternary domain values - using strongly typed enum for type safety
typedef enum {
    RTKA_FALSE = -1,
    RTKA_UNKNOWN = 0, 
    RTKA_TRUE = 1
} rtka_ternary_t;

// Operation types for recursive application
typedef enum {
    RTKA_OP_AND = 0,     // ∧ᵣ - recursive conjunction
    RTKA_OP_OR = 1,      // ∨ᵣ - recursive disjunction  
    RTKA_OP_NOT = 2,     // ¬ᵣ - recursive negation
    RTKA_OP_EQV = 3      // ↔ᵣ - recursive equivalence
} rtka_operation_t;

// Confidence value in [0,1] range with high precision
typedef double rtka_confidence_t;

// Vector structure for ternary values with confidence weights
typedef struct {
    rtka_ternary_t *values;          // Ternary values
    rtka_confidence_t *confidences;  // Associated confidence weights
    size_t length;                   // Vector length
    size_t capacity;                 // Allocated capacity
} rtka_vector_t;

// Result structure containing value and propagated confidence
typedef struct {
    rtka_ternary_t value;           // Computed ternary result
    rtka_confidence_t confidence;   // Propagated confidence
    size_t operations_performed;    // Count of operations (for analysis)
    size_t unknown_levels;          // Count of UNKNOWN values encountered
    bool early_terminated;          // Whether early termination occurred
    bool unknown_limit_reached;     // Whether 80 UNKNOWN limit was reached
    rtka_confidence_t base_confidence; // Dynamic base confidence calculation
} rtka_result_t;

// State transition system structure
typedef struct {
    rtka_ternary_t state;           // Current state
    rtka_confidence_t confidence;   // Current confidence
    uint64_t iteration;             // Iteration counter
    bool is_absorbing_state;        // Whether in quasi-absorbing UNKNOWN
} rtka_state_t;

// Performance guarantees structure (following OPT-106 pattern)
typedef struct {
    size_t max_operations;          // Guaranteed maximum operations
    size_t memory_accesses;         // Guaranteed memory access pattern
    bool deterministic_runtime;     // Whether runtime is deterministic
    double worst_case_complexity;   // Worst-case time complexity factor
} rtka_performance_t;

// System constants
#define RTKA_MAX_UNKNOWN_LEVELS 80   // Maximum UNKNOWN levels before forcing FALSE

// Core ternary operations (arithmetic-based for efficiency)
RTKA_U_INLINE rtka_ternary_t rtka_not(rtka_ternary_t a) {
    return (rtka_ternary_t)(-a);
}

RTKA_U_INLINE rtka_ternary_t rtka_and(rtka_ternary_t a, rtka_ternary_t b) {
    return (rtka_ternary_t)((a < b) ? a : b);  // min(a, b)
}

RTKA_U_INLINE rtka_ternary_t rtka_or(rtka_ternary_t a, rtka_ternary_t b) {
    return (rtka_ternary_t)((a > b) ? a : b);  // max(a, b)
}

RTKA_U_INLINE rtka_ternary_t rtka_eqv(rtka_ternary_t a, rtka_ternary_t b) {
    return (rtka_ternary_t)(a * b);  // a × b
}

// Vector management functions
rtka_vector_t* rtka_vector_create(size_t initial_capacity);
void rtka_vector_destroy(rtka_vector_t *vector);
bool rtka_vector_push(rtka_vector_t *vector, rtka_ternary_t value, rtka_confidence_t confidence);
bool rtka_vector_reserve(rtka_vector_t *vector, size_t capacity);

// Core recursive algorithm implementation
rtka_result_t rtka_recursive_eval(rtka_operation_t op, const rtka_vector_t *vector);

// Optimized variants following project patterns
rtka_result_t rtka_recursive_eval_simd(rtka_operation_t op, const rtka_vector_t *vector);
rtka_result_t rtka_recursive_eval_predictable(rtka_operation_t op, const rtka_vector_t *vector, 
                                            rtka_performance_t *guarantees);

// Confidence propagation functions
rtka_confidence_t rtka_confidence_and(const rtka_confidence_t *confidences, size_t length);
rtka_confidence_t rtka_confidence_or(const rtka_confidence_t *confidences, size_t length);
rtka_confidence_t rtka_confidence_not(rtka_confidence_t confidence);
rtka_confidence_t rtka_confidence_eqv(rtka_confidence_t a, rtka_confidence_t b);

// State transition system
rtka_state_t rtka_state_create(rtka_ternary_t initial_value, rtka_confidence_t initial_confidence);
rtka_state_t rtka_state_transition(rtka_state_t current, rtka_ternary_t input, 
                                 rtka_confidence_t input_confidence);
bool rtka_state_is_quasi_absorbing(const rtka_state_t *state, rtka_confidence_t threshold);

// UNKNOWN preservation verification
bool rtka_preserves_unknown(rtka_operation_t op, const rtka_vector_t *vector);

// Utility functions for mathematical properties
double rtka_unknown_probability(size_t n);  // (2/3)^n approximation
bool rtka_is_valid_ternary(int8_t value);
const char* rtka_ternary_to_string(rtka_ternary_t value);
const char* rtka_operation_to_string(rtka_operation_t op);

// Error handling
typedef enum {
    RTKA_SUCCESS = 0,
    RTKA_ERROR_NULL_POINTER = 1,
    RTKA_ERROR_INVALID_OPERATION = 2,
    RTKA_ERROR_MEMORY_ALLOCATION = 3,
    RTKA_ERROR_INVALID_CONFIDENCE = 4,
    RTKA_ERROR_EMPTY_VECTOR = 5
} rtka_error_t;

const char* rtka_error_to_string(rtka_error_t error);

#endif // RTKA_U_H
