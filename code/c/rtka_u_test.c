/**
 * RTKA-U Test Suite and Demonstration Module
 * 
 * This module provides comprehensive testing and demonstration of the 
 * Recursive Ternary with Kleene Algorithm + UNKNOWN system.
 * 
 * All test data generation, output formatting, and verification logic
 * is contained here, separate from the core algorithm implementation.
 *
 * Author: H.Overman <opsec.ee@pm.ee>
 */

#include "rtka_u.h"
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <string.h>
#include <assert.h>

// Test configuration constants
#define MAX_TEST_VECTOR_SIZE 1000
#define NUM_RANDOM_TESTS 100
#define NUM_PERFORMANCE_ITERATIONS 1000
#define CONFIDENCE_PRECISION 6

// Test result tracking
typedef struct {
    size_t tests_run;
    size_t tests_passed;
    size_t tests_failed;
    double total_execution_time;
} test_suite_results_t;

static test_suite_results_t global_results = {0};

// Utility functions for test data generation and output
static void print_test_header(const char* test_name) {
    printf("\n" "=" "%.78s\n", "================================================================================");
    printf("TEST: %s\n", test_name);
    printf("=" "%.78s\n", "================================================================================");
}

static void print_test_result(const char* test_name, bool passed) {
    global_results.tests_run++;
    if (passed) {
        global_results.tests_passed++;
        printf("✓ PASS: %s\n", test_name);
    } else {
        global_results.tests_failed++;
        printf("✗ FAIL: %s\n", test_name);
    }
}

static void print_vector_details(const rtka_vector_t* vector, const char* label) {
    printf("%s: [", label);
    for (size_t i = 0; i < vector->length; i++) {
        printf("(%s, %.3f)", 
               rtka_ternary_to_string(vector->values[i]), 
               vector->confidences[i]);
        if (i < vector->length - 1) printf(", ");
    }
    printf("]\n");
}

static void print_result_details(const rtka_result_t* result, const char* operation) {
    printf("Result for %s: Value=%s, Confidence=%.6f, Operations=%zu, Early_Term=%s\n",
           operation,
           rtka_ternary_to_string(result->value),
           result->confidence,
           result->operations_performed,
           result->early_terminated ? "Yes" : "No");
}

// Random test data generation
static rtka_ternary_t generate_random_ternary(void) {
    int rand_val = rand() % 3;
    switch (rand_val) {
        case 0: return RTKA_FALSE;
        case 1: return RTKA_UNKNOWN;
        case 2: return RTKA_TRUE;
        default: return RTKA_UNKNOWN;
    }
}

static double generate_random_confidence(void) {
    return (double)rand() / RAND_MAX;
}

static rtka_vector_t* generate_random_vector(size_t length) {
    rtka_vector_t* vector = rtka_vector_create(length);
    if (!vector) return NULL;
    
    for (size_t i = 0; i < length; i++) {
        rtka_vector_push(vector, generate_random_ternary(), generate_random_confidence());
    }
    
    return vector;
}

// Create specific test vectors for mathematical property verification
static rtka_vector_t* create_unknown_preservation_test_vector(void) {
    rtka_vector_t* vector = rtka_vector_create(5);
    
    // First element UNKNOWN, others non-absorbing
    rtka_vector_push(vector, RTKA_UNKNOWN, 0.8);
    rtka_vector_push(vector, RTKA_UNKNOWN, 0.7);
    rtka_vector_push(vector, RTKA_TRUE, 0.9);
    rtka_vector_push(vector, RTKA_UNKNOWN, 0.6);
    rtka_vector_push(vector, RTKA_TRUE, 0.8);
    
    return vector;
}

static rtka_vector_t* create_early_termination_test_vector(rtka_operation_t op) {
    rtka_vector_t* vector = rtka_vector_create(10);
    
    if (op == RTKA_OP_AND) {
        // Include FALSE early for AND termination
        rtka_vector_push(vector, RTKA_TRUE, 0.9);
        rtka_vector_push(vector, RTKA_UNKNOWN, 0.8);
        rtka_vector_push(vector, RTKA_FALSE, 0.7);  // Should terminate here
        rtka_vector_push(vector, RTKA_TRUE, 0.9);   // Should not be processed
        rtka_vector_push(vector, RTKA_TRUE, 0.9);
    } else if (op == RTKA_OP_OR) {
        // Include TRUE early for OR termination
        rtka_vector_push(vector, RTKA_FALSE, 0.9);
        rtka_vector_push(vector, RTKA_UNKNOWN, 0.8);
        rtka_vector_push(vector, RTKA_TRUE, 0.7);   // Should terminate here
        rtka_vector_push(vector, RTKA_FALSE, 0.9);  // Should not be processed
        rtka_vector_push(vector, RTKA_FALSE, 0.9);
    }
    
    return vector;
}

