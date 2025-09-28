/**
 * File: rtka_optimizer.c
 * Copyright (c) 2025 - H.Overman opsec.ee@pm.me
 * 
 * PROPRIETARY AND CONFIDENTIAL
 * This file is proprietary to H. Overman and may not be reproduced,
 * distributed, or used without explicit written permission.
 *
 * For licensing inquiries: opsec.ee@pm.me
 *
 * RTKA Optimizer Implementation
 */

#define _GNU_SOURCE  /* For M_PI */
#include "rtka_optimizer.h"
#include "rtka_memory.h"
#include <math.h>
#include <string.h>

/* Create SGD optimizer */
rtka_optimizer_t* rtka_optimizer_sgd(rtka_confidence_t lr, rtka_confidence_t momentum) {
    rtka_optimizer_t* opt = (rtka_optimizer_t*)rtka_alloc_state();
    if (!opt) return NULL;
    
    opt->type = OPT_SGD;
    opt->learning_rate = lr;
    opt->momentum = momentum;
    opt->weight_decay = 0.0f;
    opt->step = 0;
    opt->buffer_count = 0;
    opt->momentum_buffers = NULL;
    
    return opt;
}

/* Create Adam optimizer */
rtka_optimizer_t* rtka_optimizer_adam(rtka_confidence_t lr, rtka_confidence_t beta1, rtka_confidence_t beta2) {
    rtka_optimizer_t* opt = (rtka_optimizer_t*)rtka_alloc_state();
    if (!opt) return NULL;
    
    opt->type = OPT_ADAM;
    opt->learning_rate = lr;
    opt->beta1 = beta1;
    opt->beta2 = beta2;
    opt->epsilon = 1e-8f;
    opt->step = 0;
    opt->buffer_count = 0;
    opt->momentum_buffers = NULL;
    opt->velocity_buffers = NULL;
    
    return opt;
}

/* Create ternary optimizer */
rtka_optimizer_t* rtka_optimizer_ternary(rtka_confidence_t lr, rtka_confidence_t threshold) {
    rtka_optimizer_t* opt = (rtka_optimizer_t*)rtka_alloc_state();
    if (!opt) return NULL;
    
    opt->type = OPT_TERNARY;
    opt->learning_rate = lr;
    opt->confidence_threshold = threshold;
    opt->use_ternary_quantization = true;
    opt->step = 0;
    
    return opt;
}

/* SGD update step */
static void sgd_step(rtka_optimizer_t* opt, rtka_tensor_t* param, rtka_tensor_t* grad, uint32_t idx) {
    /* Initialize momentum buffer if needed */
    if (opt->momentum > 0 && !opt->momentum_buffers[idx]) {
        opt->momentum_buffers[idx] = rtka_tensor_zeros(param->shape, param->ndim);
    }
    
    for (uint32_t i = 0; i < param->size; i++) {
        rtka_confidence_t g = grad->data[i].confidence;
        
        if (opt->weight_decay > 0) {
            g += opt->weight_decay * param->data[i].confidence;
        }
        
        if (opt->momentum > 0) {
            rtka_confidence_t* m = &opt->momentum_buffers[idx]->data[i].confidence;
            *m = opt->momentum * (*m) + g;
            g = *m;
        }
        
        param->data[i].confidence -= opt->learning_rate * g;
    }
}

/* Adam update step */
static void adam_step(rtka_optimizer_t* opt, rtka_tensor_t* param, rtka_tensor_t* grad, uint32_t idx) {
    opt->step++;
    
    /* Initialize buffers */
    if (!opt->momentum_buffers[idx]) {
        opt->momentum_buffers[idx] = rtka_tensor_zeros(param->shape, param->ndim);
        opt->velocity_buffers[idx] = rtka_tensor_zeros(param->shape, param->ndim);
    }
    
    /* Bias correction */
    rtka_confidence_t bias_correction1 = 1.0f - powf(opt->beta1, opt->step);
    rtka_confidence_t bias_correction2 = 1.0f - powf(opt->beta2, opt->step);
    
    for (uint32_t i = 0; i < param->size; i++) {
        rtka_confidence_t g = grad->data[i].confidence;
        
        /* Update biased moments */
        rtka_confidence_t* m = &opt->momentum_buffers[idx]->data[i].confidence;
        rtka_confidence_t* v = &opt->velocity_buffers[idx]->data[i].confidence;
        
        *m = opt->beta1 * (*m) + (1.0f - opt->beta1) * g;
        *v = opt->beta2 * (*v) + (1.0f - opt->beta2) * g * g;
        
        /* Bias correction */
        rtka_confidence_t m_hat = *m / bias_correction1;
        rtka_confidence_t v_hat = *v / bias_correction2;
        
        /* Update parameter */
        param->data[i].confidence -= opt->learning_rate * m_hat / (sqrtf(v_hat) + opt->epsilon);
    }
}

