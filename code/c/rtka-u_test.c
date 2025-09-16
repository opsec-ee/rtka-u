/**
 * rtka-u_test.c
 * Test suite for RTKA-U
 */

#include <stdio.h>
#include <assert.h>
#include <math.h>
#include <stdlib.h>
#include <time.h>
#include "rtka-u.h"
#include "rtka-u_test.h"

/* Test basic Kleene operations */
void test_basic_operations(void) {
    printf("\n=== Testing Basic Operations ===\n");
    
    /* Test AND */
    printf("\nTesting AND (min operation):\n");
    printf("  TRUE ∧ TRUE = %s\n", rtka_to_string(rtka_and(RTKA_TRUE, RTKA_TRUE)));
    printf("  TRUE ∧ FALSE = %s\n", rtka_to_string(rtka_and(RTKA_TRUE, RTKA_FALSE)));
    printf("  TRUE ∧ UNKNOWN = %s\n", rtka_to_string(rtka_and(RTKA_TRUE, RTKA_UNKNOWN)));
    printf("  FALSE ∧ UNKNOWN = %s\n", rtka_to_string(rtka_and(RTKA_FALSE, RTKA_UNKNOWN)));
    assert(rtka_and(RTKA_TRUE, RTKA_TRUE) == RTKA_TRUE);
    assert(rtka_and(RTKA_TRUE, RTKA_FALSE) == RTKA_FALSE);
    assert(rtka_and(RTKA_TRUE, RTKA_UNKNOWN) == RTKA_UNKNOWN);
    assert(rtka_and(RTKA_FALSE, RTKA_UNKNOWN) == RTKA_FALSE);
    printf("  ✓ AND operations correct\n");
    
    /* Test OR */
    printf("\nTesting OR (max operation):\n");
    printf("  FALSE ∨ FALSE = %s\n", rtka_to_string(rtka_or(RTKA_FALSE, RTKA_FALSE)));
    printf("  TRUE ∨ FALSE = %s\n", rtka_to_string(rtka_or(RTKA_TRUE, RTKA_FALSE)));
    printf("  FALSE ∨ UNKNOWN = %s\n", rtka_to_string(rtka_or(RTKA_FALSE, RTKA_UNKNOWN)));
    printf("  TRUE ∨ UNKNOWN = %s\n", rtka_to_string(rtka_or(RTKA_TRUE, RTKA_UNKNOWN)));
    assert(rtka_or(RTKA_FALSE, RTKA_FALSE) == RTKA_FALSE);
    assert(rtka_or(RTKA_TRUE, RTKA_FALSE) == RTKA_TRUE);
    assert(rtka_or(RTKA_FALSE, RTKA_UNKNOWN) == RTKA_UNKNOWN);
    assert(rtka_or(RTKA_TRUE, RTKA_UNKNOWN) == RTKA_TRUE);
    printf("  ✓ OR operations correct\n");
    
    /* Test NOT */
    printf("\nTesting NOT (negation):\n");
    printf("  ¬TRUE = %s\n", rtka_to_string(rtka_not(RTKA_TRUE)));
    printf("  ¬FALSE = %s\n", rtka_to_string(rtka_not(RTKA_FALSE)));
    printf("  ¬UNKNOWN = %s\n", rtka_to_string(rtka_not(RTKA_UNKNOWN)));
    assert(rtka_not(RTKA_TRUE) == RTKA_FALSE);
    assert(rtka_not(RTKA_FALSE) == RTKA_TRUE);
    assert(rtka_not(RTKA_UNKNOWN) == RTKA_UNKNOWN);
    printf("  ✓ NOT operations correct\n");
    
    /* Test EQV */
    printf("\nTesting EQV (equivalence):\n");
    printf("  TRUE ↔ TRUE = %s\n", rtka_to_string(rtka_eqv(RTKA_TRUE, RTKA_TRUE)));
    printf("  TRUE ↔ FALSE = %s\n", rtka_to_string(rtka_eqv(RTKA_TRUE, RTKA_FALSE)));
    printf("  FALSE ↔ FALSE = %s\n", rtka_to_string(rtka_eqv(RTKA_FALSE, RTKA_FALSE)));
    printf("  UNKNOWN ↔ TRUE = %s\n", rtka_to_string(rtka_eqv(RTKA_UNKNOWN, RTKA_TRUE)));
    assert(rtka_eqv(RTKA_TRUE, RTKA_TRUE) == RTKA_TRUE);
    assert(rtka_eqv(RTKA_TRUE, RTKA_FALSE) == RTKA_FALSE);
    assert(rtka_eqv(RTKA_FALSE, RTKA_FALSE) == RTKA_TRUE);
    assert(rtka_eqv(RTKA_UNKNOWN, RTKA_TRUE) == RTKA_UNKNOWN);
    printf("  ✓ EQV operations correct\n");
}

