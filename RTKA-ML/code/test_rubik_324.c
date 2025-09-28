/**
 * File: test_rubik_324.c  
 * Copyright (c) 2025 - H.Overman opsec.ee@pm.me
 *
 * PROPRIETARY AND CONFIDENTIAL
 * This file is proprietary to H. Overman and may not be reproduced,
 * distributed, or used without explicit written permission.
 *
 * For licensing inquiries: opsec.ee@pm.me
 *
 * Test RTKA 324-State Rubik's Cube Solver
 */

#include "rtka_rubik_324.h"
#include <stdio.h>
#include <stdlib.h>
#include <time.h>

int main(void) {
    printf("╔════════════════════════════════════════════╗\n");
    printf("║   RTKA 324-STATE RUBIK'S CUBE SOLVER      ║\n");
    printf("║     Bit-Based Constraint Propagation      ║\n");
    printf("╚════════════════════════════════════════════╝\n\n");
    
    printf("The cube unwrapped as 2D constraint system:\n");
    printf("      [UP]\n");
    printf("[LEFT][FRONT][RIGHT][BACK]\n");
    printf("      [DOWN]\n\n");
    
    printf("324 ternary states = 54 stickers × 6 colors\n");
    printf("Using bit operations for O(1) rotations\n\n");
    
    /* Test 1: Create solved cube */
    printf("Initial solved state:\n");
    rubik_324_state_t* cube = rtka_rubik_324_create_solved();
    rtka_rubik_324_print_analysis(cube);
    
    /* Test 2: Single move */
    printf("\n═══════════════════════════════════════\n");
    printf("Test 1: Apply single U move\n");
    printf("═══════════════════════════════════════\n");
    
    rtka_rubik_324_rotate(cube, MOVE_324_U);
    rtka_rubik_324_print_analysis(cube);
    printf("Is solved? %s\n", rtka_rubik_324_is_solved(cube) ? "YES" : "NO");
    printf("Heuristic (correctness): %.2f%%\n", rtka_rubik_324_heuristic(cube) * 100);
    
    /* Test 3: Complete U4 = identity */
    printf("\n═══════════════════════════════════════\n");
    printf("Test 2: Apply 3 more U moves (should solve)\n");
    printf("═══════════════════════════════════════\n");
    
    for (int i = 0; i < 3; i++) {
        rtka_rubik_324_rotate(cube, MOVE_324_U);
    }
    
    rtka_rubik_324_print_analysis(cube);
    printf("Is solved? %s\n", rtka_rubik_324_is_solved(cube) ? "YES" : "NO");
    
    /* Test 4: Scramble and constraint propagation */
    printf("\n═══════════════════════════════════════\n");
    printf("Test 3: Scramble and constraint propagation\n");
    printf("═══════════════════════════════════════\n");
    
    rtka_rubik_324_free(cube);
    cube = rtka_rubik_324_create_solved();
    
    printf("Applying scramble: U R U' R'\n");
    rtka_rubik_324_rotate(cube, MOVE_324_U);
    printf("After U - transitions: %d\n", rtka_rubik_324_get_transitions(cube));
    
    rtka_rubik_324_rotate(cube, MOVE_324_R);
    printf("After R - transitions: %d\n", rtka_rubik_324_get_transitions(cube));
    
    /* U' = 3×U */
    for (int i = 0; i < 3; i++) rtka_rubik_324_rotate(cube, MOVE_324_U);
    printf("After U' - transitions: %d\n", rtka_rubik_324_get_transitions(cube));
    
    /* R' = 3×R */
    for (int i = 0; i < 3; i++) rtka_rubik_324_rotate(cube, MOVE_324_R);
    printf("After R' - transitions: %d\n", rtka_rubik_324_get_transitions(cube));
    
    rtka_rubik_324_print_analysis(cube);
    
    /* Test 5: Attempt to solve */
    printf("\n═══════════════════════════════════════\n");
    printf("Test 4: Attempt to solve\n");
    printf("═══════════════════════════════════════\n");
    
    clock_t start = clock();
    bool solved = rtka_rubik_324_solve(cube, 8);
    double elapsed = (double)(clock() - start) / CLOCKS_PER_SEC;
    
    if (solved) {
        printf("Solved in %.6f seconds!\n", elapsed);
    } else {
        printf("Not solved within depth limit\n");
    }
    
    rtka_rubik_324_print_analysis(cube);
    
    rtka_rubik_324_free(cube);
    
    printf("\n╔════════════════════════════════════════════╗\n");
    printf("║       324-STATE ADVANTAGES                ║\n");
    printf("╠════════════════════════════════════════════╣\n");
    printf("║ 1. Bit operations = O(1) rotations         ║\n");
    printf("║ 2. Constraint propagation eliminates states║\n");
    printf("║ 3. Edge/corner constraints enforced        ║\n");
    printf("║ 4. Ternary logic tracks possibilities      ║\n");
    printf("║ 5. 11×32-bit words = compact storage       ║\n");
    printf("║ 6. Parallel bit ops on modern CPUs         ║\n");
    printf("╚════════════════════════════════════════════╝\n");
    
    return 0;
}
