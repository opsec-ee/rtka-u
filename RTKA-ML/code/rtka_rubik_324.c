/**
 * File: rtka_rubik_324.c
 * Copyright (c) 2025 - H.Overman opsec.ee@pm.me
 * 
 * PROPRIETARY AND CONFIDENTIAL
 * This file is proprietary to H. Overman and may not be reproduced,
 * distributed, or used without explicit written permission.
 *
 * For licensing inquiries: opsec.ee@pm.me
 *
 * RTKA 324-State Rubik's Cube Solver
 * Bit-based constraint propagation inspired by 729-state Sudoku
 * 
 * CHANGELOG:
 * v1.1.0 - 2025-09-28
 *   - Completed all move permutations (U, D, F, B, L, R)
 *   - Fixed character encoding issues
 *   - Enhanced constraint propagation with corner triplets
 *   - Added proper bit rotation for all 6 basic moves
 *   - Improved heuristic calculation
 */

#include "rtka_rubik_324.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <stdint.h>

/* ============================================================================
 * 324-STATE BIT REPRESENTATION
 * 54 stickers × 6 colors = 324 ternary states
 * Unwrapped cube layout:
 *       [UP]
 * [LEFT][FRONT][RIGHT][BACK]
 *       [DOWN]
 * ============================================================================ */

/* Bit operations for 324 states (11 × 32-bit words) */
#define RUBIK_BIT_WORDS ((324 + 31) / 32)
typedef uint32_t rubik_bitset_t[RUBIK_BIT_WORDS];

/* Face indices in unwrapped layout */
#define FACE_UP    0
#define FACE_LEFT  1
#define FACE_FRONT 2
#define FACE_RIGHT 3
#define FACE_BACK  4
#define FACE_DOWN  5

/* Sticker indices for unwrapped cube (0-53) */
static const uint8_t face_offset[6] = {0, 9, 18, 27, 36, 45};

/* Edge connections in unwrapped form */
typedef struct {
    uint8_t sticker1;
    uint8_t sticker2;
} edge_pair_t;

/* 24 edge connections (12 edges × 2 stickers per edge) */
static const edge_pair_t edge_pairs[24] = {
    /* UP-FRONT edges */  {6, 18}, {7, 19}, {8, 20},
    /* UP-RIGHT edges */  {2, 27}, {5, 30}, {8, 33},
    /* UP-BACK edges */   {0, 36}, {1, 37}, {2, 38},
    /* UP-LEFT edges */   {0, 9},  {3, 12}, {6, 15},
    /* DOWN-FRONT edges */{24, 45}, {25, 46}, {26, 47},
    /* DOWN-RIGHT edges */{47, 29}, {50, 32}, {53, 35},
    /* DOWN-BACK edges */ {45, 38}, {48, 41}, {51, 44},
    /* DOWN-LEFT edges */ {51, 11}, {52, 14}, {53, 17}
};

/* Corner connections (8 corners × 3 stickers each) */
static const uint8_t corner_triplets[8][3] = {
    {0, 9, 38},   /* UP-LEFT-BACK */
    {2, 36, 27},  /* UP-BACK-RIGHT */
    {6, 15, 18},  /* UP-FRONT-LEFT */
    {8, 20, 33},  /* UP-RIGHT-FRONT */
    {45, 11, 24}, /* DOWN-LEFT-FRONT */
    {47, 26, 29}, /* DOWN-FRONT-RIGHT */
    {51, 35, 44}, /* DOWN-RIGHT-BACK */
    {53, 42, 17}  /* DOWN-BACK-LEFT */
};

/* 324-State Rubik's Cube */
struct rubik_324_state {
    rubik_bitset_t true_states;     /* Sticker-color pairs that ARE here */
    rubik_bitset_t false_states;    /* Sticker-color pairs that CANNOT be here */
    rubik_bitset_t unknown_states;  /* Sticker-color pairs that MIGHT be here */
    rtka_confidence_t confidence;
    uint32_t moves;
    uint32_t rtka_transitions;
};

/* Bit manipulation */
static inline void set_bit(rubik_bitset_t bits, uint16_t pos) {
    bits[pos / 32] |= (1U << (pos % 32));
}

static inline void clear_bit(rubik_bitset_t bits, uint16_t pos) {
    bits[pos / 32] &= ~(1U << (pos % 32));
}

