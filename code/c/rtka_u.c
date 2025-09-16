/**
 * RTKA-U Algorithm Implementation
 * 
 * Core implementation of Recursive Ternary with Kleene Algorithm + UNKNOWN
 * Copyright (c) 2025 - H.Overman <opsec.ee@pm.me>
 */

#include "rtka_u.h"
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <float.h>
#include <stdio.h>

// SIMD support detection and includes
#ifdef __SSE2__
#include <emmintrin.h>
#endif
#ifdef __AVX2__
#include <immintrin.h>
#endif

// Constants for mathematical properties
static const double RTKA_CONFIDENCE_THRESHOLD = 0.001;
static const double RTKA_TWO_THIRDS = 2.0 / 3.0;

// Vector management implementation following cache-friendly patterns (OPT-072)
rtka_vector_t* rtka_vector_create(size_t initial_capacity) {
    if (initial_capacity == 0) {
        initial_capacity = 16;  // Reasonable default aligned to cache line
    }
    
    rtka_vector_t *vector = malloc(sizeof(rtka_vector_t));
    if (!vector) return NULL;
    
    // Allocate aligned memory for better cache performance
    vector->values = aligned_alloc(64, initial_capacity * sizeof(rtka_ternary_t));
    vector->confidences = aligned_alloc(64, initial_capacity * sizeof(rtka_confidence_t));
    
    if (!vector->values || !vector->confidences) {
        free(vector->values);
        free(vector->confidences);
        free(vector);
        return NULL;
    }
    
    vector->length = 0;
    vector->capacity = initial_capacity;
    return vector;
}

void rtka_vector_destroy(rtka_vector_t *vector) {
    if (vector) {
        free(vector->values);
        free(vector->confidences);
        free(vector);
    }
}

bool rtka_vector_push(rtka_vector_t *vector, rtka_ternary_t value, rtka_confidence_t confidence) {
    if (!vector || !rtka_is_valid_ternary(value) || confidence < 0.0 || confidence > 1.0) {
        return false;
    }
    
    if (vector->length >= vector->capacity) {
        if (!rtka_vector_reserve(vector, vector->capacity * 2)) {
            return false;
        }
    }
    
    vector->values[vector->length] = value;
    vector->confidences[vector->length] = confidence;
    vector->length++;
    return true;
}

bool rtka_vector_reserve(rtka_vector_t *vector, size_t capacity) {
    if (!vector || capacity <= vector->capacity) {
        return capacity <= vector->capacity;
    }
    
    rtka_ternary_t *new_values = aligned_alloc(64, capacity * sizeof(rtka_ternary_t));
    rtka_confidence_t *new_confidences = aligned_alloc(64, capacity * sizeof(rtka_confidence_t));
    
    if (!new_values || !new_confidences) {
        free(new_values);
        free(new_confidences);
        return false;
    }
    
    memcpy(new_values, vector->values, vector->length * sizeof(rtka_ternary_t));
    memcpy(new_confidences, vector->confidences, vector->length * sizeof(rtka_confidence_t));
    
    free(vector->values);
    free(vector->confidences);
    
    vector->values = new_values;
    vector->confidences = new_confidences;
    vector->capacity = capacity;
    return true;
}