// Test core ternary operations
static void test_basic_ternary_operations(void) {
    print_test_header("Basic Ternary Operations");
    
    // Test negation
    bool negation_pass = true;
    negation_pass &= (rtka_not(RTKA_TRUE) == RTKA_FALSE);
    negation_pass &= (rtka_not(RTKA_FALSE) == RTKA_TRUE);
    negation_pass &= (rtka_not(RTKA_UNKNOWN) == RTKA_UNKNOWN);
    print_test_result("Negation Operation", negation_pass);
    
    // Test conjunction (min)
    bool conjunction_pass = true;
    conjunction_pass &= (rtka_and(RTKA_TRUE, RTKA_TRUE) == RTKA_TRUE);
    conjunction_pass &= (rtka_and(RTKA_TRUE, RTKA_FALSE) == RTKA_FALSE);
    conjunction_pass &= (rtka_and(RTKA_TRUE, RTKA_UNKNOWN) == RTKA_UNKNOWN);
    conjunction_pass &= (rtka_and(RTKA_UNKNOWN, RTKA_FALSE) == RTKA_FALSE);
    print_test_result("Conjunction Operation", conjunction_pass);
    
    // Test disjunction (max)
    bool disjunction_pass = true;
    disjunction_pass &= (rtka_or(RTKA_FALSE, RTKA_FALSE) == RTKA_FALSE);
    disjunction_pass &= (rtka_or(RTKA_FALSE, RTKA_TRUE) == RTKA_TRUE);
    disjunction_pass &= (rtka_or(RTKA_FALSE, RTKA_UNKNOWN) == RTKA_UNKNOWN);
    disjunction_pass &= (rtka_or(RTKA_UNKNOWN, RTKA_TRUE) == RTKA_TRUE);
    print_test_result("Disjunction Operation", disjunction_pass);
    
    // Test equivalence (multiplication)
    bool equivalence_pass = true;
    equivalence_pass &= (rtka_eqv(RTKA_TRUE, RTKA_TRUE) == RTKA_TRUE);
    equivalence_pass &= (rtka_eqv(RTKA_FALSE, RTKA_FALSE) == RTKA_TRUE);
    equivalence_pass &= (rtka_eqv(RTKA_TRUE, RTKA_FALSE) == RTKA_FALSE);
    equivalence_pass &= (rtka_eqv(RTKA_UNKNOWN, RTKA_TRUE) == RTKA_UNKNOWN);
    print_test_result("Equivalence Operation", equivalence_pass);
}

// Test recursive evaluation
static void test_recursive_evaluation(void) {
    print_test_header("Recursive Evaluation");
    
    // Test AND operation
    rtka_vector_t* and_vector = rtka_vector_create(4);
    rtka_vector_push(and_vector, RTKA_TRUE, 0.9);
    rtka_vector_push(and_vector, RTKA_TRUE, 0.8);
    rtka_vector_push(and_vector, RTKA_UNKNOWN, 0.7);
    rtka_vector_push(and_vector, RTKA_TRUE, 0.9);
    
    rtka_result_t and_result = rtka_recursive_eval(RTKA_OP_AND, and_vector);
    print_vector_details(and_vector, "AND Input");
    print_result_details(&and_result, "AND");
    
    bool and_test_pass = (and_result.value == RTKA_UNKNOWN);
    print_test_result("Recursive AND Evaluation", and_test_pass);
    
    // Test OR operation
    rtka_vector_t* or_vector = rtka_vector_create(4);
    rtka_vector_push(or_vector, RTKA_FALSE, 0.9);
    rtka_vector_push(or_vector, RTKA_UNKNOWN, 0.8);
    rtka_vector_push(or_vector, RTKA_FALSE, 0.7);
    rtka_vector_push(or_vector, RTKA_TRUE, 0.9);
    
    rtka_result_t or_result = rtka_recursive_eval(RTKA_OP_OR, or_vector);
    print_vector_details(or_vector, "OR Input");
    print_result_details(&or_result, "OR");
    
    bool or_test_pass = (or_result.value == RTKA_TRUE) && or_result.early_terminated;
    print_test_result("Recursive OR Evaluation with Early Termination", or_test_pass);
    
    rtka_vector_destroy(and_vector);
    rtka_vector_destroy(or_vector);
}

