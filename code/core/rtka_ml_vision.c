/**
 * File: rtka_ml_vision.c
 * Author H. Overman
 * Date 2025-09-18
 * Copyright (c) 2025 - H. Overman <opsec.ee@pm.me>
 */

/*
CHANGELOG:
v1.1 - Enhanced for vision AI with recursive UNKNOWN
- vector convergence for batch confidence: Balances responsiveness in image batches.
- Hoisted invariants in loop. Minimizes branches in hot paths.
- Integrated recursive eval on UNKNOWN: Ties to infinite recursion for ambiguous pixels.
*/

#include "rtka_ml.h"
#include "rtka_u_recursive_data.c"  // For recursive UNKNOWN

ternary_tensor_t* layer_forward_vision(ternary_layer_t* restrict layer,
                                       const ternary_tensor_t* restrict input) {
    size_t out_size = layer->output_size;  // Hoist invariant
    ternary_tensor_t* output = tensor_create(&out_size, 1U);
    if (!output) return NULL;

    // Batch ternary matmul with conf prop
    for (size_t o = 0U; o < out_size; o++) {
        trit_t sum = T_FALSE;
        float conf_prod = 1.0f;

        for (size_t i = 0U; i < layer->input_size; i++) {
            trit_t w = layer->weights->data[o * layer->input_size + i];
            trit_t in_val = input->data[i];
            float w_conf = layer->weights->confidence[o * layer->input_size + i];
            float in_conf = input->confidence[i];

            trit_t prod = ternary_mul(w, in_val);  // Custom mul: a*b in {-1,0,1}
            sum = ternary_add(sum, prod);  // Clamped add
            conf_prod *= rtka_conf_and(w_conf, in_conf);  // Multiplicative
        }

        trit_t activated = ternary_activate(sum + layer->bias->data[o]);
        output->data[o] = activated;
        output->confidence[o] = conf_prod * layer->bias->confidence[o];

        // Convergent conf update
        static vector_convergence_t vc = {.convergence_rate = 0.9f};
        float new_target = output->confidence[o];
        update_vector_convergence(&vc, &new_target);  // Batch-adapt
        output->confidence[o] = vc.values[0];  // Simplified for single
    }

    // Recursive UNKNOWN handling for ambiguous outputs
    for (size_t o = 0U; o < out_size; o++) {
        if (output->data[o] == T_MAYBE && output->confidence[o] < 0.5f) {
            recursive_unknown_node_t* node = init_recursive_node(RTKA_UNKNOWN, output->confidence[o], NULL, 5U);
            rtka_state_t refined = evaluate_recursive_unknown(node, logf(0.1f));  // Log threshold
            output->data[o] = (trit_t)refined.value;
            output->confidence[o] = refined.confidence;
            free_recursive_node(node);
        }
    }

    return output;
}
