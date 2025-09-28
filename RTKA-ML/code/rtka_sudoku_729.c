/**
 * File: rtka_sudoku_729.c
 * Copyright (c) 2025 - H.Overman opsec.ee@pm.me
 * Email: opsec.ee@pm.me
 * 
 * PROPRIETARY AND CONFIDENTIAL
 * This file is proprietary to H. Overman and may not be reproduced,
 * distributed, or used without explicit written permission.
 *
 * For licensing inquiries: opsec.ee@pm.me
 *
 * RTKA Sudoku Solver Implementation
 * Using 729 ternary states for constraint propagation
 */

#include "rtka_sudoku_729.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* ============================================================================
 * PEER CALCULATION
 * ============================================================================ */

peer_list_t get_peers(uint32_t cell) {
    peer_list_t peers;
    peers.count = 0;
    
    uint32_t row = cell / 9;
    uint32_t col = cell % 9;
    uint32_t box = get_box(row, col);
    uint32_t box_row = (box / 3) * 3;
    uint32_t box_col = (box % 3) * 3;
    
    /* Add cells in same row */
    for (uint32_t c = 0; c < 9; c++) {
        if (c != col) {
            peers.cells[peers.count++] = row * 9 + c;
        }
    }
    
    /* Add cells in same column */
    for (uint32_t r = 0; r < 9; r++) {
        if (r != row) {
            peers.cells[peers.count++] = r * 9 + col;
        }
    }
    
    /* Add cells in same box (avoiding duplicates) */
    for (uint32_t br = box_row; br < box_row + 3; br++) {
        for (uint32_t bc = box_col; bc < box_col + 3; bc++) {
            if (br != row && bc != col) {
                peers.cells[peers.count++] = br * 9 + bc;
            }
        }
    }
    
    return peers;
}

/* ============================================================================
 * INITIALIZATION
 * ============================================================================ */

sudoku_729_t* rtka_sudoku_init(uint8_t grid[9][9]) {
    sudoku_729_t* puzzle = calloc(1, sizeof(sudoku_729_t));
    if (!puzzle) return NULL;
    
    /* Initialize all cells as UNKNOWN for all digits */
    for (uint32_t cell = 0; cell < 81; cell++) {
        puzzle->possible[cell] = 0x1FF;  /* All 9 bits set (digits 0-8) */
        puzzle->solution[cell] = 0xFF;   /* No solution yet */
        puzzle->cell_confidence[cell] = 0.0f;
        
        for (uint32_t digit = 0; digit < 9; digit++) {
            puzzle->cell_digit[cell][digit] = (rtka_state_t){
                .value = RTKA_UNKNOWN,
                .confidence = 1.0f / 9.0f  /* Equal probability initially */
            };
        }
    }
    
    /* Place given clues and propagate constraints */
    puzzle->filled_count = 0;
    for (uint32_t row = 0; row < 9; row++) {
        for (uint32_t col = 0; col < 9; col++) {
            if (grid[row][col] != 0) {
                uint32_t cell = get_cell(row, col);
                uint32_t digit = grid[row][col] - 1;  /* Convert to 0-8 */
                
                if (!rtka_place_digit(puzzle, cell, digit)) {
                    free(puzzle);
                    return NULL;  /* Invalid puzzle */
                }
            }
        }
    }
    
    return puzzle;
}

/* ============================================================================
 * CONSTRAINT OPERATIONS
 * ============================================================================ */

bool rtka_place_digit(sudoku_729_t* puzzle, uint32_t cell, uint32_t digit) {
    /* Check if placement is valid */
    if (!bit_set(puzzle->possible[cell], digit)) {
        return false;  /* Digit not possible here */
    }
    
    /* Get position info */
    uint32_t row = cell / 9;
    uint32_t col = cell % 9;
    uint32_t box = get_box(row, col);
    
    /* Double-check: ensure digit not already in row/col/box */
    if (bit_set(puzzle->row_used[row], digit) ||
        bit_set(puzzle->col_used[col], digit) ||
        bit_set(puzzle->box_used[box], digit)) {
        return false;  /* Digit already used in constraint group */
    }
    
    /* Set this digit as TRUE, all others as FALSE for this cell */
    for (uint32_t d = 0; d < 9; d++) {
        if (d == digit) {
            puzzle->cell_digit[cell][d] = (rtka_state_t){
                .value = RTKA_TRUE,
                .confidence = 1.0f
            };
        } else {
            puzzle->cell_digit[cell][d] = (rtka_state_t){
                .value = RTKA_FALSE,
                .confidence = 1.0f
            };
        }
    }
    
    /* Update solution and masks */
    puzzle->solution[cell] = digit;
    puzzle->possible[cell] = 1 << digit;  /* Only this digit possible */
    puzzle->cell_confidence[cell] = 1.0f;
    puzzle->filled_count++;
    
    /* Update row/col/box masks */
    puzzle->row_used[row] = set_bit(puzzle->row_used[row], digit);
    puzzle->col_used[col] = set_bit(puzzle->col_used[col], digit);
    puzzle->box_used[box] = set_bit(puzzle->box_used[box], digit);
    
    /* Propagate: eliminate this digit from all peers */
    peer_list_t peers = get_peers(cell);
    for (uint32_t i = 0; i < peers.count; i++) {
        if (!rtka_eliminate_digit(puzzle, peers.cells[i], digit)) {
            return false;  /* Contradiction */
        }
    }
    
    return true;
}