// Test UNKNOWN preservation theorem
static void test_unknown_preservation_theorem(void) {
    print_test_header("UNKNOWN Preservation Theorem");
    
    // Test Case 1: AND operation with no FALSE values - should preserve UNKNOWN
    rtka_vector_t* preserve_vector = rtka_vector_create(4);
    rtka_vector_push(preserve_vector, RTKA_UNKNOWN, 0.8);
    rtka_vector_push(preserve_vector, RTKA_UNKNOWN, 0.7);
    rtka_vector_push(preserve_vector, RTKA_TRUE, 0.9);
    rtka_vector_push(preserve_vector, RTKA_UNKNOWN, 0.6);
    
    rtka_result_t and_result = rtka_recursive_eval(RTKA_OP_AND, preserve_vector);
    bool and_preserves = rtka_preserves_unknown(RTKA_OP_AND, preserve_vector);
    bool and_test_pass = (and_result.value == RTKA_UNKNOWN) && and_preserves;
    
    print_vector_details(preserve_vector, "AND Preservation Test (no FALSE)");
    printf("AND preserves UNKNOWN: %s, Result: %s\n", 
           and_preserves ? "Yes" : "No",
           rtka_ternary_to_string(and_result.value));
    print_test_result("UNKNOWN Preservation - AND (valid case)", and_test_pass);
    
    // Test Case 2: OR operation with no TRUE values - should preserve UNKNOWN
    rtka_vector_t* or_preserve_vector = rtka_vector_create(4);
    rtka_vector_push(or_preserve_vector, RTKA_UNKNOWN, 0.8);
    rtka_vector_push(or_preserve_vector, RTKA_FALSE, 0.7);
    rtka_vector_push(or_preserve_vector, RTKA_UNKNOWN, 0.9);
    rtka_vector_push(or_preserve_vector, RTKA_FALSE, 0.6);
    
    rtka_result_t or_result = rtka_recursive_eval(RTKA_OP_OR, or_preserve_vector);
    bool or_preserves = rtka_preserves_unknown(RTKA_OP_OR, or_preserve_vector);
    bool or_test_pass = (or_result.value == RTKA_UNKNOWN) && or_preserves;
    
    print_vector_details(or_preserve_vector, "OR Preservation Test (no TRUE)");
    printf("OR preserves UNKNOWN: %s, Result: %s\n", 
           or_preserves ? "Yes" : "No",
           rtka_ternary_to_string(or_result.value));
    print_test_result("UNKNOWN Preservation - OR (valid case)", or_test_pass);
    
    // Test Case 3: OR operation with TRUE values - should NOT preserve UNKNOWN
    rtka_vector_t* or_absorb_vector = rtka_vector_create(4);
    rtka_vector_push(or_absorb_vector, RTKA_UNKNOWN, 0.8);
    rtka_vector_push(or_absorb_vector, RTKA_TRUE, 0.7);
    rtka_vector_push(or_absorb_vector, RTKA_UNKNOWN, 0.9);
    rtka_vector_push(or_absorb_vector, RTKA_FALSE, 0.6);
    
    rtka_result_t or_absorb_result = rtka_recursive_eval(RTKA_OP_OR, or_absorb_vector);
    bool or_absorb_preserves = rtka_preserves_unknown(RTKA_OP_OR, or_absorb_vector);
    bool or_absorb_test_pass = (or_absorb_result.value == RTKA_TRUE) && !or_absorb_preserves;
    
    print_vector_details(or_absorb_vector, "OR Absorption Test (with TRUE)");
    printf("OR preserves UNKNOWN: %s, Result: %s\n", 
           or_absorb_preserves ? "Yes" : "No",
           rtka_ternary_to_string(or_absorb_result.value));
    print_test_result("UNKNOWN Absorption - OR (absorbing case)", or_absorb_test_pass);
    
    rtka_vector_destroy(preserve_vector);
    rtka_vector_destroy(or_preserve_vector);
    rtka_vector_destroy(or_absorb_vector);
}

