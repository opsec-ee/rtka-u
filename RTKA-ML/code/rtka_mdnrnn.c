/**
 * File: rtka_mdnrnn.c
 * Copyright (c) 2025 - H.Overman opsec.ee@pm.me
 * Email: opsec.ee@pm.me
 * 
 * RTKA MDNRNN Implementation
 * 
 * CHANGELOG:
 * v1.0.0 - 2025-10-12: Initial implementation
 *   - Combined LSTM and MDN forward pass
 *   - Input concatenation (z + actions)
 *   - Sequence processing pipeline
 *   - Single-step prediction support
 */

#include "rtka_mdnrnn.h"
#include <stdlib.h>
#include <string.h>

rtka_mdnrnn_t* rtka_mdnrnn_create(uint32_t z_size,
                                  uint32_t action_size,
                                  uint32_t hidden_size,
                                  uint32_t num_gaussians) {
    if (z_size == 0 || hidden_size == 0 || num_gaussians == 0) return NULL;
    
    rtka_mdnrnn_t* model = (rtka_mdnrnn_t*)calloc(1, sizeof(rtka_mdnrnn_t));
    if (!model) return NULL;
    
    model->z_size = z_size;
    model->action_size = action_size;
    model->hidden_size = hidden_size;
    model->num_gaussians = num_gaussians;
    model->batch_first = true;
    model->initialized = false;
    
    /* Create LSTM: input is z + actions */
    uint32_t lstm_input_size = z_size + action_size;
    model->lstm = rtka_lstm_create(lstm_input_size, hidden_size, true);
    if (!model->lstm) {
        free(model);
        return NULL;
    }
    
    /* Create MDN: input is LSTM hidden output, output is z_size predictions */
    model->mdn = rtka_mdn_create(hidden_size, z_size, num_gaussians);
    if (!model->mdn) {
        rtka_lstm_free(model->lstm);
        free(model);
        return NULL;
    }
    
    model->initialized = true;
    return model;
}

bool rtka_mdnrnn_init_hidden(rtka_mdnrnn_t* model, uint32_t batch_size) {
    if (!model || !model->lstm) return false;
    return rtka_lstm_init_hidden(model->lstm, batch_size);
}

void rtka_mdnrnn_reset_parameters(rtka_mdnrnn_t* model) {
    if (!model) return;
    
    if (model->lstm) rtka_lstm_reset_parameters(model->lstm);
    if (model->mdn) rtka_mdn_reset_parameters(model->mdn);
}

/* Concatenate z and actions along feature dimension */
static rtka_tensor_t* concatenate_inputs(rtka_tensor_t* z, rtka_tensor_t* actions) {
    if (!z || !actions) return NULL;
    
    if (z->ndim != 3 || actions->ndim != 3) return NULL;
    if (z->shape[0] != actions->shape[0] || z->shape[1] != actions->shape[1]) return NULL;
    
    uint32_t batch = z->shape[0];
    uint32_t seq_len = z->shape[1];
    uint32_t z_size = z->shape[2];
    uint32_t action_size = actions->shape[2];
    
    uint32_t concat_shape[] = {batch, seq_len, z_size + action_size};
    rtka_tensor_t* concat = rtka_tensor_create(concat_shape, 3);
    if (!concat) return NULL;
    
    /* Copy data */
    for (uint32_t b = 0; b < batch; b++) {
        for (uint32_t t = 0; t < seq_len; t++) {
            /* Copy z */
            for (uint32_t i = 0; i < z_size; i++) {
                uint32_t src_idx[] = {b, t, i};
                uint32_t dst_idx[] = {b, t, i};
                rtka_tensor_set(concat, dst_idx, *rtka_tensor_get(z, src_idx));
            }
            
            /* Copy actions */
            for (uint32_t i = 0; i < action_size; i++) {
                uint32_t src_idx[] = {b, t, i};
                uint32_t dst_idx[] = {b, t, z_size + i};
                rtka_tensor_set(concat, dst_idx, *rtka_tensor_get(actions, src_idx));
            }
        }
    }
    
    return concat;
}

