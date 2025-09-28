/**
 * File: rtka_optimizer.h
 * Copyright (c) 2025 - H.Overman opsec.ee@pm.me
 * 
 * PROPRIETARY AND CONFIDENTIAL
 * This file is proprietary to H. Overman and may not be reproduced,
 * distributed, or used without explicit written permission.
 *
 * For licensing inquiries: opsec.ee@pm.me
 *
 * RTKA Optimization Algorithms for ML
 */

#ifndef RTKA_OPTIMIZER_H
#define RTKA_OPTIMIZER_H

#include "rtka_types.h"
#include "rtka_tensor.h"
#include "rtka_gradient.h"

/* Optimizer types */
typedef enum {
    OPT_SGD,
    OPT_ADAM,
    OPT_RMSPROP,
    OPT_ADAGRAD,
    OPT_TERNARY  /* Special ternary-aware optimizer */
} rtka_optimizer_type_t;

/* Optimizer state */
typedef struct {
    rtka_optimizer_type_t type;
    rtka_confidence_t learning_rate;
    rtka_confidence_t momentum;
    rtka_confidence_t weight_decay;
    
    /* Adam parameters */
    rtka_confidence_t beta1;
    rtka_confidence_t beta2;
    rtka_confidence_t epsilon;
    
    /* State tensors */
    rtka_tensor_t** momentum_buffers;
    rtka_tensor_t** velocity_buffers;
    uint32_t buffer_count;
    uint32_t step;
    
    /* Ternary-specific */
    rtka_confidence_t confidence_threshold;
    bool use_ternary_quantization;
} rtka_optimizer_t;

/* Learning rate scheduler */
typedef struct {
    rtka_confidence_t initial_lr;
    rtka_confidence_t min_lr;
    uint32_t warmup_steps;
    uint32_t decay_steps;
    
    enum {
        LR_CONSTANT,
        LR_LINEAR,
        LR_COSINE,
        LR_EXPONENTIAL,
        LR_STEP
    } schedule_type;
    
    rtka_confidence_t decay_factor;
    uint32_t current_step;
} rtka_lr_scheduler_t;

/* Optimizer creation */
rtka_optimizer_t* rtka_optimizer_sgd(rtka_confidence_t lr, rtka_confidence_t momentum);
rtka_optimizer_t* rtka_optimizer_adam(rtka_confidence_t lr, rtka_confidence_t beta1, rtka_confidence_t beta2);
rtka_optimizer_t* rtka_optimizer_rmsprop(rtka_confidence_t lr, rtka_confidence_t alpha);
rtka_optimizer_t* rtka_optimizer_ternary(rtka_confidence_t lr, rtka_confidence_t threshold);
void rtka_optimizer_free(rtka_optimizer_t* opt);

/* Parameter update */
void rtka_optimizer_step(rtka_optimizer_t* opt, rtka_grad_node_t** parameters, uint32_t param_count);
void rtka_optimizer_zero_grad(rtka_optimizer_t* opt, rtka_grad_node_t** parameters, uint32_t param_count);

/* Ternary-specific optimization */
void rtka_optimizer_quantize_gradients(rtka_tensor_t* grad, rtka_confidence_t threshold);
void rtka_optimizer_apply_ternary_constraint(rtka_tensor_t* params);

/* Learning rate scheduling */
rtka_lr_scheduler_t* rtka_scheduler_create(rtka_confidence_t initial_lr, uint32_t schedule_type);
rtka_confidence_t rtka_scheduler_get_lr(rtka_lr_scheduler_t* scheduler);
void rtka_scheduler_step(rtka_lr_scheduler_t* scheduler);
void rtka_scheduler_free(rtka_lr_scheduler_t* scheduler);

/* Optimization utilities */
void rtka_clip_grad_norm(rtka_grad_node_t** parameters, uint32_t count, rtka_confidence_t max_norm);
rtka_confidence_t rtka_compute_grad_norm(rtka_grad_node_t** parameters, uint32_t count);

/* Momentum updates for ternary states */
RTKA_INLINE void rtka_apply_momentum_ternary(rtka_state_t* param, rtka_state_t* momentum,
                                             rtka_confidence_t grad, rtka_confidence_t beta) {
    /* Update momentum with confidence weighting */
    momentum->confidence = beta * momentum->confidence + (1.0f - beta) * grad;
    
    /* Update parameter confidence */
    param->confidence -= momentum->confidence;
    
    /* Quantize to ternary if confidence crosses threshold */
    if (param->confidence > 0.9f) {
        param->value = RTKA_TRUE;
    } else if (param->confidence < 0.1f) {
        param->value = RTKA_FALSE;
    } else {
        param->value = RTKA_UNKNOWN;
    }
}

#endif /* RTKA_OPTIMIZER_H */
