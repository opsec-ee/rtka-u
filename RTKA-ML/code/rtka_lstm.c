/**
 * File: rtka_lstm.c
 * Copyright (c) 2025 - H.Overman opsec.ee@pm.me
 * Email: opsec.ee@pm.me
 * 
 * RTKA LSTM Implementation
 * 
 * CHANGELOG:
 * v1.0.0 - 2025-10-12: Initial implementation
 *   - LSTM cell operations
 *   - Xavier weight initialization
 *   - Batch sequence processing
 *   - Memory-efficient state management
 */

#include "rtka_lstm.h"
#include "rtka_memory.h"
#include <math.h>
#include <string.h>
#include <stdlib.h>

/* Xavier/Glorot initialization for LSTM weights */
static void init_xavier(rtka_tensor_t* tensor, uint32_t fan_in, uint32_t fan_out) {
    rtka_confidence_t limit = sqrtf(6.0f / (fan_in + fan_out));
    
    for (uint32_t i = 0; i < tensor->size; i++) {
        /* Uniform distribution in [-limit, limit] */
        rtka_confidence_t value = ((rtka_confidence_t)rand() / RAND_MAX) * 2.0f * limit - limit;
        tensor->data[i].value = (value > 0.0f) ? RTKA_TRUE : 
                               (value < 0.0f) ? RTKA_FALSE : RTKA_UNKNOWN;
        tensor->data[i].confidence = fabsf(value);
    }
}

/* Initialize gate parameters */
static bool init_gate(rtka_lstm_gate_t* gate, uint32_t input_dim, uint32_t output_dim) {
    uint32_t weight_shape[] = {input_dim, output_dim};
    rtka_tensor_t* weight_data = rtka_tensor_create(weight_shape, 2);
    if (!weight_data) return false;
    
    gate->weight = rtka_grad_node_create(weight_data, true);
    if (!gate->weight) {
        rtka_tensor_free(weight_data);
        return false;
    }
    
    /* Initialize weights */
    init_xavier(weight_data, input_dim, output_dim);
    
    /* Create and initialize bias */
    uint32_t bias_shape[] = {output_dim};
    rtka_tensor_t* bias_data = rtka_tensor_create(bias_shape, 1);
    if (!bias_data) {
        rtka_grad_node_free(gate->weight);
        return false;
    }
    
    gate->bias = rtka_grad_node_create(bias_data, true);
    if (!gate->bias) {
        rtka_tensor_free(bias_data);
        rtka_grad_node_free(gate->weight);
        return false;
    }
    
    /* Initialize bias to zero for forget gate, others near zero */
    memset(bias_data->data, 0, bias_data->size * sizeof(rtka_state_t));
    
    return true;
}

rtka_lstm_layer_t* rtka_lstm_create(uint32_t input_size, 
                                    uint32_t hidden_size,
                                    bool batch_first) {
    rtka_lstm_layer_t* lstm = (rtka_lstm_layer_t*)calloc(1, sizeof(rtka_lstm_layer_t));
    if (!lstm) return NULL;
    
    lstm->input_size = input_size;
    lstm->hidden_size = hidden_size;
    lstm->batch_first = batch_first;
    lstm->initialized = false;
    
    uint32_t combined_size = input_size + hidden_size;
    
    /* Initialize all gates */
    if (!init_gate(&lstm->gate_i, combined_size, hidden_size) ||
        !init_gate(&lstm->gate_f, combined_size, hidden_size) ||
        !init_gate(&lstm->gate_g, combined_size, hidden_size) ||
        !init_gate(&lstm->gate_o, combined_size, hidden_size)) {
        rtka_lstm_free(lstm);
        return NULL;
    }
    
    lstm->h_prev = NULL;
    lstm->c_prev = NULL;
    
    return lstm;
}