/* Ternary optimizer step */
static void ternary_step(rtka_optimizer_t* opt, rtka_tensor_t* param, rtka_tensor_t* grad, uint32_t idx) {
    (void)idx;
    
    for (uint32_t i = 0; i < param->size; i++) {
        rtka_confidence_t g = grad->data[i].confidence;
        
        /* Update confidence */
        param->data[i].confidence -= opt->learning_rate * g;
        
        /* Quantize to ternary states */
        if (param->data[i].confidence > opt->confidence_threshold) {
            param->data[i].value = RTKA_TRUE;
            param->data[i].confidence = 1.0f;
        } else if (param->data[i].confidence < -opt->confidence_threshold) {
            param->data[i].value = RTKA_FALSE;
            param->data[i].confidence = 1.0f;
        } else {
            param->data[i].value = RTKA_UNKNOWN;
            param->data[i].confidence = fabsf(param->data[i].confidence);
        }
    }
}

/* Main optimizer step */
void rtka_optimizer_step(rtka_optimizer_t* opt, rtka_grad_node_t** parameters, uint32_t param_count) {
    /* Allocate buffers if needed */
    if (opt->buffer_count < param_count) {
        opt->momentum_buffers = (rtka_tensor_t**)rtka_alloc_states(param_count * sizeof(void*) / sizeof(rtka_state_t));
        opt->velocity_buffers = (rtka_tensor_t**)rtka_alloc_states(param_count * sizeof(void*) / sizeof(rtka_state_t));
        opt->buffer_count = param_count;
    }
    
    for (uint32_t i = 0; i < param_count; i++) {
        if (!parameters[i]->requires_grad) continue;
        
        rtka_tensor_t* param = parameters[i]->data;
        rtka_tensor_t* grad = parameters[i]->grad;
        
        switch (opt->type) {
            case OPT_SGD:
                sgd_step(opt, param, grad, i);
                break;
            case OPT_ADAM:
                adam_step(opt, param, grad, i);
                break;
            case OPT_TERNARY:
                ternary_step(opt, param, grad, i);
                break;
            default:
                break;
        }
    }
}

/* Zero gradients */
void rtka_optimizer_zero_grad(rtka_optimizer_t* opt, rtka_grad_node_t** parameters, uint32_t param_count) {
    (void)opt;
    for (uint32_t i = 0; i < param_count; i++) {
        rtka_grad_zero(parameters[i]);
    }
}

/* Create scheduler */
rtka_lr_scheduler_t* rtka_scheduler_create(rtka_confidence_t initial_lr, uint32_t schedule_type) {
    rtka_lr_scheduler_t* scheduler = (rtka_lr_scheduler_t*)rtka_alloc_state();
    if (!scheduler) return NULL;
    
    scheduler->initial_lr = initial_lr;
    scheduler->min_lr = initial_lr * 0.01f;
    scheduler->schedule_type = schedule_type;
    scheduler->current_step = 0;
    scheduler->warmup_steps = 1000;
    scheduler->decay_steps = 10000;
    scheduler->decay_factor = 0.1f;
    
    return scheduler;
}

/* Get current learning rate */
rtka_confidence_t rtka_scheduler_get_lr(rtka_lr_scheduler_t* scheduler) {
    if (scheduler->current_step < scheduler->warmup_steps) {
        /* Linear warmup */
        return scheduler->initial_lr * scheduler->current_step / scheduler->warmup_steps;
    }
    
    switch (scheduler->schedule_type) {
        case LR_COSINE: {
            rtka_confidence_t progress = (rtka_confidence_t)(scheduler->current_step - scheduler->warmup_steps) / 
                                        scheduler->decay_steps;
            return scheduler->min_lr + (scheduler->initial_lr - scheduler->min_lr) * 
                   (1.0f + cosf(M_PI * progress)) / 2.0f;
        }
        case LR_EXPONENTIAL:
            return scheduler->initial_lr * powf(scheduler->decay_factor, 
                   scheduler->current_step / (rtka_confidence_t)scheduler->decay_steps);
        default:
            return scheduler->initial_lr;
    }
}

/* Compute gradient norm */
rtka_confidence_t rtka_compute_grad_norm(rtka_grad_node_t** parameters, uint32_t count) {
    rtka_confidence_t norm_sq = 0.0f;
    
    for (uint32_t i = 0; i < count; i++) {
        if (!parameters[i]->grad) continue;
        
        for (uint32_t j = 0; j < parameters[i]->grad->size; j++) {
            rtka_confidence_t g = parameters[i]->grad->data[j].confidence;
            norm_sq += g * g;
        }
    }
    
    return sqrtf(norm_sq);
}