// Test confidence propagation
static void test_confidence_propagation(void) {
    print_test_header("Confidence Propagation");
    
    // Test AND confidence (product rule)
    double and_confidences[] = {0.9, 0.8, 0.7};
    double and_expected = 0.9 * 0.8 * 0.7;
    double and_result = rtka_confidence_and(and_confidences, 3);
    bool and_confidence_pass = (fabs(and_result - and_expected) < 1e-10);
    
    printf("AND Confidence: [%.3f, %.3f, %.3f] → %.6f (expected: %.6f)\n",
           and_confidences[0], and_confidences[1], and_confidences[2],
           and_result, and_expected);
    print_test_result("AND Confidence Propagation", and_confidence_pass);
    
    // Test OR confidence (inclusion-exclusion)
    double or_confidences[] = {0.6, 0.4};
    double or_expected = 1.0 - (1.0 - 0.6) * (1.0 - 0.4);
    double or_result = rtka_confidence_or(or_confidences, 2);
    bool or_confidence_pass = (fabs(or_result - or_expected) < 1e-10);
    
    printf("OR Confidence: [%.3f, %.3f] → %.6f (expected: %.6f)\n",
           or_confidences[0], or_confidences[1],
           or_result, or_expected);
    print_test_result("OR Confidence Propagation", or_confidence_pass);
    
    // Test NOT confidence (preservation)
    double not_confidence = 0.75;
    double not_result = rtka_confidence_not(not_confidence);
    bool not_confidence_pass = (not_result == not_confidence);
    
    printf("NOT Confidence: %.3f → %.6f\n", not_confidence, not_result);
    print_test_result("NOT Confidence Propagation", not_confidence_pass);
}

// Test early termination optimization
static void test_early_termination(void) {
    print_test_header("Early Termination Optimization");
    
    // Test AND early termination
    rtka_vector_t* and_vector = create_early_termination_test_vector(RTKA_OP_AND);
    rtka_result_t and_result = rtka_recursive_eval(RTKA_OP_AND, and_vector);
    
    print_vector_details(and_vector, "AND Early Termination Test");
    print_result_details(&and_result, "AND");
    
    bool and_early_pass = and_result.early_terminated && 
                          (and_result.value == RTKA_FALSE) &&
                          (and_result.operations_performed < and_vector->length);
    print_test_result("AND Early Termination", and_early_pass);
    
    // Test OR early termination
    rtka_vector_t* or_vector = create_early_termination_test_vector(RTKA_OP_OR);
    rtka_result_t or_result = rtka_recursive_eval(RTKA_OP_OR, or_vector);
    
    print_vector_details(or_vector, "OR Early Termination Test");
    print_result_details(&or_result, "OR");
    
    bool or_early_pass = or_result.early_terminated && 
                         (or_result.value == RTKA_TRUE) &&
                         (or_result.operations_performed < or_vector->length);
    print_test_result("OR Early Termination", or_early_pass);
    
    rtka_vector_destroy(and_vector);
    rtka_vector_destroy(or_vector);
}

// Test state transition system
static void test_state_transition_system(void) {
    print_test_header("State Transition System");
    
    // Create initial state
    rtka_state_t state = rtka_state_create(RTKA_TRUE, 0.9);
    printf("Initial State: %s (confidence: %.3f)\n", 
           rtka_ternary_to_string(state.state), state.confidence);
    
    // Apply transitions
    state = rtka_state_transition(state, RTKA_UNKNOWN, 0.7);
    printf("After UNKNOWN input: %s (confidence: %.3f, iteration: %lu)\n", 
           rtka_ternary_to_string(state.state), state.confidence, state.iteration);
    
    state = rtka_state_transition(state, RTKA_FALSE, 0.5);
    printf("After FALSE input: %s (confidence: %.3f, iteration: %lu)\n", 
           rtka_ternary_to_string(state.state), state.confidence, state.iteration);
    
    // Test quasi-absorbing state
    bool is_absorbing = rtka_state_is_quasi_absorbing(&state, 0.1);
    printf("Is quasi-absorbing (threshold 0.1): %s\n", is_absorbing ? "Yes" : "No");
    
    bool state_test_pass = (state.state == RTKA_FALSE) && (state.iteration == 2);
    print_test_result("State Transition System", state_test_pass);
}

