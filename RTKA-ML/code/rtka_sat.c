/**
 * File: rtka_sat.c
 * Copyright (c) 2025 - H.Overman opsec.ee@pm.me
 * 
 * PROPRIETARY AND CONFIDENTIAL
 * This file is proprietary to H. Overman and may not be reproduced,
 * distributed, or used without explicit written permission.
 *
 * For licensing inquiries: opsec.ee@pm.me
 *
 * RTKA SAT Solver Library
 */

#include <stdlib.h>
#include "rtka_sat.h"
#include <string.h>

void rtka_sat_init(sat_state_t* state, uint32_t vars) {
    memset(state, 0, sizeof(sat_state_t));
    state->num_vars = vars;
    
    for (uint32_t v = 1; v <= vars; v++) {
        state->variables[v].value = RTKA_UNKNOWN;
        state->variables[v].confidence = 0.5f;
    }
}

void rtka_sat_add_clause(sat_state_t* state, int32_t* literals, uint32_t size) {
    if (state->num_clauses >= SAT_MAX_CLAUSES) return;
    
    sat_clause_t* c = &state->clauses[state->num_clauses++];
    c->size = size;
    c->satisfied = false;
    memcpy(c->literals, literals, size * sizeof(int32_t));
}

static bool unit_propagate(sat_state_t* state) {
    bool progress = true;
    
    while (progress) {
        progress = false;
        
        for (uint32_t c = 0; c < state->num_clauses; c++) {
            if (state->clauses[c].satisfied) continue;
            
            int32_t unit_lit = 0;
            uint32_t unknowns = 0;
            
            for (uint32_t i = 0; i < state->clauses[c].size; i++) {
                int32_t lit = state->clauses[c].literals[i];
                uint32_t var = abs(lit);
                rtka_value_t val = state->variables[var].value;
                
                if (val == RTKA_UNKNOWN) {
                    unit_lit = lit;
                    unknowns++;
                } else if ((lit > 0 && val == RTKA_TRUE) || 
                          (lit < 0 && val == RTKA_FALSE)) {
                    state->clauses[c].satisfied = true;
                    break;
                }
            }
            
            if (!state->clauses[c].satisfied && unknowns == 1) {
                uint32_t var = abs(unit_lit);
                state->variables[var].value = (unit_lit > 0) ? RTKA_TRUE : RTKA_FALSE;
                state->variables[var].confidence = 1.0f;
                state->assigned++;
                state->rtka_transitions++;
                state->clauses[c].satisfied = true;
                progress = true;
            }
            
            if (!state->clauses[c].satisfied && unknowns == 0) {
                return false;
            }
        }
    }
    
    return true;
}

static uint32_t choose_variable(const sat_state_t* state) {
    uint32_t best = 0;
    rtka_confidence_t min_conf = 1.1f;
    
    for (uint32_t v = 1; v <= state->num_vars; v++) {
        if (state->variables[v].value == RTKA_UNKNOWN) {
            rtka_confidence_t uncertainty = 1.0f - state->variables[v].confidence;
            if (uncertainty < min_conf) {
                min_conf = uncertainty;
                best = v;
            }
        }
    }
    
    return best;
}

static bool dpll(sat_state_t* state) {
    if (!unit_propagate(state)) return false;
    
    bool all_sat = true;
    for (uint32_t c = 0; c < state->num_clauses; c++) {
        if (!state->clauses[c].satisfied) {
            all_sat = false;
            break;
        }
    }
    if (all_sat) return true;
    
    uint32_t var = choose_variable(state);
    if (var == 0) return false;
    
    for (int attempt = 0; attempt < 2; attempt++) {
        sat_state_t backup;
        memcpy(&backup, state, sizeof(sat_state_t));
        
        state->variables[var].value = (attempt == 0) ? RTKA_TRUE : RTKA_FALSE;
        state->variables[var].confidence = 0.5f;
        state->assigned++;
        state->rtka_transitions++;
        
        if (dpll(state)) return true;
        
        memcpy(state, &backup, sizeof(sat_state_t));
    }
    
    return false;
}

bool rtka_sat_solve(sat_state_t* state) {
    return dpll(state);
}

bool rtka_sat_is_satisfied(const sat_state_t* state) {
    for (uint32_t c = 0; c < state->num_clauses; c++) {
        if (!state->clauses[c].satisfied) return false;
    }
    return true;
}
