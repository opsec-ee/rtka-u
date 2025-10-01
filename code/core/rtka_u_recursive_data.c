/**
 * File: rtka_u_recursive_data.c
 * Author H. Overman
 * Date 2025-09-18
 * Copyright (c) 2025 - H. Overman <opsec.ee@pm.me>
 */

/*
CHANGELOG:
v1.0 - Initial implementation for recursive UNKNOWN data placement
- Integrated infinite recursion handling with depth guards
- Added logarithmic confidence scaling to handle precision from 1e-9 to 1.0 without underflow
- adaptive confidence updates in recursion
- unsigned types for depths/indices
- Early termination on absorbing states from rtka_u_core.h
*/

#include "rtka_u_core.h"  // Core ternary and state management
#include <math.h>         // For logf/fabsf
#include <stdint.h>
#include <stdlib.h>

// Recursive node for UNKNOWN branching with data pointers
typedef struct recursive_unknown_node {
    rtka_state_t state;             // Value and confidence
    void* data_ptr;                 // Pointer to data at this level
    struct recursive_unknown_node* if_unknown;  // Recurse deeper on UNKNOWN
    uint32_t depth;                 // unsigned depth
    float log_confidence;           // Log-scaled for precision (handles 1e-9 to 1)
    char padding[64 - (sizeof(rtka_state_t) + sizeof(void*) + sizeof(void*) +
                       sizeof(uint32_t) + sizeof(float)) % 64];  // cache align
} recursive_unknown_node_t;

// Convergence struct for adaptive log_conf updates
typedef struct {
    float value;                    // Current log_conf
    float target;                   // New target log_conf
    float convergence_rate;         // ρ=0.9f default
    float error_history[16];        // For adaptive tuning
    uint32_t history_idx;           // OPT-000 [TS-001] unsigned
} log_conf_convergent_t;

// Initialize recursive node
recursive_unknown_node_t* init_recursive_node(rtka_value_t value, float confidence, void* data, uint32_t max_depth) {
    if (max_depth == 0U) return NULL;  // Guard against infinite recursion

    recursive_unknown_node_t* node = calloc(1, sizeof(recursive_unknown_node_t));
    if (!node) return NULL;

    node->state = rtka_make_state(value, confidence);  // From core
    node->data_ptr = data;
    node->depth = max_depth;
    node->log_confidence = logf(fmaxf(confidence, 1e-9f));  // Log scale, clamp to avoid -inf

    if (value == RTKA_UNKNOWN && max_depth > 1U) {
        // Recurse into deeper UNKNOWN (fractal branching)
        node->if_unknown = init_recursive_node(RTKA_UNKNOWN, confidence * 0.9f, NULL, max_depth - 1U);
    }

    return node;
}

// Update log confidence adaptively (OPT-210 applied)
void update_log_confidence(log_conf_convergent_t* cv, float new_target_log) {
    float error = fabsf(cv->target - new_target_log);
    cv->error_history[cv->history_idx] = error;
    cv->history_idx = (cv->history_idx + 1U) & 15U;  // unsigned literal

    float avg_error = 0.0f;
    for (uint32_t i = 0U; i < 16U; i++) {
        avg_error += cv->error_history[i];
    }
    avg_error /= 16.0f;

    float adaptive_rate = (avg_error > 0.1f) ? 1.05f : 0.95f;  // Faster on high error
    float rate = cv->convergence_rate * adaptive_rate;
    cv->value = rate * cv->value + (1.0f - rate) * new_target_log;
    cv->target = new_target_log;
}

// Traverse and place/retrieve data at specific depth
void* place_data_at_level(recursive_unknown_node_t* node, uint32_t target_depth, void* data) {
    if (!node || node->depth < target_depth) return NULL;

    if (node->depth == target_depth) {
        void* old_data = node->data_ptr;
        node->data_ptr = data;
        return old_data;  // Return previous data if any
    }

    // Recurse if UNKNOWN branch exists (early term if not)
    if (node->if_unknown) {
        return place_data_at_level(node->if_unknown, target_depth, data);
    }
    return NULL;
}

// Evaluate recursive structure with log precision
rtka_state_t evaluate_recursive_unknown(recursive_unknown_node_t* node, float threshold_log) {
    if (!node) return rtka_make_state(RTKA_UNKNOWN, 0.0f);

    // Absorbing check
    if (node->state.value != RTKA_UNKNOWN) return node->state;

    // Recursive eval with log conf
    rtka_state_t child_state = evaluate_recursive_unknown(node->if_unknown, threshold_log);
    float combined_log_conf = node->log_confidence + child_state.confidence;  // Add logs (multiply probs)
    float exp_conf = expf(combined_log_conf);  // Back to linear scale

    // OPT-210: Adaptive update
    static log_conf_convergent_t cv = {.convergence_rate = 0.9f, .history_idx = 0U};
    update_log_confidence(&cv, combined_log_conf);

    rtka_state_t result = rtka_make_state(child_state.value, exp_conf);
    if (combined_log_conf < threshold_log) {  // Coerce if below log threshold
        result.value = RTKA_UNKNOWN;
    }

    return result;
}

// Free recursive structure (post-order to avoid leaks)
void free_recursive_node(recursive_unknown_node_t* node) {
    if (!node) return;
    free_recursive_node(node->if_unknown);
    free(node);
}