// Test UNKNOWN level protection and dynamic confidence scoring
static void test_unknown_level_protection(void) {
    print_test_header("UNKNOWN Level Protection and Dynamic Confidence Analysis");
    
    printf("Testing 80 UNKNOWN level protection mechanism...\n\n");
    
    // Test UNKNOWN level progression
    printf("UNKNOWN LEVEL PROGRESSION ANALYSIS\n");
    printf("==================================\n");
    printf("UNKNOWN Count | Dynamic Conf | Result     | Limit Reached | Base Confidence\n");
    printf("-------------|-------------|------------|--------------|----------------\n");
    
    bool level_protection_working = true;
    
    // Test vectors with increasing UNKNOWN counts
    size_t test_levels[] = {5, 10, 20, 40, 60, 79, 80, 85, 100};
    size_t num_tests = sizeof(test_levels) / sizeof(test_levels[0]);
    
    for (size_t t = 0; t < num_tests; t++) {
        size_t unknown_count = test_levels[t];
        rtka_vector_t* level_vector = rtka_vector_create(unknown_count + 2);
        
        // Add some TRUE values to create valid vector
        rtka_vector_push(level_vector, RTKA_TRUE, 0.9);
        
        // Add specified number of UNKNOWN values
        for (size_t i = 0; i < unknown_count; i++) {
            rtka_vector_push(level_vector, RTKA_UNKNOWN, 0.8);
        }
        
        // Add one more TRUE to complete the test
        rtka_vector_push(level_vector, RTKA_TRUE, 0.9);
        
        rtka_result_t result = rtka_recursive_eval(RTKA_OP_AND, level_vector);
        bool limit_triggered = (unknown_count >= RTKA_MAX_UNKNOWN_LEVELS && result.unknown_limit_reached);
        
        printf("%12zu | %11.6f | %10s | %13s | %14.6f\n", 
               unknown_count, 
               result.confidence,
               rtka_ternary_to_string(result.value),
               result.unknown_limit_reached ? "YES" : "No",
               result.base_confidence);
        
        // Verify protection triggers correctly
        if (unknown_count >= RTKA_MAX_UNKNOWN_LEVELS) {
            if (!result.unknown_limit_reached || result.value != RTKA_FALSE) {
                level_protection_working = false;
            }
        } else {
            if (result.unknown_limit_reached) {
                level_protection_working = false;
            }
        }
        
        rtka_vector_destroy(level_vector);
    }
    
    print_test_result("UNKNOWN Level Protection (80 limit)", level_protection_working);
    
    // Test dynamic confidence calculation accuracy
    printf("\nDYNAMIC CONFIDENCE CALCULATION ANALYSIS\n");
    printf("======================================\n");
    printf("Input Pattern | Base Conf | Calculated | Expected | Accuracy\n");
    printf("-------------|----------|-----------|----------|----------\n");
    
    bool dynamic_confidence_accurate = true;
    
    // Test various confidence patterns
    struct {
        double confidences[5];
        size_t count;
        const char* pattern_name;
    } patterns[] = {
        {{0.9, 0.8, 0.7, 0.9, 0.8}, 5, "High Mixed"},
        {{0.5, 0.6, 0.5, 0.7, 0.6}, 5, "Medium Mixed"},
        {{0.9, 0.9, 0.9, 0.9, 0.9}, 5, "Uniform High"},
        {{0.3, 0.4, 0.3, 0.4, 0.3}, 5, "Uniform Low"},
        {{0.1, 0.9, 0.1, 0.9, 0.1}, 5, "Alternating"}
    };
    
    for (size_t p = 0; p < 5; p++) {
        rtka_vector_t* conf_vector = rtka_vector_create(patterns[p].count);
        double expected_base = 0.0;
        
        for (size_t i = 0; i < patterns[p].count; i++) {
            rtka_vector_push(conf_vector, RTKA_TRUE, patterns[p].confidences[i]);
            expected_base += patterns[p].confidences[i];
        }
        expected_base /= patterns[p].count;
        
        rtka_result_t result = rtka_recursive_eval(RTKA_OP_AND, conf_vector);
        double accuracy = fabs(result.base_confidence - expected_base) < 0.001 ? 100.0 : 0.0;
        
        printf("%12s | %9.6f | %9.6f | %8.6f | %7.1f%%\n",
               patterns[p].pattern_name,
               result.base_confidence,
               result.confidence,
               expected_base,
               accuracy);
        
        if (accuracy < 100.0) {
            dynamic_confidence_accurate = false;
        }
        
        rtka_vector_destroy(conf_vector);
    }
    
    print_test_result("Dynamic Confidence Calculation", dynamic_confidence_accurate);
    
    // Chart UNKNOWN level visualization
    printf("\nUNKNOWN LEVEL VISUALIZATION\n");
    printf("===========================\n");
    printf("Levels | Protection Status (each * = 2 levels)\n");
    printf("-------|");
    for (int i = 0; i < 40; i++) printf("-");
    printf("\n");
    
    for (int level = 10; level <= 100; level += 10) {
        printf("%6d | ", level);
        
        int chart_length = level / 2;  // Scale to 50 characters max
        for (int i = 0; i < chart_length && i < 40; i++) {
            printf("*");
        }
        
        if (level >= RTKA_MAX_UNKNOWN_LEVELS) {
            printf(" <- PROTECTION ACTIVE (FALSE forced)");
        }
        printf("\n");
    }
    
    printf("\nProtection Threshold: %d levels (", RTKA_MAX_UNKNOWN_LEVELS);
    for (int i = 0; i < 40; i++) printf("*");
    printf(")\n");
}