bool rtka_lstm_init_hidden(rtka_lstm_layer_t* lstm, uint32_t batch_size) {
    if (!lstm) return false;
    
    /* Free existing states if any */
    if (lstm->h_prev) rtka_tensor_free(lstm->h_prev);
    if (lstm->c_prev) rtka_tensor_free(lstm->c_prev);
    
    /* Create new hidden and cell states initialized to zero */
    uint32_t shape[] = {batch_size, lstm->hidden_size};
    
    lstm->h_prev = rtka_tensor_create(shape, 2);
    if (!lstm->h_prev) return false;
    
    lstm->c_prev = rtka_tensor_create(shape, 2);
    if (!lstm->c_prev) {
        rtka_tensor_free(lstm->h_prev);
        lstm->h_prev = NULL;
        return false;
    }
    
    /* Initialize to zero */
    memset(lstm->h_prev->data, 0, lstm->h_prev->size * sizeof(rtka_state_t));
    memset(lstm->c_prev->data, 0, lstm->c_prev->size * sizeof(rtka_state_t));
    
    lstm->initialized = true;
    return true;
}

void rtka_lstm_reset_parameters(rtka_lstm_layer_t* lstm) {
    if (!lstm) return;
    
    uint32_t combined_size = lstm->input_size + lstm->hidden_size;
    uint32_t hidden_size = lstm->hidden_size;
    
    /* Reinitialize all gates */
    init_xavier(lstm->gate_i.weight->data, combined_size, hidden_size);
    init_xavier(lstm->gate_f.weight->data, combined_size, hidden_size);
    init_xavier(lstm->gate_g.weight->data, combined_size, hidden_size);
    init_xavier(lstm->gate_o.weight->data, combined_size, hidden_size);
    
    /* Reset biases */
    memset(lstm->gate_i.bias->data->data, 0, hidden_size * sizeof(rtka_state_t));
    memset(lstm->gate_f.bias->data->data, 0, hidden_size * sizeof(rtka_state_t));
    memset(lstm->gate_g.bias->data->data, 0, hidden_size * sizeof(rtka_state_t));
    memset(lstm->gate_o.bias->data->data, 0, hidden_size * sizeof(rtka_state_t));
}

/* Matrix-vector multiplication helper */
static rtka_tensor_t* matmul_add_bias(rtka_tensor_t* input, 
                                      rtka_tensor_t* weight,
                                      rtka_tensor_t* bias) {
    /* input: (batch, input_dim), weight: (input_dim, output_dim) */
    if (!input || !weight) return NULL;
    
    uint32_t batch = input->shape[0];
    uint32_t input_dim = input->shape[1];
    uint32_t output_dim = weight->shape[1];
    
    uint32_t out_shape[] = {batch, output_dim};
    rtka_tensor_t* output = rtka_tensor_create(out_shape, 2);
    if (!output) return NULL;
    
    /* Perform matrix multiplication */
    for (uint32_t b = 0; b < batch; b++) {
        for (uint32_t j = 0; j < output_dim; j++) {
            rtka_confidence_t sum = 0.0f;
            
            for (uint32_t i = 0; i < input_dim; i++) {
                uint32_t in_idx[] = {b, i};
                uint32_t w_idx[] = {i, j};
                
                rtka_state_t* in_val = rtka_tensor_get(input, in_idx);
                rtka_state_t* w_val = rtka_tensor_get(weight, w_idx);
                
                sum += in_val->confidence * w_val->confidence * 
                       (rtka_confidence_t)in_val->value * (rtka_confidence_t)w_val->value;
            }
            
            /* Add bias if provided */
            if (bias) {
                uint32_t b_idx[] = {j};
                rtka_state_t* b_val = rtka_tensor_get(bias, b_idx);
                sum += b_val->confidence * (rtka_confidence_t)b_val->value;
            }
            
            uint32_t out_idx[] = {b, j};
            rtka_tensor_set(output, out_idx, rtka_make_state(
                sum > 0.0f ? RTKA_TRUE : sum < 0.0f ? RTKA_FALSE : RTKA_UNKNOWN,
                fabsf(sum)
            ));
        }
    }
    
    return output;
}

/* Element-wise sigmoid application */
static void apply_sigmoid_inplace(rtka_tensor_t* tensor) {
    for (uint32_t i = 0; i < tensor->size; i++) {
        rtka_confidence_t val = tensor->data[i].confidence * 
                               (rtka_confidence_t)tensor->data[i].value;
        rtka_confidence_t result = rtka_lstm_sigmoid(val);
        
        tensor->data[i] = rtka_make_state(
            result > 0.5f ? RTKA_TRUE : RTKA_UNKNOWN,
            result
        );
    }
}

