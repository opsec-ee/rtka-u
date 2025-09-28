/**
 * File: test_nqueens_enhanced.c
 * Copyright (c) 2025 - H.Overman opsec.ee@pm.me
 * 
 * PROPRIETARY AND CONFIDENTIAL
 * This file is proprietary to H. Overman and may not be reproduced,
 * distributed, or used without explicit written permission.
 *
 * For licensing inquiries: opsec.ee@pm.me
 *
 * Test RTKA Enhanced N-Queens Solver
 */

#include "rtka_nqueens.h"
#include <stdio.h>
#include <time.h>
#include <stdlib.h>

int main(void) {
    printf("╔════════════════════════════════════════════╗\n");
    printf("║   RTKA ENHANCED N-QUEENS SOLVER v2.0      ║\n");
    printf("║  Full Ternary Constraint Propagation      ║\n");
    printf("╚════════════════════════════════════════════╝\n\n");
    
    uint32_t sizes[] = {8, 12, 16, 20, 24};
    const char* difficulty[] = {"Easy", "Medium", "Hard", "Very Hard", "Expert"};
    
    for (int i = 0; i < 5; i++) {
        uint32_t n = sizes[i];
        nqueens_state_t state;
        
        printf("┌────────────────────────────────────────────┐\n");
        printf("│ %2d-Queens Problem (%s)                  │\n", n, difficulty[i]);
        printf("└────────────────────────────────────────────┘\n");
        
        rtka_nqueens_init(&state, n);
        
        printf("Initial state: %d×%d board\n", n, n);
        printf("Total positions: %d\n", n * n);
        printf("Initial confidence: %.4f\n\n", state.board_confidence);
        
        clock_t start = clock();
        bool solved = rtka_nqueens_solve(&state);
        double elapsed = (double)(clock() - start) / CLOCKS_PER_SEC;
        
        if (solved) {
            printf("✅ SOLVED in %.6f seconds!\n\n", elapsed);
            
            /* Show solution for smaller boards */
            if (n <= 12) {
                printf("Solution:\n");
                rtka_nqueens_print(&state);
                printf("\n");
            }
            
            /* Show enhanced view for 8-Queens */
            if (n == 8) {
                rtka_nqueens_print_enhanced(&state);
            }
            
            rtka_nqueens_print_stats(&state);
        } else {
            printf("❌ No solution found\n");
        }
        
        printf("\n");
    }
    
    /* Demonstrate confidence evolution on 8-Queens */
    printf("╔════════════════════════════════════════════╗\n");
    printf("║     RTKA Confidence Evolution Demo        ║\n");
    printf("╚════════════════════════════════════════════╝\n\n");
    
    nqueens_state_t demo;
    rtka_nqueens_init(&demo, 8);
    
    printf("Placing queens with confidence tracking:\n\n");
    
    /* Place first few queens and show confidence changes */
    uint32_t positions[][2] = {{0,0}, {1,2}, {2,4}};
    
    for (int i = 0; i < 3; i++) {
        printf("Step %d: Placing queen at (%d,%d)\n", i+1, positions[i][0], positions[i][1]);
        rtka_nqueens_place_queen_propagate(&demo, positions[i][0], positions[i][1]);
        
        printf("  State transitions: %d\n", demo.rtka_transitions);
        printf("  Constraint propagations: %d\n", demo.constraint_propagations);
        printf("  Board confidence: %.4f\n", demo.board_confidence);
        
        /* Count states */
        uint32_t unknown = 0, threatened = 0;
        for (uint32_t r = 0; r < 8; r++) {
            for (uint32_t c = 0; c < 8; c++) {
                if (demo.board[r][c].value == RTKA_UNKNOWN) unknown++;
                if (demo.board[r][c].value == RTKA_FALSE) threatened++;
            }
        }
        printf("  Available positions: %d\n", unknown);
        printf("  Threatened positions: %d\n\n", threatened);
    }
    
    printf("╔════════════════════════════════════════════╗\n");
    printf("║         RTKA N-QUEENS ADVANTAGES          ║\n");
    printf("╠════════════════════════════════════════════╣\n");
    printf("║ 1. Ternary states track all constraints    ║\n");
    printf("║ 2. Confidence guides intelligent search    ║\n");
    printf("║ 3. Constraint propagation reduces space    ║\n");
    printf("║ 4. MRV heuristic with confidence weights   ║\n");
    printf("║ 5. UNKNOWN→FALSE transitions are tracked   ║\n");
    printf("║ 6. Threat levels use ternary OR logic      ║\n");
    printf("║ 7. Board confidence measures solution      ║\n");
    printf("╚════════════════════════════════════════════╝\n");
    
    return 0;
}