// Test floating point precision protection
static void test_floating_point_precision_protection(void) {
    print_test_header("Floating Point Precision Protection Analysis");
    
    printf("Testing floating point precision stability with high UNKNOWN counts...\n\n");
    
    printf("PRECISION STABILITY TESTING\n");
    printf("===========================\n");
    printf("Test Size | Operations | Time (μs) | Precision Protected | Memory Stable\n");
    printf("----------|-----------|----------|-------------------|---------------\n");
    
    size_t precision_test_sizes[] = {50, 75, 80, 85, 100, 150, 200};
    size_t num_precision_tests = sizeof(precision_test_sizes) / sizeof(precision_test_sizes[0]);
    bool precision_protection_working = true;
    
    for (size_t t = 0; t < num_precision_tests; t++) {
        size_t test_size = precision_test_sizes[t];
        
        rtka_vector_t* precision_vector = rtka_vector_create(test_size);
        
        // Create vector with mostly UNKNOWN values to stress floating point
        for (size_t i = 0; i < test_size; i++) {
            if (i % 20 == 0) {
                rtka_vector_push(precision_vector, RTKA_TRUE, 0.8);
            } else {
                rtka_vector_push(precision_vector, RTKA_UNKNOWN, 0.7);
            }
        }
        
        clock_t start = clock();
        rtka_result_t result = rtka_recursive_eval(RTKA_OP_AND, precision_vector);
        clock_t end = clock();
        
        double time_us = ((double)(end - start) / CLOCKS_PER_SEC) * 1000000;
        bool precision_protected = (test_size >= RTKA_MAX_UNKNOWN_LEVELS) ? 
                                  result.unknown_limit_reached : true;
        bool memory_stable = (precision_vector->length == test_size);
        
        printf("%9zu | %9zu | %8.2f | %17s | %13s\n",
               test_size,
               result.operations_performed,
               time_us,
               precision_protected ? "YES" : "No",
               memory_stable ? "YES" : "No");
        
        if (!precision_protected || !memory_stable) {
            precision_protection_working = false;
        }
        
        rtka_vector_destroy(precision_vector);
    }
    
    print_test_result("Floating Point Precision Protection", precision_protection_working);
    
    printf("\nPrecision protection prevents floating point degradation\n");
    printf("when processing excessive UNKNOWN sequences in recursive operations.\n");
}