rtka_mdnrnn_output_t rtka_mdnrnn_forward(rtka_mdnrnn_t* model,
                                         rtka_tensor_t* z,
                                         rtka_tensor_t* actions,
                                         rtka_tensor_t* h_0,
                                         rtka_tensor_t* c_0) {
    rtka_mdnrnn_output_t result = {
        .mdn_params = {NULL, NULL, NULL},
        .hidden_state = NULL,
        .cell_state = NULL
    };
    
    if (!model || !model->initialized || !z || !actions) return result;
    
    /* Concatenate z and actions */
    rtka_tensor_t* concat_input = concatenate_inputs(z, actions);
    if (!concat_input) return result;
    
    /* Create gradient node for LSTM input */
    rtka_grad_node_t* lstm_input = rtka_grad_node_create(concat_input, false);
    if (!lstm_input) {
        rtka_tensor_free(concat_input);
        return result;
    }
    
    /* Forward through LSTM */
    rtka_lstm_output_t lstm_out = rtka_lstm_forward(model->lstm, lstm_input, h_0, c_0);
    if (!lstm_out.output) {
        rtka_grad_node_free(lstm_input);
        return result;
    }
    
    /* Forward through MDN */
    result.mdn_params = rtka_mdn_forward(model->mdn, lstm_out.output);
    
    /* Store final states */
    result.hidden_state = lstm_out.hidden_state;
    result.cell_state = lstm_out.cell_state;
    
    /* Note: lstm_input and lstm_out.output are freed by caller or gradient system */
    
    return result;
}

rtka_mdn_output_t rtka_mdnrnn_step(rtka_mdnrnn_t* model,
                                   rtka_tensor_t* z_t,
                                   rtka_tensor_t* a_t) {
    rtka_mdn_output_t result = {NULL, NULL, NULL};
    
    if (!model || !model->initialized || !z_t || !a_t) return result;
    
    if (z_t->ndim != 2 || a_t->ndim != 2) return result;
    if (z_t->shape[0] != a_t->shape[0]) return result;
    
    uint32_t batch = z_t->shape[0];
    uint32_t z_size = z_t->shape[1];
    uint32_t action_size = a_t->shape[1];
    
    /* Concatenate for single timestep */
    uint32_t concat_shape[] = {batch, z_size + action_size};
    rtka_tensor_t* concat = rtka_tensor_create(concat_shape, 2);
    if (!concat) return result;
    
    for (uint32_t b = 0; b < batch; b++) {
        for (uint32_t i = 0; i < z_size; i++) {
            uint32_t src_idx[] = {b, i};
            uint32_t dst_idx[] = {b, i};
            rtka_tensor_set(concat, dst_idx, *rtka_tensor_get(z_t, src_idx));
        }
        for (uint32_t i = 0; i < action_size; i++) {
            uint32_t src_idx[] = {b, i};
            uint32_t dst_idx[] = {b, z_size + i};
            rtka_tensor_set(concat, dst_idx, *rtka_tensor_get(a_t, src_idx));
        }
    }
    
    /* Single LSTM step */
    rtka_tensor_t* h_next = NULL;
    rtka_tensor_t* c_next = NULL;
    
    bool success = rtka_lstm_cell_forward(model->lstm, concat, 
                                          model->lstm->h_prev,
                                          model->lstm->c_prev,
                                          &h_next, &c_next);
    
    rtka_tensor_free(concat);
    
    if (!success) return result;
    
    /* Update LSTM hidden states */
    if (model->lstm->h_prev) rtka_tensor_free(model->lstm->h_prev);
    if (model->lstm->c_prev) rtka_tensor_free(model->lstm->c_prev);
    model->lstm->h_prev = h_next;
    model->lstm->c_prev = c_next;
    
    /* Forward through MDN */
    rtka_grad_node_t* mdn_input = rtka_grad_node_create(h_next, false);
    if (!mdn_input) return result;
    
    result = rtka_mdn_forward(model->mdn, mdn_input);
    
    rtka_grad_node_free(mdn_input);
    
    return result;
}

void rtka_mdnrnn_free(rtka_mdnrnn_t* model) {
    if (!model) return;
    
    if (model->lstm) rtka_lstm_free(model->lstm);
    if (model->mdn) rtka_mdn_free(model->mdn);
    
    free(model);
}

void rtka_mdnrnn_output_free(rtka_mdnrnn_output_t* output) {
    if (!output) return;
    
    rtka_mdn_output_free(&output->mdn_params);
    
    if (output->hidden_state) rtka_tensor_free(output->hidden_state);
    if (output->cell_state) rtka_tensor_free(output->cell_state);
    
    output->hidden_state = NULL;
    output->cell_state = NULL;
}

uint32_t rtka_mdnrnn_param_count(const rtka_mdnrnn_t* model) {
    if (!model) return 0;
    
    uint32_t lstm_params = rtka_lstm_param_count(model->lstm);
    uint32_t mdn_params = rtka_mdn_param_count(model->mdn);
    
    return lstm_params + mdn_params;
}

rtka_tensor_t* rtka_mdnrnn_sample_next(const rtka_mdnrnn_output_t* output, 
                                       uint32_t batch_idx) {
    if (!output) return NULL;
    return rtka_mdn_sample(&output->mdn_params, batch_idx);
}