/* Element-wise tanh application */
static void apply_tanh_inplace(rtka_tensor_t* tensor) {
    for (uint32_t i = 0; i < tensor->size; i++) {
        rtka_confidence_t val = tensor->data[i].confidence * 
                               (rtka_confidence_t)tensor->data[i].value;
        rtka_confidence_t result = rtka_lstm_tanh(val);
        
        tensor->data[i] = rtka_make_state(
            result > 0.0f ? RTKA_TRUE : result < 0.0f ? RTKA_FALSE : RTKA_UNKNOWN,
            fabsf(result)
        );
    }
}

/* Element-wise multiplication */
static rtka_tensor_t* elemwise_mul(rtka_tensor_t* a, rtka_tensor_t* b) {
    if (!a || !b || a->size != b->size) return NULL;
    
    rtka_tensor_t* result = rtka_tensor_create(a->shape, a->ndim);
    if (!result) return NULL;
    
    for (uint32_t i = 0; i < a->size; i++) {
        rtka_confidence_t val = (a->data[i].confidence * (rtka_confidence_t)a->data[i].value) *
                               (b->data[i].confidence * (rtka_confidence_t)b->data[i].value);
        
        result->data[i] = rtka_make_state(
            val > 0.0f ? RTKA_TRUE : val < 0.0f ? RTKA_FALSE : RTKA_UNKNOWN,
            fabsf(val)
        );
    }
    
    return result;
}

/* Element-wise addition */
static rtka_tensor_t* elemwise_add(rtka_tensor_t* a, rtka_tensor_t* b) {
    if (!a || !b || a->size != b->size) return NULL;
    
    rtka_tensor_t* result = rtka_tensor_create(a->shape, a->ndim);
    if (!result) return NULL;
    
    for (uint32_t i = 0; i < a->size; i++) {
        rtka_confidence_t val = (a->data[i].confidence * (rtka_confidence_t)a->data[i].value) +
                               (b->data[i].confidence * (rtka_confidence_t)b->data[i].value);
        
        result->data[i] = rtka_make_state(
            val > 0.0f ? RTKA_TRUE : val < 0.0f ? RTKA_FALSE : RTKA_UNKNOWN,
            fabsf(val)
        );
    }
    
    return result;
}

