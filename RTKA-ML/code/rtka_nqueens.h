/**
 * File: rtka_nqueens.h
 * Copyright (c) 2025 - H.Overman opsec.ee@pm.me
 * 
 * PROPRIETARY AND CONFIDENTIAL
 * This file is proprietary to H. Overman and may not be reproduced,
 * distributed, or used without explicit written permission.
 *
 * For licensing inquiries: opsec.ee@pm.me
 *
 * RTKA N-Queens Solver - Enhanced with full ternary propagation
 */

#ifndef RTKA_NQUEENS_H
#define RTKA_NQUEENS_H

#include "rtka_types.h"
#include "rtka_u_core.h"
#include <stdint.h>
#include <stdbool.h>

#define NQUEENS_MAX_SIZE 32

typedef struct {
    /* Ternary board state: TRUE=queen, FALSE=threatened, UNKNOWN=possible */
    rtka_state_t board[NQUEENS_MAX_SIZE][NQUEENS_MAX_SIZE];
    
    /* Board dimensions and state */
    uint32_t n;
    uint32_t queens_placed;
    
    /* RTKA-specific tracking */
    uint32_t rtka_transitions;      /* Total state transitions */
    uint32_t constraint_propagations; /* Constraint propagation count */
    uint32_t confidence_updates;     /* Confidence recalculations */
    uint32_t backtrack_count;
    
    /* Threat tracking using ternary logic */
    rtka_state_t row_threats[NQUEENS_MAX_SIZE];      /* Row threat levels */
    rtka_state_t col_threats[NQUEENS_MAX_SIZE];      /* Column threat levels */
    rtka_state_t diag1_threats[NQUEENS_MAX_SIZE*2];  /* Main diagonal threats */
    rtka_state_t diag2_threats[NQUEENS_MAX_SIZE*2];  /* Anti-diagonal threats */
    
    /* Confidence tracking for intelligent backtracking */
    rtka_confidence_t row_confidence[NQUEENS_MAX_SIZE];
    rtka_confidence_t col_confidence[NQUEENS_MAX_SIZE];
    rtka_confidence_t board_confidence;  /* Overall board confidence */
    
    /* MRV heuristic data */
    uint32_t row_possibilities[NQUEENS_MAX_SIZE];
    uint32_t col_possibilities[NQUEENS_MAX_SIZE];
} nqueens_state_t;

/* Core functions */
void rtka_nqueens_init(nqueens_state_t* state, uint32_t n);
bool rtka_nqueens_solve(nqueens_state_t* state);
bool rtka_nqueens_solve_all(nqueens_state_t* state, uint32_t* solution_count, uint32_t max_solutions);

/* Enhanced RTKA operations */
bool rtka_nqueens_place_queen_propagate(nqueens_state_t* state, uint32_t row, uint32_t col);
void rtka_nqueens_remove_queen_restore(nqueens_state_t* state, uint32_t row, uint32_t col);
void rtka_nqueens_propagate_constraints(nqueens_state_t* state, uint32_t row, uint32_t col);
void rtka_nqueens_update_confidence(nqueens_state_t* state);

/* Heuristics using RTKA confidence */
uint32_t rtka_nqueens_select_row_mrv(nqueens_state_t* state);
uint32_t rtka_nqueens_select_col_mrv(nqueens_state_t* state, uint32_t row);
rtka_confidence_t rtka_nqueens_position_confidence(nqueens_state_t* state, uint32_t row, uint32_t col);

/* Constraint checking with ternary logic */
bool rtka_nqueens_is_safe_ternary(nqueens_state_t* state, uint32_t row, uint32_t col);
rtka_state_t rtka_nqueens_compute_threat(nqueens_state_t* state, uint32_t row, uint32_t col);

/* Validation and display */
bool rtka_nqueens_validate(const nqueens_state_t* state);
void rtka_nqueens_print(const nqueens_state_t* state);
void rtka_nqueens_print_enhanced(const nqueens_state_t* state);
void rtka_nqueens_print_stats(const nqueens_state_t* state);

#endif /* RTKA_NQUEENS_H */