static inline bool test_bit(const rubik_bitset_t bits, uint16_t pos) {
    return (bits[pos / 32] & (1U << (pos % 32))) != 0;
}

static inline uint16_t state_index(uint8_t sticker, uint8_t color) {
    return sticker * 6 + color;
}

/* Count set bits */
static uint32_t count_bits(const rubik_bitset_t bits) {
    uint32_t count = 0;
    for (int i = 0; i < RUBIK_BIT_WORDS; i++) {
        count += __builtin_popcount(bits[i]);
    }
    return count;
}

/* Create solved cube */
rubik_324_state_t* rtka_rubik_324_create_solved(void) {
    rubik_324_state_t* cube = calloc(1, sizeof(rubik_324_state_t));
    
    /* Set TRUE states for solved configuration */
    for (int face = 0; face < 6; face++) {
        uint8_t color = face;  /* Face index = color in solved state */
        for (int i = 0; i < 9; i++) {
            uint8_t sticker = face_offset[face] + i;
            uint16_t idx = state_index(sticker, color);
            set_bit(cube->true_states, idx);
        }
    }
    
    /* Set FALSE states for impossible positions */
    for (int sticker = 0; sticker < 54; sticker++) {
        uint8_t face = sticker / 9;
        uint8_t correct_color = face;
        for (uint8_t color = 0; color < 6; color++) {
            if (color != correct_color) {
                uint16_t idx = state_index(sticker, color);
                set_bit(cube->false_states, idx);
            }
        }
    }
    
    cube->confidence = 1.0f;
    return cube;
}

/* Propagate constraints using ternary logic */
static void propagate_constraints(rubik_324_state_t* cube) {
    bool changed;
    do {
        changed = false;
        cube->rtka_transitions++;
        
        /* Face constraints: each face must have one of each color */
        for (int face = 0; face < 6; face++) {
            for (uint8_t color = 0; color < 6; color++) {
                uint32_t true_count = 0;
                uint32_t possible_positions = 0;
                int last_possible = -1;
                
                for (int i = 0; i < 9; i++) {
                    uint8_t sticker = face_offset[face] + i;
                    uint16_t idx = state_index(sticker, color);
                    
                    if (test_bit(cube->true_states, idx)) {
                        true_count++;
                    } else if (!test_bit(cube->false_states, idx)) {
                        possible_positions++;
                        last_possible = sticker;
                    }
                }
                
                /* If only one position possible, it must be TRUE */
                if (true_count == 0 && possible_positions == 1) {
                    uint16_t idx = state_index(last_possible, color);
                    set_bit(cube->true_states, idx);
                    clear_bit(cube->unknown_states, idx);
                    changed = true;
                    
                    /* Mark other colors as FALSE for this sticker */
                    for (uint8_t c = 0; c < 6; c++) {
                        if (c != color) {
                            uint16_t idx2 = state_index(last_possible, c);
                            if (!test_bit(cube->false_states, idx2)) {
                                set_bit(cube->false_states, idx2);
                                clear_bit(cube->unknown_states, idx2);
                                changed = true;
                            }
                        }
                    }
                }
                
                /* If color found, mark other positions as FALSE */
                if (true_count == 1) {
                    for (int i = 0; i < 9; i++) {
                        uint8_t sticker = face_offset[face] + i;
                        uint16_t idx = state_index(sticker, color);
                        if (!test_bit(cube->true_states, idx) && 
                            !test_bit(cube->false_states, idx)) {
                            set_bit(cube->false_states, idx);
                            clear_bit(cube->unknown_states, idx);
                            changed = true;
                        }
                    }
                }
            }
        }
        
        /* Edge constraints: connected stickers must be consistent */
        for (int e = 0; e < 24; e++) {
            uint8_t s1 = edge_pairs[e].sticker1;
            uint8_t s2 = edge_pairs[e].sticker2;
            
            /* Physical edge pieces connect two faces */
            /* If we know one sticker's color, we can deduce constraints on the other */
            for (uint8_t c1 = 0; c1 < 6; c1++) {
                uint16_t idx1 = state_index(s1, c1);
                if (test_bit(cube->true_states, idx1)) {
                    /* This edge piece exists - constrain the connected sticker */
                    /* Edge pieces have specific color pairings in valid cubes */
                    for (uint8_t c2 = 0; c2 < 6; c2++) {
                        uint16_t idx2 = state_index(s2, c2);
                        /* Simple constraint: adjacent faces can't have same color */
                        if (c1 == c2 && !test_bit(cube->false_states, idx2)) {
                            set_bit(cube->false_states, idx2);
                            clear_bit(cube->unknown_states, idx2);
                            changed = true;
                        }
                    }
                }
            }
        }
        
        /* Corner constraints: triplets must be consistent */
        for (int c = 0; c < 8; c++) {
            uint8_t s1 = corner_triplets[c][0];
            uint8_t s2 = corner_triplets[c][1];
            uint8_t s3 = corner_triplets[c][2];
            
            /* Corner pieces connect three faces */
            for (uint8_t color = 0; color < 6; color++) {
                uint16_t idx1 = state_index(s1, color);
                uint16_t idx2 = state_index(s2, color);
                uint16_t idx3 = state_index(s3, color);
                
                /* If one is TRUE, others can't be same color */
                if (test_bit(cube->true_states, idx1)) {
                    if (!test_bit(cube->false_states, idx2)) {
                        set_bit(cube->false_states, idx2);
                        clear_bit(cube->unknown_states, idx2);
                        changed = true;
                    }
                    if (!test_bit(cube->false_states, idx3)) {
                        set_bit(cube->false_states, idx3);
                        clear_bit(cube->unknown_states, idx3);
                        changed = true;
                    }
                }
            }
        }
        
    } while (changed);
}

