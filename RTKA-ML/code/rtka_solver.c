/**
 * File: rtka_solver.c
 * Copyright (c) 2025 - H.Overman opsec.ee@pm.me
 * 
 * PROPRIETARY AND CONFIDENTIAL
 * This file is proprietary to H. Overman and may not be reproduced,
 * distributed, or used without explicit written permission.
 *
 * For licensing inquiries: opsec.ee@pm.me
 *
 * RTKA Generic CSP Solver Implementation
 */

#include "rtka_solver.h"
#include "rtka_memory.h"
#include <string.h>
#include <stdlib.h>
#include <time.h>

/* Global statistics */
static rtka_solver_stats_t g_stats = {0};

/* Create CSP */
rtka_csp_t* rtka_solver_create_csp(uint32_t num_vars) {
    rtka_csp_t* csp = (rtka_csp_t*)rtka_alloc_state();
    if (!csp) return NULL;
    
    csp->num_variables = num_vars;
    csp->variables = rtka_alloc_states(num_vars);
    csp->domain_sizes = (uint32_t*)rtka_alloc_states(num_vars * sizeof(uint32_t) / sizeof(rtka_state_t));
    csp->domains = (rtka_value_t**)rtka_alloc_states(num_vars * sizeof(void*) / sizeof(rtka_state_t));
    
    /* Initialize to UNKNOWN */
    for (uint32_t i = 0; i < num_vars; i++) {
        csp->variables[i] = rtka_make_state(RTKA_UNKNOWN, 0.5f);
        csp->domain_sizes[i] = 3;  /* Ternary domain by default */
    }
    
    csp->constraints = NULL;
    csp->num_constraints = 0;
    
    return csp;
}

/* Check constraint */
static bool check_constraint(rtka_constraint_t* constraint, const rtka_state_t* vars) {
    g_stats.constraint_checks++;
    
    switch (constraint->type) {
        case CONSTRAINT_ALLDIFF: {
            for (uint32_t i = 0; i < constraint->num_vars - 1; i++) {
                for (uint32_t j = i + 1; j < constraint->num_vars; j++) {
                    uint32_t vi = constraint->variables[i];
                    uint32_t vj = constraint->variables[j];
                    
                    if (vars[vi].value != RTKA_UNKNOWN && 
                        vars[vj].value != RTKA_UNKNOWN &&
                        vars[vi].value == vars[vj].value) {
                        return false;
                    }
                }
            }
            return true;
        }
        
        case CONSTRAINT_TERNARY: {
            if (constraint->check_fn) {
                rtka_state_t* constraint_vars = rtka_alloc_states(constraint->num_vars);
                for (uint32_t i = 0; i < constraint->num_vars; i++) {
                    constraint_vars[i] = vars[constraint->variables[i]];
                }
                bool result = constraint->check_fn(constraint_vars, constraint->num_vars);
                return result;
            }
            return true;
        }
        
        default:
            return true;
    }
}

/* Forward checking */
static bool forward_check(rtka_csp_t* csp, uint32_t var) {
    for (uint32_t i = 0; i < csp->num_constraints; i++) {
        rtka_constraint_t* constraint = csp->constraints[i];
        
        /* Check if var is in this constraint */
        bool involves_var = false;
        for (uint32_t j = 0; j < constraint->num_vars; j++) {
            if (constraint->variables[j] == var) {
                involves_var = true;
                break;
            }
        }
        
        if (involves_var) {
            /* Check if constraint can still be satisfied */
            bool has_support = false;
            
            for (uint32_t v = 0; v < csp->num_variables; v++) {
                if (csp->variables[v].value == RTKA_UNKNOWN) {
                    /* Try each value in domain */
                    rtka_value_t original = csp->variables[v].value;
                    
                    csp->variables[v].value = RTKA_TRUE;
                    if (check_constraint(constraint, csp->variables)) {
                        has_support = true;
                        csp->variables[v].value = original;
                        break;
                    }
                    
                    csp->variables[v].value = RTKA_FALSE;
                    if (check_constraint(constraint, csp->variables)) {
                        has_support = true;
                        csp->variables[v].value = original;
                        break;
                    }
                    
                    csp->variables[v].value = original;
                }
            }
            
            if (!has_support && constraint->num_vars > 1) {
                return false;
            }
        }
    }
    
    return true;
}

/* Ternary propagation */
bool rtka_solver_propagate_ternary(rtka_csp_t* csp) {
    bool changed = true;
    
    while (changed) {
        changed = false;
        
        for (uint32_t i = 0; i < csp->num_constraints; i++) {
            rtka_constraint_t* constraint = csp->constraints[i];
            
            /* Count unknowns in constraint */
            uint32_t unknown_count = 0;
            uint32_t unknown_var = 0;
            
            for (uint32_t j = 0; j < constraint->num_vars; j++) {
                uint32_t var = constraint->variables[j];
                if (csp->variables[var].value == RTKA_UNKNOWN) {
                    unknown_count++;
                    unknown_var = var;
                }
            }
            
            /* Unit propagation */
            if (unknown_count == 1) {
                /* Try to infer value */
                rtka_value_t original = csp->variables[unknown_var].value;
                rtka_confidence_t original_conf = csp->variables[unknown_var].confidence;
                
                bool found_valid = false;
                rtka_value_t valid_value = RTKA_UNKNOWN;
                
                for (int v = -1; v <= 1; v++) {
                    csp->variables[unknown_var].value = (rtka_value_t)v;
                    csp->variables[unknown_var].confidence = 1.0f;
                    
                    if (check_constraint(constraint, csp->variables)) {
                        if (found_valid && valid_value != (rtka_value_t)v) {
                            /* Multiple valid values - can't infer */
                            csp->variables[unknown_var].value = original;
                            csp->variables[unknown_var].confidence = original_conf;
                            break;
                        }
                        found_valid = true;
                        valid_value = (rtka_value_t)v;
                    }
                }
                
                if (found_valid && valid_value != RTKA_UNKNOWN) {
                    csp->variables[unknown_var].value = valid_value;
                    csp->variables[unknown_var].confidence = 1.0f;
                    changed = true;
                    g_stats.ternary_transitions++;
                } else {
                    csp->variables[unknown_var].value = original;
                    csp->variables[unknown_var].confidence = original_conf;
                }
            }
        }
    }
    
    return true;
}

