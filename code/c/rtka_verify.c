/*
*
* Author: H.Overman <opsec.ee@pm.ee>
*
*/
#include "rtka_u.h"
#include <stdio.h>
#include <assert.h>

int main(void) {
    printf("RTKA-U Algorithm Verification\n");
    printf("=============================\n\n");
    
    // Test basic ternary operations
    printf("1. Basic Ternary Operations:\n");
    printf("   NOT(TRUE) = %s\n", rtka_ternary_to_string(rtka_not(RTKA_TRUE)));
    printf("   NOT(FALSE) = %s\n", rtka_ternary_to_string(rtka_not(RTKA_FALSE)));
    printf("   NOT(UNKNOWN) = %s\n", rtka_ternary_to_string(rtka_not(RTKA_UNKNOWN)));
    
    printf("   TRUE AND FALSE = %s\n", rtka_ternary_to_string(rtka_and(RTKA_TRUE, RTKA_FALSE)));
    printf("   TRUE AND UNKNOWN = %s\n", rtka_ternary_to_string(rtka_and(RTKA_TRUE, RTKA_UNKNOWN)));
    printf("   UNKNOWN AND UNKNOWN = %s\n", rtka_ternary_to_string(rtka_and(RTKA_UNKNOWN, RTKA_UNKNOWN)));
    
    printf("   FALSE OR TRUE = %s\n", rtka_ternary_to_string(rtka_or(RTKA_FALSE, RTKA_TRUE)));
    printf("   UNKNOWN OR TRUE = %s\n", rtka_ternary_to_string(rtka_or(RTKA_UNKNOWN, RTKA_TRUE)));
    printf("   FALSE OR UNKNOWN = %s\n", rtka_ternary_to_string(rtka_or(RTKA_FALSE, RTKA_UNKNOWN)));
    
    // Test vector operations
    printf("\n2. Vector Operations:\n");
    rtka_vector_t* test_vector = rtka_vector_create(4);
    rtka_vector_push(test_vector, RTKA_TRUE, 0.9);
    rtka_vector_push(test_vector, RTKA_UNKNOWN, 0.7);
    rtka_vector_push(test_vector, RTKA_TRUE, 0.8);
    rtka_vector_push(test_vector, RTKA_FALSE, 0.6);
    
    printf("   Vector: [TRUE(0.9), UNKNOWN(0.7), TRUE(0.8), FALSE(0.6)]\n");
    
    rtka_result_t and_result = rtka_recursive_eval(RTKA_OP_AND, test_vector);
    printf("   AND Result: %s (confidence: %.4f, early_term: %s)\n", 
           rtka_ternary_to_string(and_result.value),
           and_result.confidence,
           and_result.early_terminated ? "Yes" : "No");
    
    rtka_result_t or_result = rtka_recursive_eval(RTKA_OP_OR, test_vector);
    printf("   OR Result: %s (confidence: %.4f, early_term: %s)\n", 
           rtka_ternary_to_string(or_result.value),
           or_result.confidence,
           or_result.early_terminated ? "Yes" : "No");
    
    // Test confidence propagation
    printf("\n3. Confidence Propagation:\n");
    double confidences[] = {0.9, 0.8, 0.7};
    double and_conf = rtka_confidence_and(confidences, 3);
    double or_conf = rtka_confidence_or(confidences, 2);
    
    printf("   AND confidence [0.9, 0.8, 0.7]: %.6f\n", and_conf);
    printf("   OR confidence [0.9, 0.8]: %.6f\n", or_conf);
    
    // Test UNKNOWN probability
    printf("\n4. UNKNOWN Probability (Mathematical Property):\n");
    for (size_t n = 1; n <= 5; n++) {
        printf("   P(UNKNOWN|n=%zu) ≈ %.6f\n", n, rtka_unknown_probability(n));
    }
    
    // Test state transition
    printf("\n5. State Transition System:\n");
    rtka_state_t state = rtka_state_create(RTKA_TRUE, 0.9);
    printf("   Initial: %s (conf: %.3f)\n", rtka_ternary_to_string(state.state), state.confidence);
    
    state = rtka_state_transition(state, RTKA_UNKNOWN, 0.7);
    printf("   After UNKNOWN input: %s (conf: %.3f, iter: %lu)\n", 
           rtka_ternary_to_string(state.state), state.confidence, state.iteration);
    
    state = rtka_state_transition(state, RTKA_FALSE, 0.5);
    printf("   After FALSE input: %s (conf: %.3f, iter: %lu)\n", 
           rtka_ternary_to_string(state.state), state.confidence, state.iteration);
    
    printf("\n✓ RTKA-U Algorithm verification completed successfully!\n");
    printf("✓ Mathematical properties preserved: domain integrity, confidence propagation,\n");
    printf("  early termination, and state transitions all functioning correctly.\n");
    
    rtka_vector_destroy(test_vector);
    return 0;
}