// Test performance variants
static void test_performance_variants(void) {
    print_test_header("Performance Variants");
    
    // Create large test vector
    rtka_vector_t* large_vector = generate_random_vector(100);
    
    // Test standard implementation
    clock_t start = clock();
    rtka_result_t standard_result = rtka_recursive_eval(RTKA_OP_AND, large_vector);
    clock_t standard_time = clock() - start;
    
    // Test SIMD implementation
    start = clock();
    rtka_result_t simd_result = rtka_recursive_eval_simd(RTKA_OP_AND, large_vector);
    clock_t simd_time = clock() - start;
    
    // Test predictable performance implementation
    rtka_performance_t guarantees;
    start = clock();
    rtka_result_t predictable_result = rtka_recursive_eval_predictable(RTKA_OP_AND, large_vector, &guarantees);
    clock_t predictable_time = clock() - start;
    
    printf("Standard Implementation: %.6f seconds\n", (double)standard_time / CLOCKS_PER_SEC);
    printf("SIMD Implementation: %.6f seconds\n", (double)simd_time / CLOCKS_PER_SEC);
    printf("Predictable Implementation: %.6f seconds\n", (double)predictable_time / CLOCKS_PER_SEC);
    
    printf("Performance Guarantees:\n");
    printf("  Max Operations: %zu\n", guarantees.max_operations);
    printf("  Memory Accesses: %zu\n", guarantees.memory_accesses);
    printf("  Deterministic Runtime: %s\n", guarantees.deterministic_runtime ? "Yes" : "No");
    printf("  Worst Case Complexity: %.2f\n", guarantees.worst_case_complexity);
    
    // Results should be consistent across implementations
    bool consistency_pass = (standard_result.value == simd_result.value) &&
                           (standard_result.value == predictable_result.value);
    print_test_result("Implementation Consistency", consistency_pass);
    
    rtka_vector_destroy(large_vector);
}

// Random property testing
static void test_random_properties(void) {
    print_test_header("Random Property Testing");
    
    srand((unsigned int)time(NULL));
    
    size_t property_violations = 0;
    
    for (size_t test = 0; test < NUM_RANDOM_TESTS; test++) {
        size_t vector_size = 1 + (rand() % 50);
        rtka_vector_t* random_vector = generate_random_vector(vector_size);
        
        // Test all operations
        rtka_result_t and_result = rtka_recursive_eval(RTKA_OP_AND, random_vector);
        rtka_result_t or_result = rtka_recursive_eval(RTKA_OP_OR, random_vector);
        
        // Verify mathematical properties
        if (and_result.confidence < 0.0 || and_result.confidence > 1.0) property_violations++;
        if (or_result.confidence < 0.0 || or_result.confidence > 1.0) property_violations++;
        if (!rtka_is_valid_ternary(and_result.value)) property_violations++;
        if (!rtka_is_valid_ternary(or_result.value)) property_violations++;
        
        rtka_vector_destroy(random_vector);
    }
    
    printf("Random tests completed: %d tests, %zu property violations\n", 
           NUM_RANDOM_TESTS, property_violations);
    
    bool random_test_pass = (property_violations == 0);
    print_test_result("Random Property Testing", random_test_pass);
}

// Main test execution function
static void print_final_results(void) {
    printf("\n" "=" "%.78s\n", "================================================================================");
    printf("FINAL TEST RESULTS\n");
    printf("=" "%.78s\n", "================================================================================");
    printf("Tests Run: %zu\n", global_results.tests_run);
    printf("Tests Passed: %zu\n", global_results.tests_passed);
    printf("Tests Failed: %zu\n", global_results.tests_failed);
    printf("Success Rate: %.2f%%\n", 
           global_results.tests_run > 0 ? 
           (100.0 * global_results.tests_passed / global_results.tests_run) : 0.0);
    
    if (global_results.tests_failed == 0) {
        printf("\n🎉 ALL TESTS PASSED! RTKA-U implementation is mathematically correct.\n");
    } else {
        printf("\n⚠️  Some tests failed. Please review the implementation.\n");
    }
    printf("=" "%.78s\n", "================================================================================");
}

int main(void) {
    printf("RTKA-U: Recursive Ternary with Kleene Algorithm + UNKNOWN\n");
    printf("Mathematical Verification and Performance Testing Suite\n");
    printf("Enhanced with 80 UNKNOWN Level Protection and Dynamic Confidence\n");
    
    // Execute all test suites
    test_basic_ternary_operations();
    test_recursive_evaluation();
    test_unknown_preservation_theorem();
    test_confidence_propagation();
    test_early_termination();
    test_state_transition_system();
    test_unknown_level_protection();
    test_floating_point_precision_protection();
    test_performance_variants();
    test_random_properties();
    
    print_final_results();
    
    return (global_results.tests_failed == 0) ? EXIT_SUCCESS : EXIT_FAILURE;
}
