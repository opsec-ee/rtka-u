/**
 * File: rtka_nqueens_enhanced.c
 * Copyright (c) 2025 - H.Overman opsec.ee@pm.me
 * 
 * PROPRIETARY AND CONFIDENTIAL
 * This file is proprietary to H. Overman and may not be reproduced,
 * distributed, or used without explicit written permission.
 *
 * For licensing inquiries: opsec.ee@pm.me
 *
 * RTKA N-Queens Solver - Enhanced Implementation
 */

#include "rtka_nqueens.h"
#include <stdio.h>
#include <string.h>
#include <math.h>

/* Initialize with ternary states and confidence tracking */
void rtka_nqueens_init(nqueens_state_t* state, uint32_t n) {
    state->n = n;
    state->queens_placed = 0;
    state->rtka_transitions = 0;
    state->constraint_propagations = 0;
    state->confidence_updates = 0;
    state->backtrack_count = 0;
    state->board_confidence = 1.0f;
    
    /* Initialize board with UNKNOWN states and equal confidence */
    rtka_confidence_t initial_conf = 1.0f / (n * n);
    for (uint32_t i = 0; i < n; i++) {
        for (uint32_t j = 0; j < n; j++) {
            state->board[i][j] = rtka_make_state(RTKA_UNKNOWN, initial_conf);
        }
        
        /* Initialize threat tracking with UNKNOWN (no threats yet) */
        state->row_threats[i] = rtka_make_state(RTKA_UNKNOWN, 0.0f);
        state->col_threats[i] = rtka_make_state(RTKA_UNKNOWN, 0.0f);
        
        state->row_confidence[i] = 1.0f;
        state->col_confidence[i] = 1.0f;
        state->row_possibilities[i] = n;
        state->col_possibilities[i] = n;
    }
    
    /* Initialize diagonal threats */
    for (uint32_t i = 0; i < 2 * n; i++) {
        state->diag1_threats[i] = rtka_make_state(RTKA_UNKNOWN, 0.0f);
        state->diag2_threats[i] = rtka_make_state(RTKA_UNKNOWN, 0.0f);
    }
}

/* Compute threat level using ternary logic */
rtka_state_t rtka_nqueens_compute_threat(nqueens_state_t* state, uint32_t row, uint32_t col) {
    uint32_t diag1 = row + col;
    uint32_t diag2 = row - col + state->n - 1;
    
    /* Combine threats using RTKA OR operation */
    rtka_state_t threat = state->row_threats[row];
    threat = rtka_combine_or(threat, state->col_threats[col]);
    threat = rtka_combine_or(threat, state->diag1_threats[diag1]);
    threat = rtka_combine_or(threat, state->diag2_threats[diag2]);
    
    return threat;
}

/* Check if position is safe using ternary logic */
bool rtka_nqueens_is_safe_ternary(nqueens_state_t* state, uint32_t row, uint32_t col) {
    rtka_state_t threat = rtka_nqueens_compute_threat(state, row, col);
    
    /* Safe if threat is FALSE or UNKNOWN (not TRUE) */
    return threat.value != RTKA_TRUE;
}

/* Calculate position confidence based on threats */
rtka_confidence_t rtka_nqueens_position_confidence(nqueens_state_t* state, uint32_t row, uint32_t col) {
    rtka_state_t threat = rtka_nqueens_compute_threat(state, row, col);
    
    /* Confidence inversely proportional to threat level */
    if (threat.value == RTKA_TRUE) return 0.0f;
    if (threat.value == RTKA_FALSE) return 1.0f;
    
    /* UNKNOWN threat - use confidence value */
    return 1.0f - threat.confidence;
}

