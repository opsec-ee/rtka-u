/**
 * File: rtka_mdn.c
 * Copyright (c) 2025 - H.Overman opsec.ee@pm.me
 * Email: opsec.ee@pm.me
 * 
 * RTKA Mixture Density Network Implementation
 * 
 * CHANGELOG:
 * v1.0.0 - 2025-10-12: Initial implementation
 *   - Parameter splitting and normalization
 *   - Softmax for mixture weights
 *   - Exponential transform for sigmas
 *   - Gaussian mixture sampling
 */

#include "rtka_mdn.h"
#include "rtka_memory.h"
#include <math.h>
#include <string.h>
#include <stdlib.h>

/* Xavier initialization helper */
static void init_xavier(rtka_tensor_t* tensor, uint32_t fan_in, uint32_t fan_out) {
    rtka_confidence_t limit = sqrtf(6.0f / (fan_in + fan_out));
    
    for (uint32_t i = 0; i < tensor->size; i++) {
        rtka_confidence_t value = ((rtka_confidence_t)rand() / RAND_MAX) * 2.0f * limit - limit;
        tensor->data[i].value = (value > 0.0f) ? RTKA_TRUE : 
                               (value < 0.0f) ? RTKA_FALSE : RTKA_UNKNOWN;
        tensor->data[i].confidence = fabsf(value);
    }
}

rtka_mdn_layer_t* rtka_mdn_create(uint32_t input_size,
                                  uint32_t output_size,
                                  uint32_t num_gaussians) {
    if (input_size == 0 || output_size == 0 || num_gaussians == 0) return NULL;
    
    rtka_mdn_layer_t* mdn = (rtka_mdn_layer_t*)calloc(1, sizeof(rtka_mdn_layer_t));
    if (!mdn) return NULL;
    
    mdn->input_size = input_size;
    mdn->output_size = output_size;
    mdn->num_gaussians = num_gaussians;
    mdn->initialized = false;
    
    /* Calculate total number of output parameters */
    /* pi: K, mu: K*output_size, log_sigma: K*output_size */
    uint32_t num_params = num_gaussians * (1 + 2 * output_size);
    
    /* Create weight and bias */
    uint32_t weight_shape[] = {input_size, num_params};
    rtka_tensor_t* weight_data = rtka_tensor_create(weight_shape, 2);
    if (!weight_data) {
        free(mdn);
        return NULL;
    }
    
    mdn->fc_weight = rtka_grad_node_create(weight_data, true);
    if (!mdn->fc_weight) {
        rtka_tensor_free(weight_data);
        free(mdn);
        return NULL;
    }
    
    uint32_t bias_shape[] = {num_params};
    rtka_tensor_t* bias_data = rtka_tensor_create(bias_shape, 1);
    if (!bias_data) {
        rtka_grad_node_free(mdn->fc_weight);
        free(mdn);
        return NULL;
    }
    
    mdn->fc_bias = rtka_grad_node_create(bias_data, true);
    if (!mdn->fc_bias) {
        rtka_tensor_free(bias_data);
        rtka_grad_node_free(mdn->fc_weight);
        free(mdn);
        return NULL;
    }
    
    /* Initialize parameters */
    init_xavier(weight_data, input_size, num_params);
    memset(bias_data->data, 0, bias_data->size * sizeof(rtka_state_t));
    
    mdn->initialized = true;
    return mdn;
}

void rtka_mdn_reset_parameters(rtka_mdn_layer_t* mdn) {
    if (!mdn || !mdn->fc_weight || !mdn->fc_bias) return;
    
    uint32_t num_params = mdn->num_gaussians * (1 + 2 * mdn->output_size);
    init_xavier(mdn->fc_weight->data, mdn->input_size, num_params);
    memset(mdn->fc_bias->data->data, 0, 
           mdn->fc_bias->data->size * sizeof(rtka_state_t));
}