bool rtka_eliminate_digit(sudoku_729_t* puzzle, uint32_t cell, uint32_t digit) {
    /* If already eliminated, nothing to do */
    if (!bit_set(puzzle->possible[cell], digit)) {
        return true;
    }
    
    /* If cell already filled, nothing to do */
    if (puzzle->solution[cell] != 0xFF) {
        return true;
    }
    
    /* Eliminate the digit */
    puzzle->possible[cell] = clear_bit(puzzle->possible[cell], digit);
    puzzle->cell_digit[cell][digit] = (rtka_state_t){
        .value = RTKA_FALSE,
        .confidence = 1.0f
    };
    
    /* Check if we've eliminated all possibilities (contradiction) */
    if (puzzle->possible[cell] == 0) {
        return false;
    }
    
    /* If only one possibility remains, place it */
    int32_t single = get_single_bit(puzzle->possible[cell]);
    if (single >= 0) {
        return rtka_place_digit(puzzle, cell, single);
    }
    
    /* Update confidence based on remaining possibilities */
    uint32_t remaining = count_bits(puzzle->possible[cell]);
    puzzle->cell_confidence[cell] = 1.0f - (1.0f / remaining);
    
    /* Update UNKNOWN states with new confidence */
    for (uint32_t d = 0; d < 9; d++) {
        if (bit_set(puzzle->possible[cell], d)) {
            puzzle->cell_digit[cell][d].confidence = 1.0f / remaining;
        }
    }
    
    return true;
}

/* ============================================================================
 * NAKED SINGLES: Cells with only one possibility
 * ============================================================================ */

bool rtka_find_naked_singles(sudoku_729_t* puzzle) {
    bool progress = false;
    
    for (uint32_t cell = 0; cell < 81; cell++) {
        if (puzzle->solution[cell] != 0xFF) continue;  /* Already filled */
        
        uint32_t count = count_bits(puzzle->possible[cell]);
        if (count == 1) {
            int32_t digit = get_single_bit(puzzle->possible[cell]);
            if (digit >= 0) {
                /* RTKA: UNKNOWN -> TRUE transition */
                if (rtka_place_digit(puzzle, cell, digit)) {
                    progress = true;
                }
            }
        }
    }
    
    return progress;
}

/* ============================================================================
 * HIDDEN SINGLES: Digits that can only go in one cell in a group
 * ============================================================================ */