bool rtka_lstm_cell_forward(rtka_lstm_layer_t* lstm,
                            rtka_tensor_t* x_t,
                            rtka_tensor_t* h_t,
                            rtka_tensor_t* c_t,
                            rtka_tensor_t** h_next,
                            rtka_tensor_t** c_next) {
    if (!lstm || !x_t || !h_t || !c_t) return false;
    
    /* Concatenate input and hidden state */
    uint32_t batch = x_t->shape[0];
    uint32_t concat_shape[] = {batch, lstm->input_size + lstm->hidden_size};
    rtka_tensor_t* concat = rtka_tensor_create(concat_shape, 2);
    if (!concat) return false;
    
    /* Copy x_t and h_t into concatenated tensor */
    for (uint32_t b = 0; b < batch; b++) {
        for (uint32_t i = 0; i < lstm->input_size; i++) {
            uint32_t src_idx[] = {b, i};
            uint32_t dst_idx[] = {b, i};
            rtka_tensor_set(concat, dst_idx, *rtka_tensor_get(x_t, src_idx));
        }
        for (uint32_t i = 0; i < lstm->hidden_size; i++) {
            uint32_t src_idx[] = {b, i};
            uint32_t dst_idx[] = {b, lstm->input_size + i};
            rtka_tensor_set(concat, dst_idx, *rtka_tensor_get(h_t, src_idx));
        }
    }
    
    /* Compute gates */
    rtka_tensor_t* i_t = matmul_add_bias(concat, lstm->gate_i.weight->data, 
                                         lstm->gate_i.bias->data);
    rtka_tensor_t* f_t = matmul_add_bias(concat, lstm->gate_f.weight->data, 
                                         lstm->gate_f.bias->data);
    rtka_tensor_t* g_t = matmul_add_bias(concat, lstm->gate_g.weight->data, 
                                         lstm->gate_g.bias->data);
    rtka_tensor_t* o_t = matmul_add_bias(concat, lstm->gate_o.weight->data, 
                                         lstm->gate_o.bias->data);
    
    rtka_tensor_free(concat);
    
    if (!i_t || !f_t || !g_t || !o_t) {
        if (i_t) rtka_tensor_free(i_t);
        if (f_t) rtka_tensor_free(f_t);
        if (g_t) rtka_tensor_free(g_t);
        if (o_t) rtka_tensor_free(o_t);
        return false;
    }
    
    /* Apply activations */
    apply_sigmoid_inplace(i_t);  /* Input gate */
    apply_sigmoid_inplace(f_t);  /* Forget gate */
    apply_tanh_inplace(g_t);     /* Cell gate */
    apply_sigmoid_inplace(o_t);  /* Output gate */
    
    /* Compute new cell state: c_next = f_t * c_t + i_t * g_t */
    rtka_tensor_t* forget_term = elemwise_mul(f_t, c_t);
    rtka_tensor_t* input_term = elemwise_mul(i_t, g_t);
    *c_next = elemwise_add(forget_term, input_term);
    
    rtka_tensor_free(forget_term);
    rtka_tensor_free(input_term);
    rtka_tensor_free(i_t);
    rtka_tensor_free(f_t);
    rtka_tensor_free(g_t);
    
    if (!*c_next) {
        rtka_tensor_free(o_t);
        return false;
    }
    
    /* Compute new hidden state: h_next = o_t * tanh(c_next) */
    rtka_tensor_t* c_next_tanh = rtka_tensor_create((*c_next)->shape, (*c_next)->ndim);
    if (!c_next_tanh) {
        rtka_tensor_free(*c_next);
        rtka_tensor_free(o_t);
        return false;
    }
    
    memcpy(c_next_tanh->data, (*c_next)->data, (*c_next)->size * sizeof(rtka_state_t));
    apply_tanh_inplace(c_next_tanh);
    
    *h_next = elemwise_mul(o_t, c_next_tanh);
    
    rtka_tensor_free(c_next_tanh);
    rtka_tensor_free(o_t);
    
    if (!*h_next) {
        rtka_tensor_free(*c_next);
        return false;
    }
    
    return true;
}