// Core recursive evaluation - standard implementation with UNKNOWN level protection
rtka_result_t rtka_recursive_eval(rtka_operation_t op, const rtka_vector_t *vector) {
    rtka_result_t result = {
        .value = RTKA_UNKNOWN,
        .confidence = 0.0,
        .operations_performed = 0,
        .unknown_levels = 0,
        .early_terminated = false,
        .unknown_limit_reached = false,
        .base_confidence = 1.0
    };
    
    if (!vector || vector->length == 0) {
        return result;
    }
    
    // Count UNKNOWN values in input vector
    size_t unknown_count = 0;
    double confidence_sum = 0.0;
    for (size_t i = 0; i < vector->length; i++) {
        if (vector->values[i] == RTKA_UNKNOWN) {
            unknown_count++;
        }
        confidence_sum += vector->confidences[i];
    }
    
    result.unknown_levels = unknown_count;
    
    // Check UNKNOWN level limit - critical floating point protection
    if (unknown_count >= RTKA_MAX_UNKNOWN_LEVELS) {
        result.value = RTKA_FALSE;
        result.unknown_limit_reached = true;
        result.confidence = 0.0;
        printf("UNKNOWN LEVEL PROTECTION: %zu UNKNOWN values >= %d limit, forcing FALSE\n", 
               unknown_count, RTKA_MAX_UNKNOWN_LEVELS);
        return result;
    }
    
    // Dynamic confidence calculation based on input characteristics
    result.base_confidence = confidence_sum / vector->length;  // Average input confidence
    
    // Base case: single element
    if (vector->length == 1) {
        result.value = (op == RTKA_OP_NOT) ? rtka_not(vector->values[0]) : vector->values[0];
        result.confidence = vector->confidences[0];
        result.operations_performed = (op == RTKA_OP_NOT) ? 1 : 0;
        return result;
    }
    
    // Recursive evaluation with early termination optimization
    rtka_ternary_t accumulator = vector->values[0];
    size_t ops = 0;
    size_t processed_elements = 1;
    
    for (size_t i = 1; i < vector->length; i++) {
        ops++;
        processed_elements = i + 1;
        
        switch (op) {
            case RTKA_OP_AND:
                accumulator = rtka_and(accumulator, vector->values[i]);
                // Early termination: FALSE is absorbing for AND
                if (accumulator == RTKA_FALSE) {
                    result.early_terminated = true;
                    break;
                }
                break;
                
            case RTKA_OP_OR:
                accumulator = rtka_or(accumulator, vector->values[i]);
                // Early termination: TRUE is absorbing for OR
                if (accumulator == RTKA_TRUE) {
                    result.early_terminated = true;
                    break;
                }
                break;
                
            case RTKA_OP_EQV:
                accumulator = rtka_eqv(accumulator, vector->values[i]);
                break;
                
            case RTKA_OP_NOT:
                // NOT is unary - only apply to first element
                accumulator = rtka_not(vector->values[0]);
                result.early_terminated = true;
                break;
                
            default:
                result.value = RTKA_UNKNOWN;
                return result;
        }
        
        if (result.early_terminated) break;
    }
    
    // Calculate confidence based on operation type and processed elements
    switch (op) {
        case RTKA_OP_AND:
            result.confidence = rtka_confidence_and(vector->confidences, 
                                                  result.early_terminated ? processed_elements : vector->length);
            break;
        case RTKA_OP_OR:
            result.confidence = rtka_confidence_or(vector->confidences, 
                                                 result.early_terminated ? processed_elements : vector->length);
            break;
        case RTKA_OP_NOT:
            result.confidence = rtka_confidence_not(vector->confidences[0]);
            break;
        case RTKA_OP_EQV:
            result.confidence = rtka_confidence_eqv(vector->confidences[0], vector->confidences[1]);
            for (size_t j = 2; j < vector->length; j++) {
                result.confidence = rtka_confidence_eqv(result.confidence, vector->confidences[j]);
            }
            break;
    }
    
    result.value = accumulator;
    result.operations_performed = ops;
    
    // Apply original confidence threshold for UNKNOWN promotion
    if (result.confidence < RTKA_CONFIDENCE_THRESHOLD) {
        result.value = RTKA_UNKNOWN;
    }
    
    return result;
}

