/**
 * File: rtka_rubik_324.h
 * Copyright (c) 2025 - H.Overman opsec.ee@pm.me
 * 
 * PROPRIETARY AND CONFIDENTIAL
 * This file is proprietary to H. Overman and may not be reproduced,
 * distributed, or used without explicit written permission.
 *
 * For licensing inquiries: opsec.ee@pm.me
 *
 * RTKA 324-State Rubik's Cube - Bit-based constraint solver
 */

#ifndef RTKA_RUBIK_324_H
#define RTKA_RUBIK_324_H

#include "rtka_u_core.h"
#include "rtka_types.h"
#include <stdbool.h>

/* Forward declaration */
typedef struct rubik_324_state rubik_324_state_t;

/* Move types - simplified for bit operations */
typedef enum {
    MOVE_324_U,    /* UP clockwise */
    MOVE_324_D,    /* DOWN clockwise */
    MOVE_324_F,    /* FRONT clockwise */
    MOVE_324_B,    /* BACK clockwise */
    MOVE_324_L,    /* LEFT clockwise */
    MOVE_324_R,    /* RIGHT clockwise */
    MOVE_324_COUNT
} rubik_324_move_t;

/* Create/destroy */
rubik_324_state_t* rtka_rubik_324_create_solved(void);
void rtka_rubik_324_free(rubik_324_state_t* cube);

/* Operations */
void rtka_rubik_324_rotate(rubik_324_state_t* cube, rubik_324_move_t move);
bool rtka_rubik_324_is_solved(const rubik_324_state_t* cube);
rtka_confidence_t rtka_rubik_324_heuristic(const rubik_324_state_t* cube);
bool rtka_rubik_324_solve(rubik_324_state_t* cube, uint32_t max_depth);

/* Analysis */
void rtka_rubik_324_print_analysis(const rubik_324_state_t* cube);
void rtka_rubik_324_scramble(rubik_324_state_t* cube, uint32_t moves);

/* Stats */
uint32_t rtka_rubik_324_get_transitions(const rubik_324_state_t* cube);
uint32_t rtka_rubik_324_get_moves(const rubik_324_state_t* cube);

#endif /* RTKA_RUBIK_324_H */