/* Backtracking search */
static bool backtrack(rtka_csp_t* csp, uint32_t depth, rtka_search_strategy_t strategy) {
    g_stats.nodes_explored++;
    
    /* Check if complete */
    bool complete = true;
    for (uint32_t i = 0; i < csp->num_variables; i++) {
        if (csp->variables[i].value == RTKA_UNKNOWN) {
            complete = false;
            break;
        }
    }
    
    if (complete) {
        /* Verify all constraints */
        for (uint32_t i = 0; i < csp->num_constraints; i++) {
            if (!check_constraint(csp->constraints[i], csp->variables)) {
                return false;
            }
        }
        return true;
    }
    
    /* Select unassigned variable */
    uint32_t var = UINT32_MAX;
    for (uint32_t i = 0; i < csp->num_variables; i++) {
        if (csp->variables[i].value == RTKA_UNKNOWN) {
            var = i;
            break;
        }
    }
    
    if (var == UINT32_MAX) return false;
    
    /* Try each value in domain */
    rtka_value_t values[] = {RTKA_TRUE, RTKA_FALSE, RTKA_UNKNOWN};
    
    for (int v = 0; v < 3; v++) {
        if (values[v] == RTKA_UNKNOWN && depth > 0) continue;
        
        rtka_state_t original = csp->variables[var];
        csp->variables[var].value = values[v];
        csp->variables[var].confidence = (values[v] == RTKA_UNKNOWN) ? 0.5f : 1.0f;
        g_stats.ternary_transitions++;
        
        bool consistent = true;
        
        /* Check constraints involving this variable */
        for (uint32_t c = 0; c < csp->num_constraints; c++) {
            rtka_constraint_t* constraint = csp->constraints[c];
            bool involves_var = false;
            
            for (uint32_t j = 0; j < constraint->num_vars; j++) {
                if (constraint->variables[j] == var) {
                    involves_var = true;
                    break;
                }
            }
            
            if (involves_var && !check_constraint(constraint, csp->variables)) {
                consistent = false;
                break;
            }
        }
        
        if (consistent) {
            bool proceed = true;
            
            if (strategy == SEARCH_FORWARD_CHECK) {
                proceed = forward_check(csp, var);
            } else if (strategy == SEARCH_TERNARY_PROPAGATE) {
                proceed = rtka_solver_propagate_ternary(csp);
            }
            
            if (proceed && backtrack(csp, depth + 1, strategy)) {
                return true;
            }
        }
        
        /* Backtrack */
        csp->variables[var] = original;
        g_stats.backtracks++;
    }
    
    return false;
}

/* Main solve function */
bool rtka_solver_solve(rtka_csp_t* csp, rtka_search_strategy_t strategy) {
    memset(&g_stats, 0, sizeof(g_stats));
    
    clock_t start = clock();
    bool result = backtrack(csp, 0, strategy);
    g_stats.solve_time = (double)(clock() - start) / CLOCKS_PER_SEC;
    
    return result;
}

/* MRV heuristic */
uint32_t rtka_solver_mrv_heuristic(rtka_csp_t* csp) {
    uint32_t best_var = UINT32_MAX;
    uint32_t min_values = UINT32_MAX;
    
    for (uint32_t i = 0; i < csp->num_variables; i++) {
        if (csp->variables[i].value == RTKA_UNKNOWN) {
            uint32_t valid_values = 0;
            
            /* Count valid values */
            for (int v = -1; v <= 1; v++) {
                csp->variables[i].value = (rtka_value_t)v;
                
                bool valid = true;
                for (uint32_t c = 0; c < csp->num_constraints; c++) {
                    if (!check_constraint(csp->constraints[c], csp->variables)) {
                        valid = false;
                        break;
                    }
                }
                
                if (valid) valid_values++;
                csp->variables[i].value = RTKA_UNKNOWN;
            }
            
            if (valid_values < min_values) {
                min_values = valid_values;
                best_var = i;
            }
        }
    }
    
    return best_var;
}

/* Create alldiff constraint */
rtka_constraint_t* rtka_constraint_alldiff(uint32_t* vars, uint32_t count) {
    rtka_constraint_t* constraint = (rtka_constraint_t*)rtka_alloc_state();
    constraint->type = CONSTRAINT_ALLDIFF;
    constraint->variables = vars;
    constraint->num_vars = count;
    constraint->check_fn = NULL;
    constraint->data = NULL;
    return constraint;
}

/* Get stats */
rtka_solver_stats_t* rtka_solver_get_stats(void) {
    return &g_stats;
}