// SIMD-optimized variant (OPT-049: SIMD String Operations pattern adapted)
rtka_result_t rtka_recursive_eval_simd(rtka_operation_t op, const rtka_vector_t *vector) {
    // For small vectors or unsupported operations, fall back to standard implementation
    if (!vector || vector->length < 16 || op == RTKA_OP_NOT || op == RTKA_OP_EQV) {
        return rtka_recursive_eval(op, vector);
    }
    
#ifdef __AVX2__
    // AVX2 implementation for AND/OR operations on ternary values
    if (op == RTKA_OP_AND || op == RTKA_OP_OR) {
        rtka_result_t result = {0};
        
        // Process 32 elements at a time (256-bit / 8-bit per element)
        const size_t simd_chunk = 32;
        size_t simd_end = (vector->length / simd_chunk) * simd_chunk;
        
        // Initialize accumulator with identity element
        __m256i acc = _mm256_set1_epi8((op == RTKA_OP_AND) ? RTKA_TRUE : RTKA_FALSE);
        
        for (size_t i = 0; i < simd_end; i += simd_chunk) {
            __m256i data = _mm256_load_si256((const __m256i*)(vector->values + i));
            
            if (op == RTKA_OP_AND) {
                acc = _mm256_min_epi8(acc, data);
            } else {
                acc = _mm256_max_epi8(acc, data);
            }
        }
        
        // Extract results from SIMD register and find final result
        int8_t results[32];
        _mm256_store_si256((__m256i*)results, acc);
        
        rtka_ternary_t final_result = results[0];
        for (size_t i = 1; i < 32; i++) {
            final_result = (op == RTKA_OP_AND) ? 
                          rtka_and(final_result, (rtka_ternary_t)results[i]) :
                          rtka_or(final_result, (rtka_ternary_t)results[i]);
        }
        
        // Process remaining elements
        for (size_t i = simd_end; i < vector->length; i++) {
            final_result = (op == RTKA_OP_AND) ? 
                          rtka_and(final_result, vector->values[i]) :
                          rtka_or(final_result, vector->values[i]);
        }
        
        result.value = final_result;
        result.confidence = (op == RTKA_OP_AND) ? 
                           rtka_confidence_and(vector->confidences, vector->length) :
                           rtka_confidence_or(vector->confidences, vector->length);
        result.operations_performed = vector->length - 1;
        
        return result;
    }
#endif
    
    // Fall back to standard implementation if SIMD not available
    return rtka_recursive_eval(op, vector);
}

// Predictable performance variant (OPT-106: Predictable Performance)
rtka_result_t rtka_recursive_eval_predictable(rtka_operation_t op, const rtka_vector_t *vector,
                                            rtka_performance_t *guarantees) {
    // Set guaranteed performance characteristics
    if (guarantees) {
        guarantees->max_operations = vector ? vector->length : 0;
        guarantees->memory_accesses = vector ? vector->length * 2 : 0; // values + confidences
        guarantees->deterministic_runtime = true;
        guarantees->worst_case_complexity = vector ? (double)vector->length : 0.0;
    }
    
    // Disable early termination for predictable performance
    rtka_result_t result = {0};
    
    if (!vector || vector->length == 0) {
        return result;
    }
    
    rtka_ternary_t accumulator = vector->values[0];
    
    // Process all elements without early termination
    for (size_t i = 1; i < vector->length; i++) {
        switch (op) {
            case RTKA_OP_AND:
                accumulator = rtka_and(accumulator, vector->values[i]);
                break;
            case RTKA_OP_OR:
                accumulator = rtka_or(accumulator, vector->values[i]);
                break;
            case RTKA_OP_EQV:
                accumulator = rtka_eqv(accumulator, vector->values[i]);
                break;
            case RTKA_OP_NOT:
                accumulator = rtka_not(vector->values[0]);
                break;
        }
    }
    
    result.value = accumulator;
    result.operations_performed = vector->length - 1;
    
    // Calculate confidence
    switch (op) {
        case RTKA_OP_AND:
            result.confidence = rtka_confidence_and(vector->confidences, vector->length);
            break;
        case RTKA_OP_OR:
            result.confidence = rtka_confidence_or(vector->confidences, vector->length);
            break;
        case RTKA_OP_NOT:
            result.confidence = rtka_confidence_not(vector->confidences[0]);
            break;
        case RTKA_OP_EQV:
            result.confidence = vector->confidences[0];
            for (size_t i = 1; i < vector->length; i++) {
                result.confidence = rtka_confidence_eqv(result.confidence, vector->confidences[i]);
            }
            break;
    }
    
    return result;
}

// Confidence propagation implementations following mathematical specifications
rtka_confidence_t rtka_confidence_and(const rtka_confidence_t *confidences, size_t length) {
    if (!confidences || length == 0) return 0.0;
    
    // Product rule: C(∧) = ∏ cᵢ
    double product = 1.0;
    for (size_t i = 0; i < length; i++) {
        product *= confidences[i];
    }
    return product;
}

rtka_confidence_t rtka_confidence_or(const rtka_confidence_t *confidences, size_t length) {
    if (!confidences || length == 0) return 0.0;
    
    // Inclusion-exclusion principle: C(∨) = 1 - ∏(1 - cᵢ)
    double complement_product = 1.0;
    for (size_t i = 0; i < length; i++) {
        complement_product *= (1.0 - confidences[i]);
    }
    return 1.0 - complement_product;
}