/* Test Use Case 1: Sensor Fusion */
void test_sensor_fusion(void) {
    printf("\n=== Testing Sensor Fusion (OR) ===\n");
    
    rtka_confidence_t confidences[] = {0.9, 0.7, 0.8, 0.95, 0.6};
    
    /* Run 1: [0, 1, 0, -1, 0] → Should be TRUE */
    {
        rtka_ternary_t inputs[] = {0, 1, 0, -1, 0};
        printf("  Input: [UNKNOWN, TRUE, UNKNOWN, FALSE, UNKNOWN]\n");
        printf("  Confidences: [0.9, 0.7, 0.8, 0.95, 0.6]\n");
        
        rtka_result_t result = rtka_eval_or(inputs, confidences, 5);
        
        printf("  Result: %s (confidence: %.4f)\n", rtka_to_string(result.value), result.confidence);
        printf("  Early terminated: %s after %zu operations\n", 
               result.early_terminated ? "YES" : "NO", result.operations_performed);
        
        assert(result.value == RTKA_TRUE);
        assert(result.early_terminated == true);
        printf("  ✓ TRUE absorbs in OR operation\n\n");
    }
    
    /* Run 2: [0, 0, 0, 0, 0] → Should remain UNKNOWN */
    {
        rtka_ternary_t inputs[] = {0, 0, 0, 0, 0};
        printf("  Input: [UNKNOWN, UNKNOWN, UNKNOWN, UNKNOWN, UNKNOWN]\n");
        printf("  Confidences: [0.9, 0.7, 0.8, 0.95, 0.6]\n");
        
        rtka_result_t result = rtka_eval_or(inputs, confidences, 5);
        
        printf("  Result: %s (confidence: %.4f)\n", rtka_to_string(result.value), result.confidence);
        printf("  Operations performed: %zu\n", result.operations_performed);
        
        assert(result.value == RTKA_UNKNOWN);
        printf("  ✓ All UNKNOWN preserved\n\n");
    }
    
    /* Run 3: [0, -1, -1, -1, -1] → Should remain UNKNOWN */
    {
        rtka_ternary_t inputs[] = {0, -1, -1, -1, -1};
        printf("  Input: [UNKNOWN, FALSE, FALSE, FALSE, FALSE]\n");
        printf("  Confidences: [0.9, 0.7, 0.8, 0.95, 0.6]\n");
        
        rtka_result_t result = rtka_eval_or(inputs, confidences, 5);
        
        printf("  Result: %s (confidence: %.4f)\n", rtka_to_string(result.value), result.confidence);
        printf("  Operations performed: %zu\n", result.operations_performed);
        
        assert(result.value == RTKA_UNKNOWN);
        printf("  ✓ UNKNOWN persists with no TRUE\n\n");
    }
}