/* Apply rotation as bit permutation */
void rtka_rubik_324_rotate(rubik_324_state_t* cube, rubik_324_move_t move) {
    rubik_bitset_t new_true = {0};
    rubik_bitset_t new_false = {0};
    rubik_bitset_t new_unknown = {0};
    
    /* Copy unchanged stickers first */
    memcpy(new_true, cube->true_states, sizeof(rubik_bitset_t));
    memcpy(new_false, cube->false_states, sizeof(rubik_bitset_t));
    memcpy(new_unknown, cube->unknown_states, sizeof(rubik_bitset_t));
    
    /* Define rotation permutations for each move */
    /* Format: from_sticker, to_sticker pairs */
    
    if (move == MOVE_324_U) {
        /* UP face clockwise rotation */
        static const uint8_t perm_U[] = {
            /* Face rotation (UP face indices 0-8) */
            0,2, 1,5, 2,8, 3,1, 4,4, 5,7, 6,0, 7,3, 8,6,
            /* Edge cycle: Front->Left->Back->Right->Front */
            18,9, 19,10, 20,11,  /* Front top to Left top */
            9,36, 10,37, 11,38,  /* Left top to Back top */
            36,27, 37,28, 38,29, /* Back top to Right top */
            27,18, 28,19, 29,20  /* Right top to Front top */
        };
        
        /* Clear affected positions */
        for (int i = 0; i < sizeof(perm_U)/sizeof(perm_U[0]); i += 2) {
            uint8_t from = perm_U[i];
            for (uint8_t color = 0; color < 6; color++) {
                uint16_t idx = state_index(from, color);
                clear_bit(new_true, idx);
                clear_bit(new_false, idx);
                clear_bit(new_unknown, idx);
            }
        }
        
        /* Apply permutation */
        for (int i = 0; i < sizeof(perm_U)/sizeof(perm_U[0]); i += 2) {
            uint8_t from = perm_U[i];
            uint8_t to = perm_U[i + 1];
            
            for (uint8_t color = 0; color < 6; color++) {
                uint16_t from_idx = state_index(from, color);
                uint16_t to_idx = state_index(to, color);
                
                if (test_bit(cube->true_states, from_idx))
                    set_bit(new_true, to_idx);
                if (test_bit(cube->false_states, from_idx))
                    set_bit(new_false, to_idx);
                if (test_bit(cube->unknown_states, from_idx))
                    set_bit(new_unknown, to_idx);
            }
        }
    }
    else if (move == MOVE_324_R) {
        /* RIGHT face clockwise rotation */
        static const uint8_t perm_R[] = {
            /* Face rotation (RIGHT face indices 27-35) */
            27,29, 28,32, 29,35, 30,28, 31,31, 32,34, 33,27, 34,30, 35,33,
            /* Edge cycle: Front->Up->Back->Down->Front */
            20,2, 23,5, 26,8,    /* Front right to Up right */
            2,38, 5,41, 8,44,    /* Up right to Back left */
            38,47, 41,50, 44,53, /* Back left to Down right */
            47,20, 50,23, 53,26  /* Down right to Front right */
        };
        
        /* Clear and apply similar to U move */
        for (int i = 0; i < sizeof(perm_R)/sizeof(perm_R[0]); i += 2) {
            uint8_t from = perm_R[i];
            for (uint8_t color = 0; color < 6; color++) {
                uint16_t idx = state_index(from, color);
                clear_bit(new_true, idx);
                clear_bit(new_false, idx);
                clear_bit(new_unknown, idx);
            }
        }
        
        for (int i = 0; i < sizeof(perm_R)/sizeof(perm_R[0]); i += 2) {
            uint8_t from = perm_R[i];
            uint8_t to = perm_R[i + 1];
            
            for (uint8_t color = 0; color < 6; color++) {
                uint16_t from_idx = state_index(from, color);
                uint16_t to_idx = state_index(to, color);
                
                if (test_bit(cube->true_states, from_idx))
                    set_bit(new_true, to_idx);
                if (test_bit(cube->false_states, from_idx))
                    set_bit(new_false, to_idx);
                if (test_bit(cube->unknown_states, from_idx))
                    set_bit(new_unknown, to_idx);
            }
        }
    }
    else if (move == MOVE_324_F) {
        /* FRONT face clockwise rotation */
        static const uint8_t perm_F[] = {
            /* Face rotation (FRONT face indices 18-26) */
            18,20, 19,23, 20,26, 21,19, 22,22, 23,25, 24,18, 25,21, 26,24,
            /* Edge cycle: Up->Right->Down->Left->Up */
            6,27, 7,30, 8,33,    /* Up bottom to Right left */
            27,47, 30,46, 33,45, /* Right left to Down top */
            47,17, 46,14, 45,11, /* Down top to Left right */
            17,6, 14,7, 11,8     /* Left right to Up bottom */
        };
        
        for (int i = 0; i < sizeof(perm_F)/sizeof(perm_F[0]); i += 2) {
            uint8_t from = perm_F[i];
            for (uint8_t color = 0; color < 6; color++) {
                uint16_t idx = state_index(from, color);
                clear_bit(new_true, idx);
                clear_bit(new_false, idx);
                clear_bit(new_unknown, idx);
            }
        }
        
        for (int i = 0; i < sizeof(perm_F)/sizeof(perm_F[0]); i += 2) {
            uint8_t from = perm_F[i];
            uint8_t to = perm_F[i + 1];
            
            for (uint8_t color = 0; color < 6; color++) {
                uint16_t from_idx = state_index(from, color);
                uint16_t to_idx = state_index(to, color);
                
                if (test_bit(cube->true_states, from_idx))
                    set_bit(new_true, to_idx);
                if (test_bit(cube->false_states, from_idx))
                    set_bit(new_false, to_idx);
                if (test_bit(cube->unknown_states, from_idx))
                    set_bit(new_unknown, to_idx);
            }
        }
    }
    else if (move == MOVE_324_B) {
        /* BACK face clockwise rotation */
        static const uint8_t perm_B[] = {
            /* Face rotation (BACK face indices 36-44) */
            36,38, 37,41, 38,44, 39,37, 40,40, 41,43, 42,36, 43,39, 44,42,
            /* Edge cycle: Up->Left->Down->Right->Up */
            0,29, 1,32, 2,35,    /* Up top to Right right */
            29,53, 32,52, 35,51, /* Right right to Down bottom */
            53,9, 52,12, 51,15,  /* Down bottom to Left left */
            9,0, 12,1, 15,2      /* Left left to Up top */
        };
        
        for (int i = 0; i < sizeof(perm_B)/sizeof(perm_B[0]); i += 2) {
            uint8_t from = perm_B[i];
            for (uint8_t color = 0; color < 6; color++) {
                uint16_t idx = state_index(from, color);
                clear_bit(new_true, idx);
                clear_bit(new_false, idx);
                clear_bit(new_unknown, idx);
            }
        }
        
        for (int i = 0; i < sizeof(perm_B)/sizeof(perm_B[0]); i += 2) {
            uint8_t from = perm_B[i];
            uint8_t to = perm_B[i + 1];
            
            for (uint8_t color = 0; color < 6; color++) {
                uint16_t from_idx = state_index(from, color);
                uint16_t to_idx = state_index(to, color);
                
                if (test_bit(cube->true_states, from_idx))
                    set_bit(new_true, to_idx);
                if (test_bit(cube->false_states, from_idx))
                    set_bit(new_false, to_idx);
                if (test_bit(cube->unknown_states, from_idx))
                    set_bit(new_unknown, to_idx);
            }
        }
    }
    else if (move == MOVE_324_L) {
        /* LEFT face clockwise rotation */
        static const uint8_t perm_L[] = {
            /* Face rotation (LEFT face indices 9-17) */
            9,11, 10,14, 11,17, 12,10, 13,13, 14,16, 15,9, 16,12, 17,15,
            /* Edge cycle: Front->Down->Back->Up->Front */
            18,45, 21,48, 24,51, /* Front left to Down left */
            45,44, 48,41, 51,38, /* Down left to Back right */
            44,6, 41,3, 38,0,    /* Back right to Up left */
            6,18, 3,21, 0,24     /* Up left to Front left */
        };
        
        for (int i = 0; i < sizeof(perm_L)/sizeof(perm_L[0]); i += 2) {
            uint8_t from = perm_L[i];
            for (uint8_t color = 0; color < 6; color++) {
                uint16_t idx = state_index(from, color);
                clear_bit(new_true, idx);
                clear_bit(new_false, idx);
                clear_bit(new_unknown, idx);
            }
        }
        
        for (int i = 0; i < sizeof(perm_L)/sizeof(perm_L[0]); i += 2) {
            uint8_t from = perm_L[i];
            uint8_t to = perm_L[i + 1];
            
            for (uint8_t color = 0; color < 6; color++) {
                uint16_t from_idx = state_index(from, color);
                uint16_t to_idx = state_index(to, color);
                
                if (test_bit(cube->true_states, from_idx))
                    set_bit(new_true, to_idx);
                if (test_bit(cube->false_states, from_idx))
                    set_bit(new_false, to_idx);
                if (test_bit(cube->unknown_states, from_idx))
                    set_bit(new_unknown, to_idx);
            }
        }
    }
    else if (move == MOVE_324_D) {
        /* DOWN face clockwise rotation */
        static const uint8_t perm_D[] = {
            /* Face rotation (DOWN face indices 45-53) */
            45,47, 46,50, 47,53, 48,46, 49,49, 50,52, 51,45, 52,48, 53,51,
            /* Edge cycle: Front->Right->Back->Left->Front */
            24,33, 25,34, 26,35, /* Front bottom to Right bottom */
            33,42, 34,43, 35,44, /* Right bottom to Back bottom */
            42,15, 43,16, 44,17, /* Back bottom to Left bottom */
            15,24, 16,25, 17,26  /* Left bottom to Front bottom */
        };
        
        for (int i = 0; i < sizeof(perm_D)/sizeof(perm_D[0]); i += 2) {
            uint8_t from = perm_D[i];
            for (uint8_t color = 0; color < 6; color++) {
                uint16_t idx = state_index(from, color);
                clear_bit(new_true, idx);
                clear_bit(new_false, idx);
                clear_bit(new_unknown, idx);
            }
        }
        
        for (int i = 0; i < sizeof(perm_D)/sizeof(perm_D[0]); i += 2) {
            uint8_t from = perm_D[i];
            uint8_t to = perm_D[i + 1];
            
            for (uint8_t color = 0; color < 6; color++) {
                uint16_t from_idx = state_index(from, color);
                uint16_t to_idx = state_index(to, color);
                
                if (test_bit(cube->true_states, from_idx))
                    set_bit(new_true, to_idx);
                if (test_bit(cube->false_states, from_idx))
                    set_bit(new_false, to_idx);
                if (test_bit(cube->unknown_states, from_idx))
                    set_bit(new_unknown, to_idx);
            }
        }
    }
    
    /* Update cube state */
    memcpy(cube->true_states, new_true, sizeof(rubik_bitset_t));
    memcpy(cube->false_states, new_false, sizeof(rubik_bitset_t));
    memcpy(cube->unknown_states, new_unknown, sizeof(rubik_bitset_t));
    
    cube->moves++;
    cube->confidence *= 0.95f;  /* Decay confidence with moves */
    
    /* Propagate constraints after move */
    propagate_constraints(cube);
}