rtka_lstm_output_t rtka_lstm_forward(rtka_lstm_layer_t* lstm,
                                     rtka_grad_node_t* input,
                                     rtka_tensor_t* h_0,
                                     rtka_tensor_t* c_0) {
    rtka_lstm_output_t result = {NULL, NULL, NULL};
    
    if (!lstm || !input || !input->data) return result;
    
    rtka_tensor_t* input_data = input->data;
    uint32_t batch = input_data->shape[0];
    uint32_t seq_len = input_data->shape[1];
    
    /* Initialize hidden states if not provided */
    rtka_tensor_t* h_t = h_0 ? h_0 : lstm->h_prev;
    rtka_tensor_t* c_t = c_0 ? c_0 : lstm->c_prev;
    
    if (!h_t || !c_t) {
        if (!rtka_lstm_init_hidden(lstm, batch)) return result;
        h_t = lstm->h_prev;
        c_t = lstm->c_prev;
    }
    
    /* Allocate output sequence tensor */
    uint32_t out_shape[] = {batch, seq_len, lstm->hidden_size};
    rtka_tensor_t* output_seq = rtka_tensor_create(out_shape, 3);
    if (!output_seq) return result;
    
    /* Process sequence */
    for (uint32_t t = 0; t < seq_len; t++) {
        /* Extract timestep t */
        uint32_t x_t_shape[] = {batch, lstm->input_size};
        rtka_tensor_t* x_t = rtka_tensor_create(x_t_shape, 2);
        if (!x_t) {
            rtka_tensor_free(output_seq);
            return result;
        }
        
        for (uint32_t b = 0; b < batch; b++) {
            for (uint32_t i = 0; i < lstm->input_size; i++) {
                uint32_t src_idx[] = {b, t, i};
                uint32_t dst_idx[] = {b, i};
                rtka_tensor_set(x_t, dst_idx, *rtka_tensor_get(input_data, src_idx));
            }
        }
        
        /* LSTM cell forward */
        rtka_tensor_t* h_next = NULL;
        rtka_tensor_t* c_next = NULL;
        
        if (!rtka_lstm_cell_forward(lstm, x_t, h_t, c_t, &h_next, &c_next)) {
            rtka_tensor_free(x_t);
            rtka_tensor_free(output_seq);
            return result;
        }
        
        rtka_tensor_free(x_t);
        
        /* Copy h_next to output sequence */
        for (uint32_t b = 0; b < batch; b++) {
            for (uint32_t i = 0; i < lstm->hidden_size; i++) {
                uint32_t src_idx[] = {b, i};
                uint32_t dst_idx[] = {b, t, i};
                rtka_tensor_set(output_seq, dst_idx, *rtka_tensor_get(h_next, src_idx));
            }
        }
        
        /* Update states for next iteration */
        if (t > 0 || h_0 == NULL) {
            if (h_t != h_0 && h_t != lstm->h_prev) rtka_tensor_free(h_t);
            if (c_t != c_0 && c_t != lstm->c_prev) rtka_tensor_free(c_t);
        }
        h_t = h_next;
        c_t = c_next;
    }
    
    /* Create gradient node for output */
    result.output = rtka_grad_node_create(output_seq, true);
    result.hidden_state = h_t;
    result.cell_state = c_t;
    
    /* Update layer's hidden states */
    if (lstm->h_prev && lstm->h_prev != h_0) rtka_tensor_free(lstm->h_prev);
    if (lstm->c_prev && lstm->c_prev != c_0) rtka_tensor_free(lstm->c_prev);
    
    lstm->h_prev = rtka_tensor_create(h_t->shape, h_t->ndim);
    lstm->c_prev = rtka_tensor_create(c_t->shape, c_t->ndim);
    
    if (lstm->h_prev && lstm->c_prev) {
        memcpy(lstm->h_prev->data, h_t->data, h_t->size * sizeof(rtka_state_t));
        memcpy(lstm->c_prev->data, c_t->data, c_t->size * sizeof(rtka_state_t));
    }
    
    return result;
}

void rtka_lstm_free(rtka_lstm_layer_t* lstm) {
    if (!lstm) return;
    
    /* Free gate parameters */
    if (lstm->gate_i.weight) rtka_grad_node_free(lstm->gate_i.weight);
    if (lstm->gate_i.bias) rtka_grad_node_free(lstm->gate_i.bias);
    if (lstm->gate_f.weight) rtka_grad_node_free(lstm->gate_f.weight);
    if (lstm->gate_f.bias) rtka_grad_node_free(lstm->gate_f.bias);
    if (lstm->gate_g.weight) rtka_grad_node_free(lstm->gate_g.weight);
    if (lstm->gate_g.bias) rtka_grad_node_free(lstm->gate_g.bias);
    if (lstm->gate_o.weight) rtka_grad_node_free(lstm->gate_o.weight);
    if (lstm->gate_o.bias) rtka_grad_node_free(lstm->gate_o.bias);
    
    /* Free hidden states */
    if (lstm->h_prev) rtka_tensor_free(lstm->h_prev);
    if (lstm->c_prev) rtka_tensor_free(lstm->c_prev);
    
    free(lstm);
}

uint32_t rtka_lstm_param_count(const rtka_lstm_layer_t* lstm) {
    if (!lstm) return 0;
    
    /* Each gate has (input_size + hidden_size) * hidden_size weights */
    /* Plus hidden_size biases */
    uint32_t weights_per_gate = (lstm->input_size + lstm->hidden_size) * lstm->hidden_size;
    uint32_t bias_per_gate = lstm->hidden_size;
    uint32_t params_per_gate = weights_per_gate + bias_per_gate;
    
    /* 4 gates total */
    return 4 * params_per_gate;
}
