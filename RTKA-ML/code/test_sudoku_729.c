/**
 * File: test_sudoku_729.c
 * Copyright (c) 2025 - H.Overman opsec.ee@pm.me
 * 
 * PROPRIETARY AND CONFIDENTIAL
 * This file is proprietary to H. Overman and may not be reproduced,
 * distributed, or used without explicit written permission.
 *
 * For licensing inquiries: opsec.ee@pm.me
 *
 * Test RTKA 729-State Sudoku Solver
 */

#include "rtka_sudoku_729.h"
#include <stdio.h>
#include <time.h>
#include <stdlib.h>

void print_header(void) {
    printf("╔════════════════════════════════════════════╗\n");
    printf("║    RTKA 729-STATE SUDOKU SOLVER v1.0       ║\n");
    printf("║    Using Constraint Propagation Logic      ║\n");
    printf("╚════════════════════════════════════════════╝\n\n");
}

void print_puzzle_header(const char* name) {
    printf("┌────────────────────────────────────────────┐\n");
    printf("│ %s%-43s│\n", name, " ");
    printf("└────────────────────────────────────────────┘\n\n");
}

void print_analysis(sudoku_729_t* puzzle) {
    printf("┌─── RTKA 729-State Analysis ────────────┐\n");
    printf("│ Total States: 729 (81 cells × 9 digits)│\n");
    printf("│ TRUE states:    %3d (must be here)     │\n", puzzle->filled_count);
    printf("│ FALSE states:   %3d (cannot be here)   │\n", 729 - puzzle->filled_count - puzzle->unknown_count);
    printf("│ UNKNOWN states: %3d (might be here)    │\n", puzzle->unknown_count);
    printf("│                                        │\n");
    printf("│ Constraint Groups: 81 total            │\n");
    printf("│ - 27 horizontal (9 rows × 3)           │\n");
    printf("│ - 27 vertical (9 cols × 3)             │\n");
    printf("│ - 27 box (9 boxes × 3)                 │\n");
    printf("└────────────────────────────────────────┘\n\n");
}

int main(void) {
    print_header();
    
    /* AI Escargot - World's hardest */
    uint8_t escargot[9][9] = {
        {1,0,0, 0,0,7, 0,9,0},
        {0,3,0, 0,2,0, 0,0,8},
        {0,0,9, 6,0,0, 5,0,0},
        {0,0,5, 3,0,0, 9,0,0},
        {0,1,0, 0,8,0, 0,0,2},
        {6,0,0, 0,0,4, 0,0,0},
        {3,0,0, 0,0,0, 0,1,0},
        {0,4,0, 0,0,0, 0,0,7},
        {0,0,7, 0,0,0, 3,0,0}
    };
    
    /* Platinum Blonde */
    uint8_t platinum[9][9] = {
        {0,0,0, 0,0,0, 1,2,0},
        {0,0,0, 0,3,5, 0,0,0},
        {0,0,0, 6,0,0, 0,7,0},
        {7,0,0, 0,0,0, 3,0,0},
        {0,0,0, 4,0,0, 8,0,0},
        {1,0,0, 0,0,0, 0,0,0},
        {0,0,0, 1,2,0, 0,0,0},
        {0,8,0, 0,0,0, 0,4,0},
        {0,5,0, 0,0,0, 6,0,0}
    };
    
    /* Golden Nugget */
    uint8_t golden[9][9] = {
        {0,0,0, 0,0,0, 0,3,9},
        {0,0,0, 0,0,1, 0,0,5},
        {0,0,3, 0,5,0, 8,0,0},
        {0,0,8, 0,9,0, 0,0,6},
        {0,7,0, 0,0,2, 0,0,0},
        {1,0,0, 4,0,0, 0,0,0},
        {0,0,9, 0,8,0, 0,5,0},
        {0,2,0, 0,0,0, 6,0,0},
        {4,0,0, 7,0,0, 0,0,0}
    };
    
    struct {
        const char* name;
        uint8_t (*puzzle)[9];
    } puzzles[] = {
        {"Puzzle 1: AI Escargot (Hardest)", escargot},
        {"Puzzle 2: Platinum Blonde (Very Hard)", platinum},
        {"Puzzle 3: Golden Nugget (Very Hard)", golden}
    };
    
    for (int i = 0; i < 3; i++) {
        print_puzzle_header(puzzles[i].name);
        
        printf("Initial puzzle:\n");
        rtka_print_grid(puzzles[i].puzzle);
        
        sudoku_729_t* solver = rtka_sudoku_init(puzzles[i].puzzle);
        if (!solver) {
            printf("Failed to initialize\n");
            continue;
        }
        
        printf("\nInitial state: %d cells filled\n\n", solver->filled_count);
        
        solver->unknown_count = 0;
        for (int c = 0; c < 81; c++) {
            for (int d = 0; d < 9; d++) {
                if (solver->cell_digit[c][d].value == RTKA_UNKNOWN) {
                    solver->unknown_count++;
                }
            }
        }
        
        print_analysis(solver);
        
        clock_t start = clock();
        bool solved = rtka_solve_recursive(solver);
        double elapsed = (double)(clock() - start) / CLOCKS_PER_SEC;
        
        if (solved) {
            printf("SOLVED in %.6f seconds!\n\n", elapsed);
            printf("Solution:\n");
            rtka_print_puzzle(solver);
            
            if (rtka_validate_solution(solver)) {
                printf("\n✓ Solution validated - All constraints satisfied!\n\n");
            }
            
            solver->unknown_count = 0;
            print_analysis(solver);
        } else {
            printf("❌ Failed to solve\n");
        }
        
        free(solver);
        printf("\n════════════════════════════════════════\n\n");
    }
    
    printf("\n╔════════════════════════════════════════════╗\n");
    printf("║           RTKA 729 ADVANTAGES              ║\n");
    printf("╠════════════════════════════════════════════╣\n");
    printf("║ 1. Tracks 729 ternary states efficiently   ║\n");
    printf("║ 2. Propagates constraints through logic    ║\n");
    printf("║ 3. UNKNOWN→FALSE elimination is instant    ║\n");
    printf("║ 4. Bit operations for 27×27 constraints    ║\n");
    printf("║ 5. No random evolution needed              ║\n");
    printf("║ 6. Confidence guides smart backtracking    ║\n");
    printf("╚════════════════════════════════════════════╝\n");
    
    return 0;
}
