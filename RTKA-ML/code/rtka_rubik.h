/**
 * File: rtka_rubik.h
 * Copyright (c) 2025 - H.Overman opsec.ee@pm.me
 * 
 * PROPRIETARY AND CONFIDENTIAL
 * This file is proprietary to H. Overman and may not be reproduced,
 * distributed, or used without explicit written permission.
 *
 * For licensing inquiries: opsec.ee@pm.me
 *
 * RTKA Rubik's Cube Solver - 3D puzzle solving with ternary logic
 * 
 * CHANGELOG:
 * v1.1.0 - 2025-09-28
 *   - Fixed truncated typedef at end of file
 *   - Added missing include guards
 *   - Corrected function signatures for consistency
 */

#ifndef RTKA_RUBIK_H
#define RTKA_RUBIK_H

#include "rtka_u_core.h"
#include "rtka_types.h"
#include <stdbool.h>
#include <stdint.h>

/* Cube dimensions */
#define RUBIK_3X3 3
#define RUBIK_4X4 4  /* Rubik's Revenge */
#define RUBIK_5X5 5  /* Professor's Cube */

/* Simple representation for test compatibility */
#define RUBIK_SIZE 54
#define RUBIK_FACES 6
#define RUBIK_STICKERS_PER_FACE 9

/* Face indices */
typedef enum {
    FACE_FRONT = 0, FACE_BACK = 1,
    FACE_UP = 2,    FACE_DOWN = 3,
    FACE_LEFT = 4,  FACE_RIGHT = 5
} face_t;

/* Colors (6 for standard cube) */
typedef enum {
    WHITE = 0, YELLOW = 1, 
    RED = 2,   ORANGE = 3,
    BLUE = 4,  GREEN = 5
} color_t;

/* Cubie types */
typedef enum {
    CUBIE_CENTER,  /* Fixed position (1 color) */
    CUBIE_EDGE,    /* 2 colors */
    CUBIE_CORNER   /* 3 colors */
} cubie_type_t;

/* Move types */
typedef enum {
    MOVE_U, MOVE_U_PRIME, MOVE_U2,
    MOVE_D, MOVE_D_PRIME, MOVE_D2,
    MOVE_F, MOVE_F_PRIME, MOVE_F2,
    MOVE_B, MOVE_B_PRIME, MOVE_B2,
    MOVE_L, MOVE_L_PRIME, MOVE_L2,
    MOVE_R, MOVE_R_PRIME, MOVE_R2,
    MOVE_COUNT
} rubik_move_t;

/* Single cubie state with ternary evaluation */
typedef struct {
    cubie_type_t type;
    uint8_t colors[3];      /* Up to 3 colors per cubie */
    uint8_t position[3];    /* x,y,z coordinates */
    uint8_t orientation;    /* 0-23 possible orientations */
    rtka_state_t correctness;  /* TRUE=correct, UNKNOWN=close, FALSE=wrong */
} cubie_t;

/* Complete cube state */
typedef struct {
    uint32_t size;          /* 3 for 3x3, 5 for 5x5, etc */
    cubie_t* cubies;        /* All cubies */
    uint32_t num_cubies;
    uint8_t*** faces;       /* [face][row][col] = color */
    rtka_confidence_t solved_confidence;  /* Overall solution confidence */
} rubik_cube_t;

/* Simple state for existing tests */
typedef struct {
    rtka_state_t stickers[RUBIK_SIZE];
    uint32_t depth;
    rubik_move_t last_move;
    rtka_confidence_t heuristic;
} rubik_state_t;

/* Move notation */
typedef struct {
    face_t face;
    int8_t turns;  /* 1=90°, 2=180°, -1=90° counter */
    uint8_t layer; /* 0=outer, 1=middle (for 4x4+) */
} cube_move_t;

/* Solver context */
typedef struct {
    rubik_state_t* current;
    rubik_state_t* goal;
    uint32_t max_depth;
    uint32_t nodes_explored;
    uint32_t rtka_transitions;
    rubik_move_t* solution;
    uint32_t solution_length;
} rubik_solver_t;

/* IDA* with ternary heuristic - Fixed typedef */
typedef struct {
    rtka_state_t* cubie_states;     /* Per-cubie correctness */
    uint32_t moves_so_far;
    rtka_confidence_t heuristic;    /* Distance estimate */
} cube_search_state_t;

/* Full cube operations */
rubik_cube_t* rtka_cube_create(uint32_t size);
void rtka_cube_destroy(rubik_cube_t* cube);
void rtka_cube_scramble(rubik_cube_t* cube, uint32_t moves);
rtka_error_t rtka_cube_rotate(rubik_cube_t* cube, cube_move_t move);
rtka_state_t rtka_cube_evaluate_position(const rubik_cube_t* cube, const cubie_t* cubie);
rtka_confidence_t rtka_cube_solved_confidence(const rubik_cube_t* cube);
cube_move_t* rtka_cube_solve(rubik_cube_t* cube, uint32_t* move_count);
cube_search_state_t* rtka_cube_ida_star(rubik_cube_t* cube, uint32_t max_depth);

/* Simple interface for tests */
rubik_state_t* rtka_rubik_create(void);
rubik_state_t* rtka_rubik_create_solved(void);
void rtka_rubik_free(rubik_state_t* cube);
void rtka_rubik_apply_move(rubik_state_t* cube, rubik_move_t move);
bool rtka_rubik_is_solved(rubik_state_t* cube);
void rtka_rubik_print(rubik_state_t* cube);
rubik_solver_t* rtka_rubik_solver_create(rubik_state_t* initial);
bool rtka_rubik_solve(rubik_solver_t* solver);
rtka_confidence_t rtka_rubik_manhattan_distance(rubik_state_t* cube);

#endif /* RTKA_RUBIK_H */