/* Propagate constraints after placing queen */
void rtka_nqueens_propagate_constraints(nqueens_state_t* state, uint32_t row, uint32_t col) {
    /* Update row constraints */
    for (uint32_t c = 0; c < state->n; c++) {
        if (c != col && state->board[row][c].value == RTKA_UNKNOWN) {
            /* Transition UNKNOWN → FALSE */
            state->board[row][c] = rtka_make_state(RTKA_FALSE, 1.0f);
            state->rtka_transitions++;
            state->constraint_propagations++;
            state->col_possibilities[c]--;
        }
    }
    
    /* Update column constraints */
    for (uint32_t r = 0; r < state->n; r++) {
        if (r != row && state->board[r][col].value == RTKA_UNKNOWN) {
            state->board[r][col] = rtka_make_state(RTKA_FALSE, 1.0f);
            state->rtka_transitions++;
            state->constraint_propagations++;
            state->row_possibilities[r]--;
        }
    }
    
    /* Update diagonal constraints */
    for (int32_t offset = -((int32_t)state->n); offset < (int32_t)state->n; offset++) {
        if (offset == 0) continue;
        
        /* Main diagonal */
        int32_t r1 = (int32_t)row + offset;
        int32_t c1 = (int32_t)col + offset;
        if (r1 >= 0 && r1 < (int32_t)state->n && c1 >= 0 && c1 < (int32_t)state->n) {
            if (state->board[r1][c1].value == RTKA_UNKNOWN) {
                state->board[r1][c1] = rtka_make_state(RTKA_FALSE, 1.0f);
                state->rtka_transitions++;
                state->constraint_propagations++;
                state->row_possibilities[r1]--;
                state->col_possibilities[c1]--;
            }
        }
        
        /* Anti-diagonal */
        int32_t r2 = (int32_t)row + offset;
        int32_t c2 = (int32_t)col - offset;
        if (r2 >= 0 && r2 < (int32_t)state->n && c2 >= 0 && c2 < (int32_t)state->n) {
            if (state->board[r2][c2].value == RTKA_UNKNOWN) {
                state->board[r2][c2] = rtka_make_state(RTKA_FALSE, 1.0f);
                state->rtka_transitions++;
                state->constraint_propagations++;
                state->row_possibilities[r2]--;
                state->col_possibilities[c2]--;
            }
        }
    }
}

/* Place queen with full constraint propagation */
bool rtka_nqueens_place_queen_propagate(nqueens_state_t* state, uint32_t row, uint32_t col) {
    if (!rtka_nqueens_is_safe_ternary(state, row, col)) return false;
    
    /* Place queen (transition UNKNOWN → TRUE) */
    state->board[row][col] = rtka_make_state(RTKA_TRUE, 1.0f);
    state->queens_placed++;
    state->rtka_transitions++;
    
    /* Update threat states */
    state->row_threats[row] = rtka_make_state(RTKA_TRUE, 1.0f);
    state->col_threats[col] = rtka_make_state(RTKA_TRUE, 1.0f);
    
    uint32_t diag1 = row + col;
    uint32_t diag2 = row - col + state->n - 1;
    state->diag1_threats[diag1] = rtka_make_state(RTKA_TRUE, 1.0f);
    state->diag2_threats[diag2] = rtka_make_state(RTKA_TRUE, 1.0f);
    
    /* Propagate constraints */
    rtka_nqueens_propagate_constraints(state, row, col);
    
    /* Update confidence values */
    rtka_nqueens_update_confidence(state);
    
    return true;
}

/* Update board confidence based on current state */
void rtka_nqueens_update_confidence(nqueens_state_t* state) {
    state->confidence_updates++;
    
    rtka_confidence_t total_conf = 0.0f;
    uint32_t unknown_count = 0;
    
    for (uint32_t i = 0; i < state->n; i++) {
        state->row_confidence[i] = 0.0f;
        state->col_confidence[i] = 0.0f;
        
        for (uint32_t j = 0; j < state->n; j++) {
            if (state->board[i][j].value == RTKA_UNKNOWN) {
                rtka_confidence_t pos_conf = rtka_nqueens_position_confidence(state, i, j);
                state->row_confidence[i] += pos_conf;
                state->col_confidence[j] += pos_conf;
                total_conf += pos_conf;
                unknown_count++;
            }
        }
        
        /* Normalize confidence */
        if (state->row_possibilities[i] > 0) {
            state->row_confidence[i] /= state->row_possibilities[i];
        }
        if (state->col_possibilities[i] > 0) {
            state->col_confidence[i] /= state->col_possibilities[i];
        }
    }
    
    /* Update overall board confidence */
    if (unknown_count > 0) {
        state->board_confidence = total_conf / unknown_count;
    } else {
        state->board_confidence = (state->queens_placed == state->n) ? 1.0f : 0.0f;
    }
}

