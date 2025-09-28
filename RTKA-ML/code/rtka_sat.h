/**
 * File: rtka_sat.h
 * Copyright (c) 2025 - H.Overman opsec.ee@pm.me
 *
 * PROPRIETARY AND CONFIDENTIAL
 * This file is proprietary to H. Overman and may not be reproduced,
 * distributed, or used without explicit written permission.
 *
 * For licensing inquiries: opsec.ee@pm.me
 *
 */

#ifndef RTKA_SAT_H
#define RTKA_SAT_H

#include "rtka_u_core.h"
#include "rtka_types.h"

#define SAT_MAX_VARS 100
#define SAT_MAX_CLAUSES 500
#define SAT_MAX_LITERALS 10

typedef struct {
    int32_t literals[SAT_MAX_LITERALS];
    uint32_t size;
    bool satisfied;
} sat_clause_t;

typedef struct {
    rtka_state_t variables[SAT_MAX_VARS + 1];
    sat_clause_t clauses[SAT_MAX_CLAUSES];
    uint32_t num_vars;
    uint32_t num_clauses;
    uint32_t assigned;
    uint32_t rtka_transitions;
} sat_state_t;

void rtka_sat_init(sat_state_t* state, uint32_t vars);
void rtka_sat_add_clause(sat_state_t* state, int32_t* literals, uint32_t size);
bool rtka_sat_solve(sat_state_t* state);
bool rtka_sat_is_satisfied(const sat_state_t* state);

#endif