/* Check if solved using ternary logic */
bool rtka_rubik_324_is_solved(const rubik_324_state_t* cube) {
    /* Check each face has uniform color */
    for (int face = 0; face < 6; face++) {
        uint8_t expected_color = face;  /* In solved state, face index = color */
        
        for (int i = 0; i < 9; i++) {
            uint8_t sticker = face_offset[face] + i;
            uint16_t idx = state_index(sticker, expected_color);
            
            if (!test_bit(cube->true_states, idx)) {
                return false;
            }
        }
    }
    
    return count_bits(cube->unknown_states) == 0;
}

/* Heuristic using ternary state counts */
rtka_confidence_t rtka_rubik_324_heuristic(const rubik_324_state_t* cube) {
    uint32_t correct = 0;
    
    for (int face = 0; face < 6; face++) {
        uint8_t expected_color = face;
        for (int i = 0; i < 9; i++) {
            uint8_t sticker = face_offset[face] + i;
            uint16_t idx = state_index(sticker, expected_color);
            if (test_bit(cube->true_states, idx)) {
                correct++;
            }
        }
    }
    
    /* Also factor in constraint satisfaction */
    float constraint_score = (float)(270 - count_bits(cube->false_states)) / 270.0f;
    float position_score = (float)correct / 54.0f;
    
    return (position_score * 0.7f + constraint_score * 0.3f);
}

