/**
 * File: rtka_solver.h
 * Copyright (c) 2025 - H.Overman opsec.ee@pm.me
 * 
 * PROPRIETARY AND CONFIDENTIAL
 * This file is proprietary to H. Overman and may not be reproduced,
 * distributed, or used without explicit written permission.
 *
 * For licensing inquiries: opsec.ee@pm.me
 *
 * RTKA Generic Constraint Satisfaction Framework
 */

#ifndef RTKA_SOLVER_H
#define RTKA_SOLVER_H

#include "rtka_types.h"
#include "rtka_u_core.h"

/* Constraint types */
typedef enum {
    CONSTRAINT_UNARY,
    CONSTRAINT_BINARY,
    CONSTRAINT_ALLDIFF,
    CONSTRAINT_SUM,
    CONSTRAINT_TERNARY
} rtka_constraint_type_t;

/* Constraint structure */
typedef struct {
    rtka_constraint_type_t type;
    uint32_t* variables;
    uint32_t num_vars;
    bool (*check_fn)(const rtka_state_t*, uint32_t);
    void* data;
} rtka_constraint_t;

/* CSP problem */
typedef struct {
    rtka_state_t* variables;
    uint32_t num_variables;
    rtka_constraint_t** constraints;
    uint32_t num_constraints;
    uint32_t* domain_sizes;
    rtka_value_t** domains;
} rtka_csp_t;

/* Search strategies */
typedef enum {
    SEARCH_BACKTRACK,
    SEARCH_FORWARD_CHECK,
    SEARCH_ARC_CONSISTENCY,
    SEARCH_TERNARY_PROPAGATE
} rtka_search_strategy_t;

/* CSP creation */
rtka_csp_t* rtka_solver_create_csp(uint32_t num_vars);
void rtka_solver_add_constraint(rtka_csp_t* csp, rtka_constraint_t* constraint);
void rtka_solver_free_csp(rtka_csp_t* csp);

/* Solving */
bool rtka_solver_solve(rtka_csp_t* csp, rtka_search_strategy_t strategy);
bool rtka_solver_find_all_solutions(rtka_csp_t* csp, rtka_state_t** solutions, uint32_t max_solutions);

/* Constraint propagation */
bool rtka_solver_arc_consistency(rtka_csp_t* csp);
bool rtka_solver_propagate_ternary(rtka_csp_t* csp);

/* Variable selection heuristics */
uint32_t rtka_solver_mrv_heuristic(rtka_csp_t* csp);  /* Minimum Remaining Values */
uint32_t rtka_solver_degree_heuristic(rtka_csp_t* csp);
uint32_t rtka_solver_confidence_heuristic(rtka_csp_t* csp);

/* Common constraints */
rtka_constraint_t* rtka_constraint_alldiff(uint32_t* vars, uint32_t count);
rtka_constraint_t* rtka_constraint_sum(uint32_t* vars, uint32_t count, rtka_confidence_t target);
rtka_constraint_t* rtka_constraint_ternary_logic(uint32_t* vars, uint32_t count, 
                                                rtka_state_t (*logic_fn)(const rtka_state_t*, uint32_t));

/* Statistics */
typedef struct {
    uint32_t nodes_explored;
    uint32_t backtracks;
    uint32_t constraint_checks;
    uint32_t ternary_transitions;
    double solve_time;
} rtka_solver_stats_t;

rtka_solver_stats_t* rtka_solver_get_stats(void);

#endif /* RTKA_SOLVER_H */