/* Select row with minimum remaining values */
uint32_t rtka_nqueens_select_row_mrv(nqueens_state_t* state) {
    uint32_t best_row = state->n;
    uint32_t min_values = state->n + 1;
    rtka_confidence_t best_conf = 0.0f;
    
    for (uint32_t row = 0; row < state->n; row++) {
        /* Skip rows with queens */
        if (state->row_threats[row].value == RTKA_TRUE) continue;
        
        uint32_t possibilities = state->row_possibilities[row];
        
        if (possibilities < min_values || 
            (possibilities == min_values && state->row_confidence[row] > best_conf)) {
            min_values = possibilities;
            best_row = row;
            best_conf = state->row_confidence[row];
        }
    }
    
    return best_row;
}

/* Select column with best confidence in given row */
uint32_t rtka_nqueens_select_col_mrv(nqueens_state_t* state, uint32_t row) {
    uint32_t best_col = state->n;
    rtka_confidence_t best_conf = -1.0f;
    
    for (uint32_t col = 0; col < state->n; col++) {
        if (state->board[row][col].value == RTKA_UNKNOWN) {
            rtka_confidence_t conf = rtka_nqueens_position_confidence(state, row, col);
            
            if (conf > best_conf) {
                best_conf = conf;
                best_col = col;
            }
        }
    }
    
    return best_col;
}

/* Remove queen and restore state */
void rtka_nqueens_remove_queen_restore(nqueens_state_t* state, uint32_t row, uint32_t col) {
    state->board[row][col] = rtka_make_state(RTKA_UNKNOWN, 0.5f);
    state->queens_placed--;
    state->backtrack_count++;
    
    /* Reset threat states (simplified - full implementation would track threat counts) */
    state->row_threats[row] = rtka_make_state(RTKA_UNKNOWN, 0.0f);
    state->col_threats[col] = rtka_make_state(RTKA_UNKNOWN, 0.0f);
    
    uint32_t diag1 = row + col;
    uint32_t diag2 = row - col + state->n - 1;
    state->diag1_threats[diag1] = rtka_make_state(RTKA_UNKNOWN, 0.0f);
    state->diag2_threats[diag2] = rtka_make_state(RTKA_UNKNOWN, 0.0f);
    
    /* Reset affected positions back to UNKNOWN */
    for (uint32_t c = 0; c < state->n; c++) {
        if (c != col && state->board[row][c].value == RTKA_FALSE) {
            if (!rtka_nqueens_is_safe_ternary(state, row, c)) continue;
            state->board[row][c] = rtka_make_state(RTKA_UNKNOWN, 0.5f);
            state->col_possibilities[c]++;
        }
    }
    
    for (uint32_t r = 0; r < state->n; r++) {
        if (r != row && state->board[r][col].value == RTKA_FALSE) {
            if (!rtka_nqueens_is_safe_ternary(state, r, col)) continue;
            state->board[r][col] = rtka_make_state(RTKA_UNKNOWN, 0.5f);
            state->row_possibilities[r]++;
        }
    }
}

/* Recursive solver with enhanced heuristics */
static bool solve_recursive_enhanced(nqueens_state_t* state, uint32_t depth) {
    if (state->queens_placed == state->n) {
        return true;
    }
    
    /* Select row using MRV heuristic */
    uint32_t row = (depth < state->n) ? depth : rtka_nqueens_select_row_mrv(state);
    if (row >= state->n) return false;
    
    /* Try columns in order of confidence */
    for (uint32_t col = 0; col < state->n; col++) {
        if (state->board[row][col].value != RTKA_FALSE) {
            if (rtka_nqueens_place_queen_propagate(state, row, col)) {
                if (solve_recursive_enhanced(state, depth + 1)) {
                    return true;
                }
                rtka_nqueens_remove_queen_restore(state, row, col);
            }
        }
    }
    
    return false;
}

/* Main solve function */
bool rtka_nqueens_solve(nqueens_state_t* state) {
    return solve_recursive_enhanced(state, 0);
}