rtka_confidence_t rtka_confidence_not(rtka_confidence_t confidence) {
    // Negation preserves confidence
    return confidence;
}

rtka_confidence_t rtka_confidence_eqv(rtka_confidence_t a, rtka_confidence_t b) {
    // Equivalence confidence as geometric mean
    return sqrt(a * b);
}

// State transition system implementation
rtka_state_t rtka_state_create(rtka_ternary_t initial_value, rtka_confidence_t initial_confidence) {
    return (rtka_state_t) {
        .state = initial_value,
        .confidence = initial_confidence,
        .iteration = 0,
        .is_absorbing_state = (initial_value == RTKA_UNKNOWN && 
                             initial_confidence < RTKA_CONFIDENCE_THRESHOLD)
    };
}

rtka_state_t rtka_state_transition(rtka_state_t current, rtka_ternary_t input, 
                                 rtka_confidence_t input_confidence) {
    rtka_state_t next = current;
    next.iteration++;
    
    // Apply state transition: S(t+1) = Φ(S(t), I(t))
    // Simple model: new_state = AND(current_state, input)
    next.state = rtka_and(current.state, input);
    
    // Calculate combined confidence
    rtka_confidence_t confidences[2] = {current.confidence, input_confidence};
    next.confidence = rtka_confidence_and(confidences, 2);
    
    // Update absorbing state status
    next.is_absorbing_state = rtka_state_is_quasi_absorbing(&next, RTKA_CONFIDENCE_THRESHOLD);
    
    return next;
}

bool rtka_state_is_quasi_absorbing(const rtka_state_t *state, rtka_confidence_t threshold) {
    return state && state->state == RTKA_UNKNOWN && state->confidence < threshold;
}

// UNKNOWN preservation verification
bool rtka_preserves_unknown(rtka_operation_t op, const rtka_vector_t *vector) {
    if (!vector || vector->length == 0) return false;
    
    // Check if first element is UNKNOWN
    if (vector->values[0] != RTKA_UNKNOWN) return false;
    
    // Apply UNKNOWN Preservation Theorem
    for (size_t i = 1; i < vector->length; i++) {
        if ((op == RTKA_OP_AND && vector->values[i] == RTKA_FALSE) ||
            (op == RTKA_OP_OR && vector->values[i] == RTKA_TRUE)) {
            return false;  // UNKNOWN will not be preserved
        }
    }
    
    return true;
}

// Mathematical utility functions
double rtka_unknown_probability(size_t n) {
    // Approximation: P(UNKNOWN|n) ≈ (2/3)^n
    return pow(RTKA_TWO_THIRDS, (double)n);
}

bool rtka_is_valid_ternary(int8_t value) {
    return value == RTKA_FALSE || value == RTKA_UNKNOWN || value == RTKA_TRUE;
}

const char* rtka_ternary_to_string(rtka_ternary_t value) {
    switch (value) {
        case RTKA_FALSE: return "FALSE";
        case RTKA_UNKNOWN: return "UNKNOWN";
        case RTKA_TRUE: return "TRUE";
        default: return "INVALID";
    }
}

const char* rtka_operation_to_string(rtka_operation_t op) {
    switch (op) {
        case RTKA_OP_AND: return "AND";
        case RTKA_OP_OR: return "OR";
        case RTKA_OP_NOT: return "NOT";
        case RTKA_OP_EQV: return "EQV";
        default: return "INVALID";
    }
}

const char* rtka_error_to_string(rtka_error_t error) {
    switch (error) {
        case RTKA_SUCCESS: return "Success";
        case RTKA_ERROR_NULL_POINTER: return "Null pointer error";
        case RTKA_ERROR_INVALID_OPERATION: return "Invalid operation";
        case RTKA_ERROR_MEMORY_ALLOCATION: return "Memory allocation error";
        case RTKA_ERROR_INVALID_CONFIDENCE: return "Invalid confidence value";
        case RTKA_ERROR_EMPTY_VECTOR: return "Empty vector error";
        default: return "Unknown error";
    }
}
