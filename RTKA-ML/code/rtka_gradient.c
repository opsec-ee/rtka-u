/**
 * File: rtka_gradient.c
 * Copyright (c) 2025 - H.Overman opsec.ee@pm.me
 * 
 * PROPRIETARY AND CONFIDENTIAL
 * This file is proprietary to H. Overman and may not be reproduced,
 * distributed, or used without explicit written permission.
 *
 * RTKA Gradient Operations Implementation
 */

#include "rtka_gradient.h"
#include "rtka_memory.h"
#include <string.h>
#include <math.h>

/* Thread-local gradient tape */
_Thread_local rtka_grad_tape_t* rtka_current_tape = NULL;

/* Create gradient tape */
rtka_grad_tape_t* rtka_grad_tape_create(void) {
    rtka_grad_tape_t* tape = (rtka_grad_tape_t*)rtka_alloc_state();
    if (!tape) return NULL;
    
    tape->capacity = 256;
    tape->nodes = (rtka_grad_node_t**)rtka_alloc_states(tape->capacity * sizeof(void*) / sizeof(rtka_state_t));
    if (!tape->nodes) {
        rtka_free_state((rtka_state_t*)tape);
        return NULL;
    }
    
    tape->node_count = 0;
    tape->recording = false;
    return tape;
}

/* Begin recording */
void rtka_grad_tape_begin(rtka_grad_tape_t* tape) {
    tape->recording = true;
    rtka_current_tape = tape;
}

/* Create gradient node */
rtka_grad_node_t* rtka_grad_node_create(rtka_tensor_t* data, bool requires_grad) {
    rtka_grad_node_t* node = (rtka_grad_node_t*)rtka_alloc_state();
    if (!node) return NULL;
    
    node->data = data;
    node->grad = requires_grad ? rtka_tensor_zeros(data->shape, data->ndim) : NULL;
    node->op = GRAD_OP_NONE;
    node->inputs[0] = node->inputs[1] = NULL;
    node->outputs = NULL;
    node->output_count = 0;
    node->requires_grad = requires_grad;
    node->grad_computed = false;
    node->ref_count = 1;
    
    /* Add to tape if recording */
    if (rtka_current_tape && rtka_current_tape->recording) {
        if (rtka_current_tape->node_count >= rtka_current_tape->capacity) {
            /* Expand capacity */
            uint32_t new_capacity = rtka_current_tape->capacity * 2;
            rtka_grad_node_t** new_nodes = (rtka_grad_node_t**)rtka_alloc_states(
                new_capacity * sizeof(void*) / sizeof(rtka_state_t));
            if (!new_nodes) return node;
            
            memcpy(new_nodes, rtka_current_tape->nodes, 
                   rtka_current_tape->node_count * sizeof(rtka_grad_node_t*));
            rtka_current_tape->nodes = new_nodes;
            rtka_current_tape->capacity = new_capacity;
        }
        
        rtka_current_tape->nodes[rtka_current_tape->node_count++] = node;
    }
    
    return node;
}

/* Forward AND with gradient tracking */
rtka_grad_node_t* rtka_grad_and(rtka_grad_node_t* a, rtka_grad_node_t* b) {
    if (!a || !b) return NULL;
    
    rtka_tensor_t* result = rtka_tensor_and(a->data, b->data);
    if (!result) return NULL;
    
    bool requires_grad = a->requires_grad || b->requires_grad;
    rtka_grad_node_t* node = rtka_grad_node_create(result, requires_grad);
    if (!node) {
        rtka_tensor_free(result);
        return NULL;
    }
    
    node->op = GRAD_OP_AND;
    node->inputs[0] = a;
    node->inputs[1] = b;
    
    /* Save inputs for backward pass */
    if (requires_grad) {
        node->saved_tensors[0] = a->data;
        node->saved_tensors[1] = b->data;
    }
    
    return node;
}

/* Forward OR with gradient tracking */
rtka_grad_node_t* rtka_grad_or(rtka_grad_node_t* a, rtka_grad_node_t* b) {
    if (!a || !b) return NULL;
    
    rtka_tensor_t* result = rtka_tensor_or(a->data, b->data);
    if (!result) return NULL;
    
    bool requires_grad = a->requires_grad || b->requires_grad;
    rtka_grad_node_t* node = rtka_grad_node_create(result, requires_grad);
    if (!node) {
        rtka_tensor_free(result);
        return NULL;
    }
    
    node->op = GRAD_OP_OR;
    node->inputs[0] = a;
    node->inputs[1] = b;
    
    if (requires_grad) {
        node->saved_tensors[0] = a->data;
        node->saved_tensors[1] = b->data;
    }
    
    return node;
}