bool rtka_find_hidden_singles(sudoku_729_t* puzzle) {
    bool progress = false;
    
    /* Check rows */
    for (uint32_t row = 0; row < 9; row++) {
        for (uint32_t digit = 0; digit < 9; digit++) {
            if (bit_set(puzzle->row_used[row], digit)) continue;
            
            uint32_t possible_cells = 0;
            uint32_t last_cell = 0;
            
            for (uint32_t col = 0; col < 9; col++) {
                uint32_t cell = row * 9 + col;
                if (bit_set(puzzle->possible[cell], digit)) {
                    possible_cells++;
                    last_cell = cell;
                }
            }
            
            if (possible_cells == 1) {
                /* Hidden single found */
                if (rtka_place_digit(puzzle, last_cell, digit)) {
                    progress = true;
                }
            }
        }
    }
    
    /* Check columns */
    for (uint32_t col = 0; col < 9; col++) {
        for (uint32_t digit = 0; digit < 9; digit++) {
            if (bit_set(puzzle->col_used[col], digit)) continue;
            
            uint32_t possible_cells = 0;
            uint32_t last_cell = 0;
            
            for (uint32_t row = 0; row < 9; row++) {
                uint32_t cell = row * 9 + col;
                if (bit_set(puzzle->possible[cell], digit)) {
                    possible_cells++;
                    last_cell = cell;
                }
            }
            
            if (possible_cells == 1) {
                if (rtka_place_digit(puzzle, last_cell, digit)) {
                    progress = true;
                }
            }
        }
    }
    
    /* Check boxes */
    for (uint32_t box = 0; box < 9; box++) {
        for (uint32_t digit = 0; digit < 9; digit++) {
            if (bit_set(puzzle->box_used[box], digit)) continue;
            
            uint32_t possible_cells = 0;
            uint32_t last_cell = 0;
            
            uint32_t box_row = (box / 3) * 3;
            uint32_t box_col = (box % 3) * 3;
            
            for (uint32_t r = box_row; r < box_row + 3; r++) {
                for (uint32_t c = box_col; c < box_col + 3; c++) {
                    uint32_t cell = r * 9 + c;
                    if (bit_set(puzzle->possible[cell], digit)) {
                        possible_cells++;
                        last_cell = cell;
                    }
                }
            }
            
            if (possible_cells == 1) {
                if (rtka_place_digit(puzzle, last_cell, digit)) {
                    progress = true;
                }
            }
        }
    }
    
    return progress;
}

/* ============================================================================
 * MAIN CONSTRAINT PROPAGATION
 * ============================================================================ */

bool rtka_propagate_constraints(sudoku_729_t* puzzle) {
    bool progress = true;
    
    while (progress) {
        progress = false;
        
        /* Apply naked singles rule */
        if (rtka_find_naked_singles(puzzle)) {
            progress = true;
        }
        
        /* Apply hidden singles rule */
        if (rtka_find_hidden_singles(puzzle)) {
            progress = true;
        }
    }
    
    return puzzle->filled_count == 81;  /* Solved if all cells filled */
}

/* ============================================================================
 * LOGICAL SOLVER (No Guessing)
 * ============================================================================ */

bool rtka_solve_logic(sudoku_729_t* puzzle) {
    return rtka_propagate_constraints(puzzle);
}

/* ============================================================================
 * RECURSIVE SOLVER (With Backtracking if Needed)
 * ============================================================================ */

uint32_t rtka_find_best_guess(sudoku_729_t* puzzle) {
    uint32_t best_cell = 0xFF;
    uint32_t min_choices = 10;
    
    for (uint32_t cell = 0; cell < 81; cell++) {
        if (puzzle->solution[cell] != 0xFF) continue;
        
        uint32_t choices = count_bits(puzzle->possible[cell]);
        if (choices < min_choices) {
            min_choices = choices;
            best_cell = cell;
            if (choices == 2) break;  /* Can't do better than 2 */
        }
    }
    
    return best_cell;
}

bool rtka_solve_recursive(sudoku_729_t* puzzle) {
    /* Try constraint propagation first */
    if (rtka_propagate_constraints(puzzle)) {
        return true;  /* Solved! */
    }
    
    /* Check if we're actually done */
    if (puzzle->filled_count == 81) {
        return rtka_validate_solution(puzzle);
    }
    
    /* Check for contradiction */
    for (uint32_t cell = 0; cell < 81; cell++) {
        if (puzzle->solution[cell] == 0xFF && puzzle->possible[cell] == 0) {
            return false;  /* No possibilities = contradiction */
        }
    }
    
    /* Find best cell to guess (minimum remaining values) */
    uint32_t guess_cell = rtka_find_best_guess(puzzle);
    if (guess_cell == 0xFF) {
        return puzzle->filled_count == 81 && rtka_validate_solution(puzzle);
    }
    
    /* Try each possible digit */
    uint16_t possible = puzzle->possible[guess_cell];
    for (uint32_t digit = 0; digit < 9; digit++) {
        if (!bit_set(possible, digit)) continue;
        
        /* Make a deep copy for backtracking */
        sudoku_729_t* backup = malloc(sizeof(sudoku_729_t));
        memcpy(backup, puzzle, sizeof(sudoku_729_t));
        
        /* Try placing this digit */
        if (rtka_place_digit(puzzle, guess_cell, digit)) {
            if (rtka_solve_recursive(puzzle)) {
                free(backup);
                return true;  /* Solution found! */
            }
        }
        
        /* Backtrack - restore complete state */
        memcpy(puzzle, backup, sizeof(sudoku_729_t));
        free(backup);
    }
    
    return false;  /* No solution */
}

