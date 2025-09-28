/**
 * File: rtka_nn.h
 * Copyright (c) 2025 - H.Overman opsec.ee@pm.me
 * 
 * PROPRIETARY AND CONFIDENTIAL
 * This file is proprietary to H. Overman and may not be reproduced,
 * distributed, or used without explicit written permission.
 *
 * For licensing inquiries: opsec.ee@pm.me
 *
 * RTKA Neural Network Layers
 */

#ifndef RTKA_NN_H
#define RTKA_NN_H

#include "rtka_types.h"
#include "rtka_tensor.h"
#include "rtka_gradient.h"
#include <math.h>

/* Layer types */
typedef enum {
    LAYER_LINEAR,
    LAYER_TERNARY,
    LAYER_CONV2D,
    LAYER_POOL,
    LAYER_DROPOUT,
    LAYER_BATCHNORM,
    LAYER_ACTIVATION
} rtka_layer_type_t;

/* Base layer structure */
typedef struct rtka_layer {
    rtka_layer_type_t type;
    rtka_grad_node_t* (*forward)(struct rtka_layer*, rtka_grad_node_t*);
    void (*reset_parameters)(struct rtka_layer*);
    
    /* Parameters */
    rtka_grad_node_t* weight;
    rtka_grad_node_t* bias;
    
    /* Configuration */
    uint32_t in_features;
    uint32_t out_features;
    bool training;
    
    /* Layer-specific data */
    void* layer_data;
} rtka_layer_t;

/* Linear layer */
typedef struct {
    rtka_layer_t base;
    bool use_bias;
} rtka_linear_layer_t;

/* Ternary layer - optimized for ternary weights */
typedef struct {
    rtka_layer_t base;
    rtka_confidence_t threshold;
    bool quantize_activations;
} rtka_ternary_layer_t;

/* Convolutional layer */
typedef struct {
    rtka_layer_t base;
    uint32_t kernel_size[2];
    uint32_t stride[2];
    uint32_t padding[2];
    uint32_t in_channels;
    uint32_t out_channels;
} rtka_conv2d_layer_t;

/* Activation functions for ternary logic */
typedef enum {
    ACT_TERNARY_SIGN,    /* Maps to -1, 0, 1 */
    ACT_TERNARY_SIGMOID,  /* Smooth ternary */
    ACT_TERNARY_TANH,
    ACT_IDENTITY
} rtka_activation_t;

/* Layer creation */
rtka_linear_layer_t* rtka_nn_linear(uint32_t in_features, uint32_t out_features, bool bias);
rtka_ternary_layer_t* rtka_nn_ternary(uint32_t in_features, uint32_t out_features, rtka_confidence_t threshold);
rtka_conv2d_layer_t* rtka_nn_conv2d(uint32_t in_channels, uint32_t out_channels, 
                                    uint32_t kernel_size, uint32_t stride, uint32_t padding);

/* Forward pass */
rtka_grad_node_t* rtka_nn_forward(rtka_layer_t* layer, rtka_grad_node_t* input);

/* Activation functions */
rtka_grad_node_t* rtka_nn_ternary_activation(rtka_grad_node_t* input, rtka_activation_t type, rtka_confidence_t threshold);

/* Ternary-specific activation - smooth approximation */
RTKA_INLINE rtka_state_t rtka_ternary_sigmoid(rtka_state_t x, rtka_confidence_t threshold) {
    rtka_confidence_t conf = x.confidence;
    
    if (conf > threshold) {
        return rtka_make_state(RTKA_TRUE, tanhf(conf));
    } else if (conf < -threshold) {
        return rtka_make_state(RTKA_FALSE, tanhf(-conf));
    } else {
        return rtka_make_state(RTKA_UNKNOWN, fabsf(conf) / threshold);
    }
}

/* Dropout for ternary networks */
rtka_grad_node_t* rtka_nn_ternary_dropout(rtka_grad_node_t* input, rtka_confidence_t p, bool training);

/* Batch normalization for ternary */
typedef struct {
    rtka_layer_t base;
    rtka_tensor_t* running_mean;
    rtka_tensor_t* running_var;
    rtka_confidence_t momentum;
    rtka_confidence_t epsilon;
} rtka_batchnorm_layer_t;

rtka_batchnorm_layer_t* rtka_nn_batchnorm(uint32_t num_features, rtka_confidence_t momentum);

/* Sequential model */
typedef struct {
    rtka_layer_t** layers;
    uint32_t num_layers;
    uint32_t capacity;
} rtka_sequential_t;

rtka_sequential_t* rtka_nn_sequential(void);
void rtka_nn_sequential_add(rtka_sequential_t* model, rtka_layer_t* layer);
rtka_grad_node_t* rtka_nn_sequential_forward(rtka_sequential_t* model, rtka_grad_node_t* input);

/* Weight initialization for ternary networks */
void rtka_nn_init_ternary_uniform(rtka_tensor_t* weight, rtka_confidence_t threshold);
void rtka_nn_init_ternary_normal(rtka_tensor_t* weight, rtka_confidence_t std);

/* Loss functions for ternary outputs */
rtka_confidence_t rtka_nn_ternary_cross_entropy(rtka_grad_node_t* output, rtka_tensor_t* target);
rtka_confidence_t rtka_nn_ternary_mse(rtka_grad_node_t* output, rtka_tensor_t* target);

#endif /* RTKA_NN_H */