/* Test Use Case 2: Evidence Chaining */
void test_evidence_chaining(void) {
    printf("\n=== Testing Evidence Chaining (AND) ===\n");
    
    rtka_confidence_t confidences[] = {0.85, 0.75, 0.9, 0.65};
    
    /* Run 1: [1, 1, 0, 1] → Should be UNKNOWN */
    {
        rtka_ternary_t inputs[] = {1, 1, 0, 1};
        printf("  Input: [TRUE, TRUE, UNKNOWN, TRUE]\n");
        printf("  Confidences: [0.85, 0.75, 0.9, 0.65]\n");
        
        rtka_result_t result = rtka_eval_and(inputs, confidences, 4);
        
        printf("  Result: %s (confidence: %.4f)\n", rtka_to_string(result.value), result.confidence);
        printf("  Operations performed: %zu\n", result.operations_performed);
        
        assert(result.value == RTKA_UNKNOWN);
        printf("  ✓ UNKNOWN persists in AND chain\n\n");
    }
    
    /* Run 2: [1, 1, 1, -1] → Should be FALSE */
    {
        rtka_ternary_t inputs[] = {1, 1, 1, -1};
        printf("  Input: [TRUE, TRUE, TRUE, FALSE]\n");
        printf("  Confidences: [0.85, 0.75, 0.9, 0.65]\n");
        
        rtka_result_t result = rtka_eval_and(inputs, confidences, 4);
        
        printf("  Result: %s (confidence: %.4f)\n", rtka_to_string(result.value), result.confidence);
        printf("  Early terminated: %s after %zu operations\n",
               result.early_terminated ? "YES" : "NO", result.operations_performed);
        
        assert(result.value == RTKA_FALSE);
        assert(result.early_terminated == true);
        printf("  ✓ FALSE absorbs in AND operation\n\n");
    }
    
    /* Run 3: [1, 1, 1, 1] → Should be TRUE */
    {
        rtka_ternary_t inputs[] = {1, 1, 1, 1};
        printf("  Input: [TRUE, TRUE, TRUE, TRUE]\n");
        printf("  Confidences: [0.85, 0.75, 0.9, 0.65]\n");
        
        rtka_result_t result = rtka_eval_and(inputs, confidences, 4);
        
        printf("  Result: %s (confidence: %.4f)\n", rtka_to_string(result.value), result.confidence);
        printf("  Expected confidence: 0.85 * 0.75 * 0.9 * 0.65 = 0.3729\n");
        printf("  Operations performed: %zu\n", result.operations_performed);
        
        assert(result.value == RTKA_TRUE);
        assert(fabs(result.confidence - 0.372937) < 0.001);
        printf("  ✓ All TRUE produces TRUE with correct confidence\n\n");
    }
}

/* Test UNKNOWN persistence probability */
void test_unknown_persistence(void) {
    printf("\n=== Testing UNKNOWN Persistence ===\n");
    printf("Model: P(UNKNOWN persists) = (2/3)^(n-1)\n");
    printf("Starting with UNKNOWN, followed by n-1 random inputs from {-1,0,1}\n\n");
    
    srand(42);  /* Fixed seed for reproducibility */
    int trials = 10000;
    
    /* Test for multiple values of n */
    for (int n = 1; n <= 6; n++) {
        int unknown_count_and = 0;
        int unknown_count_or = 0;
        
        for (int trial = 0; trial < trials; trial++) {
            rtka_ternary_t inputs[10];  /* Max size we'll test */
            inputs[0] = RTKA_UNKNOWN;
            
            /* Generate n-1 random inputs */
            for (int i = 1; i < n; i++) {
                int r = rand() % 3;
                inputs[i] = (rtka_ternary_t)(r - 1);
            }
            
            rtka_result_t result_and = rtka_eval_and(inputs, NULL, n);
            rtka_result_t result_or = rtka_eval_or(inputs, NULL, n);
            
            if (result_and.value == RTKA_UNKNOWN) unknown_count_and++;
            if (result_or.value == RTKA_UNKNOWN) unknown_count_or++;
        }
        
        double empirical_and = (double)unknown_count_and / trials;
        double empirical_or = (double)unknown_count_or / trials;
        double theoretical = pow(2.0 / 3.0, n - 1);
        
        printf("  n=%d: Theory=%.4f  AND=%.4f (err=%.4f)  OR=%.4f (err=%.4f)\n", 
               n, theoretical, 
               empirical_and, fabs(empirical_and - theoretical),
               empirical_or, fabs(empirical_or - theoretical));
        
        assert(fabs(empirical_and - theoretical) < 0.01);
        assert(fabs(empirical_or - theoretical) < 0.01);
    }
    
    printf("\n  ✓ UNKNOWN persistence matches (2/3)^(n-1) model for both AND and OR\n");
}

/* Main test runner */
int main(void) {
    printf("\n");
    printf("RTKA-U TEST SUITE\n");
    printf("=================\n");
    
    test_basic_operations();
    test_sensor_fusion();
    test_evidence_chaining();
    test_unknown_persistence();
    
    printf("\n=================\n");
    printf("ALL TESTS PASSED\n");
    printf("=================\n\n");
    
    return 0;
}
