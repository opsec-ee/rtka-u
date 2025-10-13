/**
 * File: rtka_mdn.h
 * Copyright (c) 2025 - H.Overman opsec.ee@pm.me
 * Email: opsec.ee@pm.me
 * 
 * RTKA Mixture Density Network (MDN) Implementation
 * Provides probabilistic outputs via mixture of Gaussians
 * 
 * CHANGELOG:
 * v1.0.0 - 2025-10-12: Initial implementation
 *   - Mixture of Gaussians output layer
 *   - Parameter splitting (pi, mu, sigma)
 *   - Softmax for mixture weights
 *   - Log-sigma to sigma conversion
 *   - Integration with LSTM outputs
 */

#ifndef RTKA_MDN_H
#define RTKA_MDN_H

#include "rtka_types.h"
#include "rtka_tensor.h"
#include "rtka_gradient.h"
#include <stdint.h>
#include <stdbool.h>

/* MDN layer structure */
typedef struct {
    uint32_t input_size;
    uint32_t output_size;      /* Dimension of output space (z_size) */
    uint32_t num_gaussians;    /* K gaussians in mixture */
    
    /* Linear transformation to MDN parameters */
    rtka_grad_node_t* fc_weight;  /* (input_size, num_params) */
    rtka_grad_node_t* fc_bias;    /* (num_params,) */
    
    /* num_params = K + K*output_size + K*output_size */
    /*             = K * (1 + 2*output_size) */
    /* pi: K mixture weights */
    /* mu: K * output_size means */
    /* log_sigma: K * output_size log standard deviations */
    
    bool initialized;
} rtka_mdn_layer_t;

/* MDN output structure */
typedef struct {
    rtka_tensor_t* pi;         /* (batch, num_gaussians) - mixture weights */
    rtka_tensor_t* mu;         /* (batch, num_gaussians, output_size) - means */
    rtka_tensor_t* sigma;      /* (batch, num_gaussians, output_size) - std devs */
} rtka_mdn_output_t;

/**
 * Create MDN layer
 * 
 * @param input_size     Size of input features (typically hidden_size from LSTM)
 * @param output_size    Dimension of output space (z_size)
 * @param num_gaussians  Number of Gaussian components in mixture
 * @return Pointer to initialized MDN layer or NULL on error
 */
rtka_mdn_layer_t* rtka_mdn_create(uint32_t input_size,
                                  uint32_t output_size,
                                  uint32_t num_gaussians);

/**
 * Initialize MDN parameters with Xavier/Glorot initialization
 * 
 * @param mdn MDN layer to initialize
 */
void rtka_mdn_reset_parameters(rtka_mdn_layer_t* mdn);

/**
 * Forward pass through MDN
 * Splits output into pi, mu, sigma parameters
 * 
 * @param mdn   MDN layer
 * @param input Input tensor (batch, input_size)
 * @return MDN output structure with pi, mu, sigma or NULL components on error
 */
rtka_mdn_output_t rtka_mdn_forward(rtka_mdn_layer_t* mdn, 
                                   rtka_grad_node_t* input);

/**
 * Split MDN parameters from linear output
 * Internal function for parameter extraction
 * 
 * @param params      Raw parameter tensor (batch, num_params)
 * @param num_gaussians Number of Gaussian components
 * @param output_size   Dimension of output space
 * @return MDN output structure
 */
rtka_mdn_output_t rtka_mdn_split_params(rtka_tensor_t* params,
                                        uint32_t num_gaussians,
                                        uint32_t output_size);

/**
 * Apply softmax to mixture weights
 * Ensures pi sums to 1.0 across gaussians
 * 
 * @param pi Mixture weight tensor to normalize (batch, num_gaussians)
 */
void rtka_mdn_softmax_pi(rtka_tensor_t* pi);

/**
 * Convert log_sigma to sigma
 * Applies exp() to ensure positive standard deviations
 * 
 * @param log_sigma Input tensor with log(sigma) values
 * @return Tensor with sigma = exp(log_sigma)
 */
rtka_tensor_t* rtka_mdn_exp_sigma(rtka_tensor_t* log_sigma);

/**
 * Free MDN layer and all associated resources
 * 
 * @param mdn MDN layer to free
 */
void rtka_mdn_free(rtka_mdn_layer_t* mdn);

/**
 * Free MDN output structure
 * 
 * @param output MDN output to free
 */
void rtka_mdn_output_free(rtka_mdn_output_t* output);

/**
 * Get parameter count for MDN layer
 * 
 * @param mdn MDN layer
 * @return Total number of parameters
 */
uint32_t rtka_mdn_param_count(const rtka_mdn_layer_t* mdn);

/**
 * Sample from mixture of Gaussians
 * Selects gaussian according to pi, then samples from that gaussian
 * 
 * @param output MDN output with pi, mu, sigma
 * @param batch_idx Index in batch to sample from
 * @return Sampled output vector or NULL on error
 */
rtka_tensor_t* rtka_mdn_sample(const rtka_mdn_output_t* output, uint32_t batch_idx);

#endif /* RTKA_MDN_H */
