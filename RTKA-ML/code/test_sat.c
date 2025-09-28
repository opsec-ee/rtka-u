/**
 * File: test_sat.c
 * Copyright (c) 2025 - H.Overman opsec.ee@pm.me
 *
 * PROPRIETARY AND CONFIDENTIAL
 * This file is proprietary to H. Overman and may not be reproduced,
 * distributed, or used without explicit written permission.
 *
 * For licensing inquiries: opsec.ee@pm.me
 *
 */

#include "rtka_sat.h"
#include <stdio.h>
#include <time.h>

int main(void) {
    printf("RTKA SAT Test\n");
    printf("=============\n\n");
    
    sat_state_t state;
    rtka_sat_init(&state, 3);
    
    int32_t c1[] = {1, 2};
    int32_t c2[] = {-1, 3};
    int32_t c3[] = {-2, -3};
    int32_t c4[] = {1, 3};
    
    rtka_sat_add_clause(&state, c1, 2);
    rtka_sat_add_clause(&state, c2, 2);
    rtka_sat_add_clause(&state, c3, 2);
    rtka_sat_add_clause(&state, c4, 2);
    
    printf("Formula: (x1∨x2) ∧ (¬x1∨x3) ∧ (¬x2∨¬x3) ∧ (x1∨x3)\n\n");
    
    clock_t start = clock();
    bool sat = rtka_sat_solve(&state);
    double elapsed = (double)(clock() - start) / CLOCKS_PER_SEC;
    
    if (sat) {
        printf("✅ SATISFIABLE in %.6f seconds\n", elapsed);
        printf("Solution: ");
        for (uint32_t v = 1; v <= 3; v++) {
            printf("x%d=%s ", v, 
                   state.variables[v].value == RTKA_TRUE ? "T" : "F");
        }
        printf("\nRTKA transitions: %d\n", state.rtka_transitions);
    } else {
        printf("❌ UNSATISFIABLE\n");
    }
    
    return 0;
}