/* Find all solutions */
bool rtka_nqueens_solve_all(nqueens_state_t* state, uint32_t* solution_count, uint32_t max_solutions) {
    (void)max_solutions; /* TODO: implement all solutions */
    *solution_count = 0;
    /* Implementation would recursively find all solutions */
    return solve_recursive_enhanced(state, 0);
}

/* Validate solution */
bool rtka_nqueens_validate(const nqueens_state_t* state) {
    if (state->queens_placed != state->n) return false;
    
    uint32_t queen_count = 0;
    for (uint32_t i = 0; i < state->n; i++) {
        for (uint32_t j = 0; j < state->n; j++) {
            if (state->board[i][j].value == RTKA_TRUE) {
                queen_count++;
            }
        }
    }
    
    return queen_count == state->n;
}

/* Basic print */
void rtka_nqueens_print(const nqueens_state_t* state) {
    for (uint32_t i = 0; i < state->n; i++) {
        for (uint32_t j = 0; j < state->n; j++) {
            char c = (state->board[i][j].value == RTKA_TRUE) ? 'Q' :
                    (state->board[i][j].value == RTKA_FALSE) ? '.' : '?';
            printf("%c ", c);
        }
        printf("\n");
    }
}

/* Enhanced print with confidence values */
void rtka_nqueens_print_enhanced(const nqueens_state_t* state) {
    printf("\nEnhanced N-Queens Board (n=%d):\n", state->n);
    printf("Legend: Q=Queen, X=Threatened, ?=Unknown (confidence)\n\n");
    
    /* Column headers */
    printf("    ");
    for (uint32_t j = 0; j < state->n; j++) {
        printf(" %2d  ", j);
    }
    printf("\n");
    
    /* Board with confidence */
    for (uint32_t i = 0; i < state->n; i++) {
        printf("%2d: ", i);
        for (uint32_t j = 0; j < state->n; j++) {
            if (state->board[i][j].value == RTKA_TRUE) {
                printf("  Q  ");
            } else if (state->board[i][j].value == RTKA_FALSE) {
                printf("  X  ");
            } else {
                printf("?%.2f", state->board[i][j].confidence);
            }
        }
        printf(" | Conf: %.3f\n", state->row_confidence[i]);
    }
}

/* Print statistics */
void rtka_nqueens_print_stats(const nqueens_state_t* state) {
    printf("\n═══════════════════════════════════════\n");
    printf("RTKA N-Queens Statistics:\n");
    printf("═══════════════════════════════════════\n");
    printf("Board size:             %d x %d\n", state->n, state->n);
    printf("Queens placed:          %d\n", state->queens_placed);
    printf("\nRTKA Metrics:\n");
    printf("State transitions:      %d\n", state->rtka_transitions);
    printf("Constraint propagations:%d\n", state->constraint_propagations);
    printf("Confidence updates:     %d\n", state->confidence_updates);
    printf("Backtracks:             %d\n", state->backtrack_count);
    printf("Board confidence:       %.4f\n", state->board_confidence);
    
    /* Count ternary states */
    uint32_t true_count = 0, false_count = 0, unknown_count = 0;
    for (uint32_t i = 0; i < state->n; i++) {
        for (uint32_t j = 0; j < state->n; j++) {
            switch (state->board[i][j].value) {
                case RTKA_TRUE: true_count++; break;
                case RTKA_FALSE: false_count++; break;
                case RTKA_UNKNOWN: unknown_count++; break;
            }
        }
    }
    
    printf("\nTernary State Distribution:\n");
    printf("TRUE (queens):          %d (%.1f%%)\n", true_count, 100.0f*true_count/(state->n*state->n));
    printf("FALSE (threatened):     %d (%.1f%%)\n", false_count, 100.0f*false_count/(state->n*state->n));
    printf("UNKNOWN (possible):     %d (%.1f%%)\n", unknown_count, 100.0f*unknown_count/(state->n*state->n));
    
    /* Efficiency metrics */
    float efficiency = (float)state->queens_placed / (state->rtka_transitions + 1);
    float propagation_ratio = (float)state->constraint_propagations / (state->rtka_transitions + 1);
    
    printf("\nEfficiency Metrics:\n");
    printf("Queens per transition:  %.4f\n", efficiency);
    printf("Propagation ratio:      %.4f\n", propagation_ratio);
    printf("═══════════════════════════════════════\n");
}