/* ============================================================================
 * RTKA CONSTRAINT COMBINATION
 * ============================================================================ */

rtka_state_t rtka_combine_constraints(
    rtka_state_t row_constraint,
    rtka_state_t col_constraint,
    rtka_state_t box_constraint
) {
    /* All three constraints must be satisfied */
    rtka_value_t combined = rtka_and(
        rtka_and(row_constraint.value, col_constraint.value),
        box_constraint.value
    );
    
    /* Confidence is product of all three */
    rtka_confidence_t confidence = rtka_conf_and(
        rtka_conf_and(row_constraint.confidence, col_constraint.confidence),
        box_constraint.confidence
    );
    
    return (rtka_state_t){.value = combined, .confidence = confidence};
}

/* ============================================================================
 * DISPLAY FUNCTIONS
 * ============================================================================ */

void rtka_print_grid(uint8_t grid[9][9]) {
    for (int r = 0; r < 9; r++) {
        if (r % 3 == 0) printf("+-------+-------+-------+\n");
        for (int c = 0; c < 9; c++) {
            if (c % 3 == 0) printf("| ");
            printf("%d ", grid[r][c]);
        }
        printf("|\n");
    }
    printf("+-------+-------+-------+\n");
}

void rtka_print_puzzle(sudoku_729_t* puzzle) {
    printf("+-------+-------+-------+\n");
    for (uint32_t row = 0; row < 9; row++) {
        if (row % 3 == 0 && row != 0) {
            printf("+-------+-------+-------+\n");
        }
        
        for (uint32_t col = 0; col < 9; col++) {
            if (col % 3 == 0) printf("| ");
            
            uint32_t cell = row * 9 + col;
            if (puzzle->solution[cell] != 0xFF) {
                printf("%d ", puzzle->solution[cell] + 1);
            } else {
                printf(". ");
            }
        }
        printf("|\n");
    }
    printf("+-------+-------+-------+\n");
}

void rtka_print_possibilities(sudoku_729_t* puzzle) {
    printf("\nCell Possibilities (RTKA States):\n");
    for (uint32_t cell = 0; cell < 81; cell++) {
        if (puzzle->solution[cell] != 0xFF) continue;
        
        uint32_t count = count_bits(puzzle->possible[cell]);
        if (count > 0 && count < 9) {
            printf("Cell (%d,%d): ", cell/9, cell%9);
            for (uint32_t d = 0; d < 9; d++) {
                if (bit_set(puzzle->possible[cell], d)) {
                    rtka_state_t state = puzzle->cell_digit[cell][d];
                    char symbol = (state.value == RTKA_TRUE) ? '!' :
                                  (state.value == RTKA_UNKNOWN) ? '?' : 'x';
                    printf("%d%c ", d+1, symbol);
                }
            }
            printf("(conf=%.2f)\n", puzzle->cell_confidence[cell]);
        }
    }
}

bool rtka_validate_solution(sudoku_729_t* puzzle) {
    if (puzzle->filled_count != 81) return false;
    
    /* Check rows */
    for (uint32_t row = 0; row < 9; row++) {
        uint16_t seen = 0;
        for (uint32_t col = 0; col < 9; col++) {
            uint32_t cell = row * 9 + col;
            uint32_t digit = puzzle->solution[cell];
            if (digit == 0xFF || bit_set(seen, digit)) return false;
            seen = set_bit(seen, digit);
        }
    }
    
    /* Check columns */
    for (uint32_t col = 0; col < 9; col++) {
        uint16_t seen = 0;
        for (uint32_t row = 0; row < 9; row++) {
            uint32_t cell = row * 9 + col;
            uint32_t digit = puzzle->solution[cell];
            if (digit == 0xFF || bit_set(seen, digit)) return false;
            seen = set_bit(seen, digit);
        }
    }
    
    /* Check boxes */
    for (uint32_t box = 0; box < 9; box++) {
        uint16_t seen = 0;
        uint32_t box_row = (box / 3) * 3;
        uint32_t box_col = (box % 3) * 3;
        
        for (uint32_t r = box_row; r < box_row + 3; r++) {
            for (uint32_t c = box_col; c < box_col + 3; c++) {
                uint32_t cell = r * 9 + c;
                uint32_t digit = puzzle->solution[cell];
                if (digit == 0xFF || bit_set(seen, digit)) return false;
                seen = set_bit(seen, digit);
            }
        }
    }
    
    return true;
}
