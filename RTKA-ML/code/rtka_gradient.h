/**
 * File: rtka_gradient.h
 * Copyright (c) 2025 - H.Overman opsec.ee@pm.me
 * 
 * RTKA Gradient Operations - Automatic Differentiation
 */

#ifndef RTKA_GRADIENT_H
#define RTKA_GRADIENT_H

#include "rtka_types.h"
#include "rtka_tensor.h"

/* Forward declaration */
typedef struct rtka_grad_node rtka_grad_node_t;

/* Gradient operation types */
typedef enum {
    GRAD_OP_NONE,
    GRAD_OP_AND,
    GRAD_OP_OR,
    GRAD_OP_NOT,
    GRAD_OP_MATMUL,
    GRAD_OP_ADD,
    GRAD_OP_MULTIPLY,
    GRAD_OP_REDUCE,
    GRAD_OP_MAX
} rtka_grad_op_t;

/* Computation graph node */
struct rtka_grad_node {
    rtka_tensor_t* data;
    rtka_tensor_t* grad;
    
    rtka_grad_op_t op;
    rtka_grad_node_t* inputs[2];
    rtka_grad_node_t** outputs;
    uint32_t output_count;
    
    bool requires_grad;
    bool grad_computed;
    uint32_t ref_count;
    
    /* Saved tensors for backward pass */
    void* saved_tensors[2];
};

/* Gradient tape for automatic differentiation */
typedef struct {
    rtka_grad_node_t** nodes;
    uint32_t node_count;
    uint32_t capacity;
    bool recording;
} rtka_grad_tape_t;

/* Gradient functions for ternary operations */
typedef struct {
    rtka_confidence_t (*grad_and_a)(rtka_state_t a, rtka_state_t b, rtka_confidence_t grad_out);
    rtka_confidence_t (*grad_and_b)(rtka_state_t a, rtka_state_t b, rtka_confidence_t grad_out);
    rtka_confidence_t (*grad_or_a)(rtka_state_t a, rtka_state_t b, rtka_confidence_t grad_out);
    rtka_confidence_t (*grad_or_b)(rtka_state_t a, rtka_state_t b, rtka_confidence_t grad_out);
    rtka_confidence_t (*grad_not)(rtka_state_t a, rtka_confidence_t grad_out);
} rtka_grad_functions_t;

/* Global gradient tape */
extern _Thread_local rtka_grad_tape_t* rtka_current_tape;

/* Tape operations */
rtka_grad_tape_t* rtka_grad_tape_create(void);
void rtka_grad_tape_free(rtka_grad_tape_t* tape);
void rtka_grad_tape_begin(rtka_grad_tape_t* tape);
void rtka_grad_tape_end(rtka_grad_tape_t* tape);

/* Node creation */
rtka_grad_node_t* rtka_grad_node_create(rtka_tensor_t* data, bool requires_grad);
void rtka_grad_node_free(rtka_grad_node_t* node);

/* Forward operations with gradient tracking */
rtka_grad_node_t* rtka_grad_and(rtka_grad_node_t* a, rtka_grad_node_t* b);
rtka_grad_node_t* rtka_grad_or(rtka_grad_node_t* a, rtka_grad_node_t* b);
rtka_grad_node_t* rtka_grad_not(rtka_grad_node_t* a);
rtka_grad_node_t* rtka_grad_matmul(rtka_grad_node_t* a, rtka_grad_node_t* b);

/* Backward pass */
void rtka_grad_backward(rtka_grad_node_t* output);
void rtka_grad_zero(rtka_grad_node_t* node);

/* Gradient computation for ternary logic */
RTKA_INLINE rtka_confidence_t rtka_grad_and_wrt_a(rtka_state_t a, rtka_state_t b, rtka_confidence_t grad_out) {
    /* ∂(a∧b)/∂a: gradient flows if b allows it */
    (void)a;  /* Used in logic, suppress warning */
    if (b.value == RTKA_FALSE) return 0.0f;
    if (b.value == RTKA_TRUE) return grad_out;
    return grad_out * b.confidence;
}

RTKA_INLINE rtka_confidence_t rtka_grad_and_wrt_b(rtka_state_t a, rtka_state_t b, rtka_confidence_t grad_out) {
    /* ∂(a∧b)/∂b: gradient flows if a allows it */
    (void)b;  /* Used in logic, suppress warning */
    if (a.value == RTKA_FALSE) return 0.0f;
    if (a.value == RTKA_TRUE) return grad_out;
    return grad_out * a.confidence;
}

RTKA_INLINE rtka_confidence_t rtka_grad_or_wrt_a(rtka_state_t a, rtka_state_t b, rtka_confidence_t grad_out) {
    /* ∂(a∨b)/∂a: gradient flows unless b blocks it */
    (void)a;  /* Used in logic, suppress warning */
    if (b.value == RTKA_TRUE) return 0.0f;
    if (b.value == RTKA_FALSE) return grad_out;
    return grad_out * (1.0f - b.confidence);
}

/* Gradient accumulation */
void rtka_grad_accumulate(rtka_tensor_t* grad, const rtka_tensor_t* delta);

/* Gradient clipping */
void rtka_grad_clip_norm(rtka_tensor_t* grad, rtka_confidence_t max_norm);
void rtka_grad_clip_value(rtka_tensor_t* grad, rtka_confidence_t min_val, rtka_confidence_t max_val);

/* Checkpointing for memory efficiency */
typedef struct {
    rtka_grad_node_t** checkpoints;
    uint32_t checkpoint_count;
    uint32_t checkpoint_interval;
} rtka_checkpoint_manager_t;

rtka_checkpoint_manager_t* rtka_checkpoint_create(uint32_t interval);
void rtka_checkpoint_save(rtka_checkpoint_manager_t* mgr, rtka_grad_node_t* node);
void rtka_checkpoint_free(rtka_checkpoint_manager_t* mgr);

#endif /* RTKA_GRADIENT_H */
