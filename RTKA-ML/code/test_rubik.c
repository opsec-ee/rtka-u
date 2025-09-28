/**
 * File: test_rubik.c
 * Copyright (c) 2025 - H.Overman opsec.ee@pm.me
 * 
 * PROPRIETARY AND CONFIDENTIAL
 * This file is proprietary to H. Overman and may not be reproduced,
 * distributed, or used without explicit written permission.
 *
 * For licensing inquiries: opsec.ee@pm.me
 *
 * Test RTKA Rubik's Cube Solver
 */

#include "rtka_rubik.h"
#include <stdio.h>
#include <time.h>
#include <stdlib.h>

int main(void) {
    printf("╔════════════════════════════════════════════╗\n");
    printf("║      RTKA RUBIK'S CUBE SOLVER v1.0        ║\n");
    printf("║         IDA* with Ternary States          ║\n");
    printf("╚════════════════════════════════════════════╝\n\n");
    
    /* Test 1: Simple scramble */
    printf("Test 1: Simple Scramble (4 moves)\n");
    printf("════════════════════════════════════════\n");
    
    rubik_state_t* cube = rtka_rubik_create_solved();
    
    /* Apply scramble: R U R' U' */
    rubik_move_t scramble[] = {MOVE_R, MOVE_U, MOVE_R_PRIME, MOVE_U_PRIME};
    printf("Scramble: R U R' U'\n\n");
    
    for (int i = 0; i < 4; i++) {
        rtka_rubik_apply_move(cube, scramble[i]);
    }
    
    printf("Scrambled state:\n");
    rtka_rubik_print(cube);
    
    rubik_solver_t* solver = rtka_rubik_solver_create(cube);
    
    clock_t start = clock();
    bool solved = rtka_rubik_solve(solver);
    double elapsed = (double)(clock() - start) / CLOCKS_PER_SEC;
    
    if (solved) {
        printf("\n✅ SOLVED in %.6f seconds!\n", elapsed);
        printf("Solution length: %d moves\n", solver->solution_length);
        printf("Nodes explored: %d\n", solver->nodes_explored);
        printf("RTKA transitions: %d\n", solver->rtka_transitions);
        
        printf("\nSolution: ");
        for (uint32_t i = 0; i < solver->solution_length; i++) {
            const char* move_names[] = {
                "U", "U'", "U2", "D", "D'", "D2",
                "F", "F'", "F2", "B", "B'", "B2",
                "L", "L'", "L2", "R", "R'", "R2"
            };
            printf("%s ", move_names[solver->solution[i]]);
        }
        printf("\n");
    } else {
        printf("❌ Failed to solve\n");
    }
    
    rtka_rubik_free(cube);
    free(solver->solution);
    free(solver);
    
    /* Test 2: Harder scramble */
    printf("\n\nTest 2: Complex Scramble (8 moves)\n");
    printf("════════════════════════════════════════\n");
    
    cube = rtka_rubik_create_solved();
    
    rubik_move_t scramble2[] = {
        MOVE_F, MOVE_R, MOVE_U_PRIME, MOVE_D,
        MOVE_R2, MOVE_B, MOVE_L_PRIME, MOVE_U2
    };
    printf("Scramble: F R U' D R2 B L' U2\n\n");
    
    for (int i = 0; i < 8; i++) {
        rtka_rubik_apply_move(cube, scramble2[i]);
    }
    
    solver = rtka_rubik_solver_create(cube);
    solver->max_depth = 12;  /* Increase search depth */
    
    start = clock();
    solved = rtka_rubik_solve(solver);
    elapsed = (double)(clock() - start) / CLOCKS_PER_SEC;
    
    if (solved) {
        printf("✅ SOLVED in %.6f seconds!\n", elapsed);
        printf("Solution length: %d moves\n", solver->solution_length);
        printf("Nodes explored: %d\n", solver->nodes_explored);
        printf("RTKA transitions: %d\n", solver->rtka_transitions);
    } else {
        printf("❌ Failed to solve (needs deeper search)\n");
    }
    
    rtka_rubik_free(cube);
    free(solver->solution);
    free(solver);
    
    printf("\n╔════════════════════════════════════════════╗\n");
    printf("║        RTKA RUBIK'S ADVANTAGES            ║\n");
    printf("╠════════════════════════════════════════════╣\n");
    printf("║ 1. Ternary states track cube confidence    ║\n");
    printf("║ 2. IDA* with ternary heuristics            ║\n");
    printf("║ 3. Confidence decay per move               ║\n");
    printf("║ 4. Manhattan distance with ternary logic   ║\n");
    printf("╚════════════════════════════════════════════╝\n");
    
    return 0;
}