/* Print state analysis */
void rtka_rubik_324_print_analysis(const rubik_324_state_t* cube) {
    printf("\n┌─── RTKA 324-State Analysis ────────────┐\n");
    printf("│ Total States: 324 (54 stickers × 6)    │\n");
    printf("│ TRUE states:    %3d (must be here)     │\n", count_bits(cube->true_states));
    printf("│ FALSE states:   %3d (cannot be here)   │\n", count_bits(cube->false_states));
    printf("│ UNKNOWN states: %3d (might be here)    │\n", count_bits(cube->unknown_states));
    printf("│                                        │\n");
    printf("│ Constraint Groups:                     │\n");
    printf("│ - 6 face constraints (9 stickers each) │\n");
    printf("│ - 24 edge pairs (connected stickers)   │\n");
    printf("│ - 8 corner triplets (3-way connected)  │\n");
    printf("│                                        │\n");
    printf("│ Moves: %d, Confidence: %.4f         │\n", cube->moves, cube->confidence);
    printf("│ RTKA transitions: %d                   │\n", cube->rtka_transitions);
    printf("└────────────────────────────────────────┘\n");
}

/* IDA* with constraint propagation */
bool rtka_rubik_324_solve(rubik_324_state_t* cube, uint32_t max_depth) {
    if (rtka_rubik_324_is_solved(cube)) return true;
    
    if (max_depth == 0) return false;
    
    /* Try each move with constraint propagation */
    for (rubik_324_move_t move = 0; move < MOVE_324_COUNT; move++) {
        /* Save state */
        rubik_324_state_t saved;
        memcpy(&saved, cube, sizeof(rubik_324_state_t));
        
        /* Apply move and propagate */
        rtka_rubik_324_rotate(cube, move);
        
        /* Check if constraints lead to solution */
        if (count_bits(cube->false_states) > 270) {
            /* Too many impossibilities - bad path */
            memcpy(cube, &saved, sizeof(rubik_324_state_t));
            continue;
        }
        
        /* Use heuristic to prune */
        if (rtka_rubik_324_heuristic(cube) < 0.2f) {
            /* Too far from solution */
            memcpy(cube, &saved, sizeof(rubik_324_state_t));
            continue;
        }
        
        /* Recursive search */
        if (rtka_rubik_324_solve(cube, max_depth - 1)) {
            return true;
        }
        
        /* Restore state */
        memcpy(cube, &saved, sizeof(rubik_324_state_t));
    }
    
    return false;
}

/* Get transition count */
uint32_t rtka_rubik_324_get_transitions(const rubik_324_state_t* cube) {
    return cube->rtka_transitions;
}

/* Get move count */
uint32_t rtka_rubik_324_get_moves(const rubik_324_state_t* cube) {
    return cube->moves;
}

/* Free cube */
void rtka_rubik_324_free(rubik_324_state_t* cube) {
    free(cube);
}

/* Scramble cube */
void rtka_rubik_324_scramble(rubik_324_state_t* cube, uint32_t moves) {
    for (uint32_t i = 0; i < moves; i++) {
        rubik_324_move_t move = rand() % MOVE_324_COUNT;
        rtka_rubik_324_rotate(cube, move);
    }
}