/* Backward pass for AND */
static void backward_and(rtka_grad_node_t* node) {
    rtka_tensor_t* a_data = (rtka_tensor_t*)node->saved_tensors[0];
    rtka_tensor_t* b_data = (rtka_tensor_t*)node->saved_tensors[1];
    
    if (node->inputs[0] && node->inputs[0]->requires_grad) {
        /* Compute gradient w.r.t. first input */
        for (uint32_t i = 0; i < a_data->size; i++) {
            rtka_confidence_t grad = rtka_grad_and_wrt_a(
                a_data->data[i], b_data->data[i], 
                node->grad->data[i].confidence);
            node->inputs[0]->grad->data[i].confidence += grad;
        }
    }
    
    if (node->inputs[1] && node->inputs[1]->requires_grad) {
        /* Compute gradient w.r.t. second input */
        for (uint32_t i = 0; i < b_data->size; i++) {
            rtka_confidence_t grad = rtka_grad_and_wrt_b(
                a_data->data[i], b_data->data[i],
                node->grad->data[i].confidence);
            node->inputs[1]->grad->data[i].confidence += grad;
        }
    }
}

/* Backward pass for OR */
static void backward_or(rtka_grad_node_t* node) {
    rtka_tensor_t* a_data = (rtka_tensor_t*)node->saved_tensors[0];
    rtka_tensor_t* b_data = (rtka_tensor_t*)node->saved_tensors[1];
    
    if (node->inputs[0] && node->inputs[0]->requires_grad) {
        for (uint32_t i = 0; i < a_data->size; i++) {
            rtka_confidence_t grad = rtka_grad_or_wrt_a(
                a_data->data[i], b_data->data[i],
                node->grad->data[i].confidence);
            node->inputs[0]->grad->data[i].confidence += grad;
        }
    }
    
    if (node->inputs[1] && node->inputs[1]->requires_grad) {
        for (uint32_t i = 0; i < b_data->size; i++) {
            /* For OR: ∂(a∨b)/∂b similar to ∂(a∨b)/∂a */
            rtka_confidence_t grad = rtka_grad_or_wrt_a(
                b_data->data[i], a_data->data[i],
                node->grad->data[i].confidence);
            node->inputs[1]->grad->data[i].confidence += grad;
        }
    }
}

/* Main backward pass */
void rtka_grad_backward(rtka_grad_node_t* output) {
    if (!output || !output->requires_grad) return;
    
    /* Initialize output gradient to 1 */
    for (uint32_t i = 0; i < output->grad->size; i++) {
        output->grad->data[i].confidence = 1.0f;
    }
    
    /* Traverse computation graph in reverse */
    if (!rtka_current_tape) return;
    
    for (int32_t i = rtka_current_tape->node_count - 1; i >= 0; i--) {
        rtka_grad_node_t* node = rtka_current_tape->nodes[i];
        if (!node->requires_grad || node->grad_computed) continue;
        
        switch (node->op) {
            case GRAD_OP_AND:
                backward_and(node);
                break;
            case GRAD_OP_OR:
                backward_or(node);
                break;
            default:
                break;
        }
        
        node->grad_computed = true;
    }
}

/* Gradient clipping by norm */
void rtka_grad_clip_norm(rtka_tensor_t* grad, rtka_confidence_t max_norm) {
    /* Calculate L2 norm */
    rtka_confidence_t norm_sq = 0.0f;
    for (uint32_t i = 0; i < grad->size; i++) {
        rtka_confidence_t c = grad->data[i].confidence;
        norm_sq += c * c;
    }
    
    rtka_confidence_t norm = sqrtf(norm_sq);
    if (norm > max_norm) {
        rtka_confidence_t scale = max_norm / norm;
        for (uint32_t i = 0; i < grad->size; i++) {
            grad->data[i].confidence *= scale;
        }
    }
}

/* Zero gradients */
void rtka_grad_zero(rtka_grad_node_t* node) {
    if (!node || !node->grad) return;
    
    for (uint32_t i = 0; i < node->grad->size; i++) {
        node->grad->data[i].confidence = 0.0f;
    }
}

/* Free gradient tape */
void rtka_grad_tape_free(rtka_grad_tape_t* tape) {
    if (!tape) return;
    
    for (uint32_t i = 0; i < tape->node_count; i++) {
        /* Nodes are managed by memory pools */
    }
    
    rtka_free_state((rtka_state_t*)tape);
}
