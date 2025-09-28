/**
 * File: rtka_sudoku_729.h
 * Copyright (c) 2025 - H.Overman opsec.ee@pm.me
 * 
 * PROPRIETARY AND CONFIDENTIAL
 * This file is proprietary to H. Overman and may not be reproduced,
 * distributed, or used without explicit written permission.
 *
 * For licensing inquiries: opsec.ee@pm.me
 *
 * RTKA 729-State Sudoku Solver
 */

#ifndef RTKA_SUDOKU_729_H
#define RTKA_SUDOKU_729_H

#include "rtka_types.h"
#include "rtka_u_core.h"
#include <stdint.h>
#include <stdbool.h>

/* Peer list for constraint propagation */
typedef struct {
    uint32_t cells[20];
    uint32_t count;
} peer_list_t;

/* Main solver structure - 729 states (81 cells × 9 digits) */
typedef struct {
    rtka_state_t cell_digit[81][9];  /* Ternary state for each digit in each cell */
    uint16_t possible[81];            /* Bitmask of possible digits */
    uint16_t row_used[9];             /* Bitmask of used digits per row */
    uint16_t col_used[9];             /* Bitmask of used digits per col */
    uint16_t box_used[9];             /* Bitmask of used digits per box */
    uint8_t solution[81];             /* Final solution */
    float cell_confidence[81];        /* Confidence per cell */
    uint32_t filled_count;            /* Number of filled cells */
    uint32_t unknown_count;           /* For statistics */
} sudoku_729_t;

/* Bit manipulation helpers */
#define bit_set(mask, bit) ((mask) & (1U << (bit)))
#define set_bit(mask, bit) ((mask) | (1U << (bit)))
#define clear_bit(mask, bit) ((mask) & ~(1U << (bit)))
#define count_bits(mask) __builtin_popcount(mask)

static inline int32_t get_single_bit(uint16_t mask) {
    if (count_bits(mask) != 1) return -1;
    return __builtin_ctz(mask);
}

/* Initialization */
sudoku_729_t* rtka_sudoku_init(uint8_t grid[9][9]);

/* Core solving functions */
bool rtka_place_digit(sudoku_729_t* puzzle, uint32_t cell, uint32_t digit);
bool rtka_eliminate_digit(sudoku_729_t* puzzle, uint32_t cell, uint32_t digit);
bool rtka_propagate_constraints(sudoku_729_t* puzzle);
bool rtka_solve_recursive(sudoku_729_t* puzzle);

/* Validation */
bool rtka_validate_solution(sudoku_729_t* puzzle);
bool rtka_check_constraints(sudoku_729_t* puzzle);

/* Display */
void rtka_print_puzzle(sudoku_729_t* puzzle);
void rtka_print_grid(uint8_t grid[9][9]);
void rtka_print_state_analysis(sudoku_729_t* puzzle);

/* Helper functions */
static inline uint32_t get_cell(uint32_t row, uint32_t col) {
    return row * 9 + col;
}

static inline uint32_t get_box(uint32_t row, uint32_t col) {
    return (row / 3) * 3 + (col / 3);
}

peer_list_t get_peers(uint32_t cell);

#endif /* RTKA_SUDOKU_729_H */