/* Matrix multiplication helper */
static rtka_tensor_t* matmul_add_bias(rtka_tensor_t* input, 
                                      rtka_tensor_t* weight,
                                      rtka_tensor_t* bias) {
    if (!input || !weight) return NULL;
    
    uint32_t batch = input->shape[0];
    uint32_t input_dim = input->shape[1];
    uint32_t output_dim = weight->shape[1];
    
    uint32_t out_shape[] = {batch, output_dim};
    rtka_tensor_t* output = rtka_tensor_create(out_shape, 2);
    if (!output) return NULL;
    
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

void rtka_mdn_softmax_pi(rtka_tensor_t* pi) {
    if (!pi || pi->ndim != 2) return;
    
    uint32_t batch = pi->shape[0];
    uint32_t num_gaussians = pi->shape[1];
    
    for (uint32_t b = 0; b < batch; b++) {
        /* Find max for numerical stability */
        rtka_confidence_t max_val = -INFINITY;
        for (uint32_t k = 0; k < num_gaussians; k++) {
            uint32_t idx[] = {b, k};
            rtka_state_t* val = rtka_tensor_get(pi, idx);
            rtka_confidence_t logit = val->confidence * (rtka_confidence_t)val->value;
            if (logit > max_val) max_val = logit;
        }
        
        /* Compute exp(x - max) and sum */
        rtka_confidence_t sum = 0.0f;
        rtka_confidence_t exp_vals[64];  /* Assuming num_gaussians <= 64 */
        
        for (uint32_t k = 0; k < num_gaussians; k++) {
            uint32_t idx[] = {b, k};
            rtka_state_t* val = rtka_tensor_get(pi, idx);
            rtka_confidence_t logit = val->confidence * (rtka_confidence_t)val->value;
            exp_vals[k] = expf(logit - max_val);
            sum += exp_vals[k];
        }
        
        /* Normalize */
        for (uint32_t k = 0; k < num_gaussians; k++) {
            uint32_t idx[] = {b, k};
            rtka_confidence_t prob = exp_vals[k] / sum;
            rtka_tensor_set(pi, idx, rtka_make_state(RTKA_TRUE, prob));
        }
    }
}

rtka_tensor_t* rtka_mdn_exp_sigma(rtka_tensor_t* log_sigma) {
    if (!log_sigma) return NULL;
    
    rtka_tensor_t* sigma = rtka_tensor_create(log_sigma->shape, log_sigma->ndim);
    if (!sigma) return NULL;
    
    for (uint32_t i = 0; i < log_sigma->size; i++) {
        rtka_confidence_t log_val = log_sigma->data[i].confidence * 
                                    (rtka_confidence_t)log_sigma->data[i].value;
        rtka_confidence_t sig_val = expf(log_val);
        
        sigma->data[i] = rtka_make_state(RTKA_TRUE, sig_val);
    }
    
    return sigma;
}

rtka_mdn_output_t rtka_mdn_split_params(rtka_tensor_t* params,
                                        uint32_t num_gaussians,
                                        uint32_t output_size) {
    rtka_mdn_output_t result = {NULL, NULL, NULL};
    
    if (!params || params->ndim != 2) return result;
    
    uint32_t batch = params->shape[0];
    uint32_t K = num_gaussians;
    uint32_t Z = output_size;
    
    /* Create output tensors */
    uint32_t pi_shape[] = {batch, K};
    result.pi = rtka_tensor_create(pi_shape, 2);
    if (!result.pi) return result;
    
    uint32_t mu_shape[] = {batch, K, Z};
    result.mu = rtka_tensor_create(mu_shape, 3);
    if (!result.mu) {
        rtka_tensor_free(result.pi);
        result.pi = NULL;
        return result;
    }
    
    uint32_t sigma_shape[] = {batch, K, Z};
    rtka_tensor_t* log_sigma = rtka_tensor_create(sigma_shape, 3);
    if (!log_sigma) {
        rtka_tensor_free(result.pi);
        rtka_tensor_free(result.mu);
        result.pi = NULL;
        result.mu = NULL;
        return result;
    }
    
    /* Extract parameters */
    for (uint32_t b = 0; b < batch; b++) {
        uint32_t offset = 0;
        
        /* Extract pi (K values) */
        for (uint32_t k = 0; k < K; k++) {
            uint32_t param_idx[] = {b, offset + k};
            uint32_t pi_idx[] = {b, k};
            rtka_tensor_set(result.pi, pi_idx, *rtka_tensor_get(params, param_idx));
        }
        offset += K;
        
        /* Extract mu (K * Z values) */
        for (uint32_t k = 0; k < K; k++) {
            for (uint32_t z = 0; z < Z; z++) {
                uint32_t param_idx[] = {b, offset + k * Z + z};
                uint32_t mu_idx[] = {b, k, z};
                rtka_tensor_set(result.mu, mu_idx, *rtka_tensor_get(params, param_idx));
            }
        }
        offset += K * Z;
        
        /* Extract log_sigma (K * Z values) */
        for (uint32_t k = 0; k < K; k++) {
            for (uint32_t z = 0; z < Z; z++) {
                uint32_t param_idx[] = {b, offset + k * Z + z};
                uint32_t sigma_idx[] = {b, k, z};
                rtka_tensor_set(log_sigma, sigma_idx, *rtka_tensor_get(params, param_idx));
            }
        }
    }
    
    /* Apply softmax to pi */
    rtka_mdn_softmax_pi(result.pi);
    
    /* Convert log_sigma to sigma */
    result.sigma = rtka_mdn_exp_sigma(log_sigma);
    rtka_tensor_free(log_sigma);
    
    if (!result.sigma) {
        rtka_tensor_free(result.pi);
        rtka_tensor_free(result.mu);
        result.pi = NULL;
        result.mu = NULL;
    }
    
    return result;
}

rtka_mdn_output_t rtka_mdn_forward(rtka_mdn_layer_t* mdn, 
                                   rtka_grad_node_t* input) {
    rtka_mdn_output_t result = {NULL, NULL, NULL};
    
    if (!mdn || !mdn->initialized || !input || !input->data) return result;
    
    /* Linear transformation */
    rtka_tensor_t* params = matmul_add_bias(input->data, 
                                            mdn->fc_weight->data,
                                            mdn->fc_bias->data);
    if (!params) return result;
    
    /* Split into pi, mu, sigma */
    result = rtka_mdn_split_params(params, mdn->num_gaussians, mdn->output_size);
    
    rtka_tensor_free(params);
    return result;
}

void rtka_mdn_free(rtka_mdn_layer_t* mdn) {
    if (!mdn) return;
    
    if (mdn->fc_weight) rtka_grad_node_free(mdn->fc_weight);
    if (mdn->fc_bias) rtka_grad_node_free(mdn->fc_bias);
    
    free(mdn);
}

void rtka_mdn_output_free(rtka_mdn_output_t* output) {
    if (!output) return;
    
    if (output->pi) rtka_tensor_free(output->pi);
    if (output->mu) rtka_tensor_free(output->mu);
    if (output->sigma) rtka_tensor_free(output->sigma);
    
    output->pi = NULL;
    output->mu = NULL;
    output->sigma = NULL;
}

uint32_t rtka_mdn_param_count(const rtka_mdn_layer_t* mdn) {
    if (!mdn) return 0;
    
    uint32_t num_params = mdn->num_gaussians * (1 + 2 * mdn->output_size);
    return mdn->input_size * num_params + num_params;  /* weights + biases */
}

rtka_tensor_t* rtka_mdn_sample(const rtka_mdn_output_t* output, uint32_t batch_idx) {
    if (!output || !output->pi || !output->mu || !output->sigma) return NULL;
    
    if (batch_idx >= output->pi->shape[0]) return NULL;
    
    uint32_t K = output->pi->shape[1];
    uint32_t Z = output->mu->shape[2];
    
    /* Sample gaussian index according to pi */
    rtka_confidence_t rand_val = (rtka_confidence_t)rand() / RAND_MAX;
    rtka_confidence_t cumsum = 0.0f;
    uint32_t selected_k = 0;
    
    for (uint32_t k = 0; k < K; k++) {
        uint32_t pi_idx[] = {batch_idx, k};
        rtka_state_t* pi_val = rtka_tensor_get(output->pi, pi_idx);
        cumsum += pi_val->confidence;
        if (rand_val <= cumsum) {
            selected_k = k;
            break;
        }
    }
    
    /* Create output vector */
    uint32_t out_shape[] = {Z};
    rtka_tensor_t* sample = rtka_tensor_create(out_shape, 1);
    if (!sample) return NULL;
    
    /* Sample from selected Gaussian */
    for (uint32_t z = 0; z < Z; z++) {
        uint32_t mu_idx[] = {batch_idx, selected_k, z};
        uint32_t sigma_idx[] = {batch_idx, selected_k, z};
        
        rtka_state_t* mu_val = rtka_tensor_get(output->mu, mu_idx);
        rtka_state_t* sigma_val = rtka_tensor_get(output->sigma, sigma_idx);
        
        rtka_confidence_t mu = mu_val->confidence * (rtka_confidence_t)mu_val->value;
        rtka_confidence_t sigma = sigma_val->confidence;
        
        /* Box-Muller transform for normal distribution */
        rtka_confidence_t u1 = (rtka_confidence_t)rand() / RAND_MAX;
        rtka_confidence_t u2 = (rtka_confidence_t)rand() / RAND_MAX;
        rtka_confidence_t z0 = sqrtf(-2.0f * logf(u1)) * cosf(2.0f * M_PI * u2);
        
        rtka_confidence_t sampled_val = mu + sigma * z0;
        
        uint32_t out_idx[] = {z};
        rtka_tensor_set(sample, out_idx, rtka_make_state(
            sampled_val > 0.0f ? RTKA_TRUE : sampled_val < 0.0f ? RTKA_FALSE : RTKA_UNKNOWN,
            fabsf(sampled_val)
        ));
    }
    
    return sample;
}
