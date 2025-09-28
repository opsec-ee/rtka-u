/**
 * File: rtka_nn.c
 * Copyright (c) 2025 - H.Overman opsec.ee@pm.me
 * 
 * PROPRIETARY AND CONFIDENTIAL
 * This file is proprietary to H. Overman and may not be reproduced,
 * distributed, or used without explicit written permission.
 *
 * For licensing inquiries: opsec.ee@pm.me
 *
 * RTKA Neural Network Implementation
 */

#include "rtka_nn.h"
#include "rtka_memory.h"
#include "rtka_random.h"
#include <math.h>
#include <string.h>

/* Forward declarations */
rtka_grad_node_t* rtka_nn_linear_forward(rtka_linear_layer_t* layer, rtka_grad_node_t* input);
rtka_grad_node_t* rtka_nn_ternary_forward(rtka_ternary_layer_t* layer, rtka_grad_node_t* input);

/* Create linear layer */
rtka_linear_layer_t* rtka_nn_linear(uint32_t in_features, uint32_t out_features, bool bias) {
    rtka_linear_layer_t* layer = (rtka_linear_layer_t*)rtka_alloc_state();
    if (!layer) return NULL;
    
    layer->base.type = LAYER_LINEAR;
    layer->base.in_features = in_features;
    layer->base.out_features = out_features;
    layer->base.training = true;
    layer->use_bias = bias;
    
    /* Initialize weights */
    uint32_t weight_shape[] = {out_features, in_features};
    rtka_tensor_t* weight = rtka_tensor_unknown(weight_shape, 2);
    
    /* Xavier initialization */
    float scale = sqrtf(2.0f / (in_features + out_features));
    for (uint32_t i = 0; i < weight->size; i++) {
        weight->data[i].confidence = (rtka_random_float() - 0.5f) * 2.0f * scale;
        weight->data[i].value = RTKA_UNKNOWN;
    }
    
    layer->base.weight = rtka_grad_node_create(weight, true);
    
    if (bias) {
        uint32_t bias_shape[] = {out_features};
        rtka_tensor_t* bias_tensor = rtka_tensor_zeros(bias_shape, 1);
        layer->base.bias = rtka_grad_node_create(bias_tensor, true);
    }
    
    layer->base.forward = (void*)rtka_nn_linear_forward;
    return layer;
}

/* Create ternary layer */
rtka_ternary_layer_t* rtka_nn_ternary(uint32_t in_features, uint32_t out_features, rtka_confidence_t threshold) {
    rtka_ternary_layer_t* layer = (rtka_ternary_layer_t*)rtka_alloc_state();
    if (!layer) return NULL;
    
    layer->base.type = LAYER_TERNARY;
    layer->base.in_features = in_features;
    layer->base.out_features = out_features;
    layer->base.training = true;
    layer->threshold = threshold;
    layer->quantize_activations = true;
    
    /* Initialize ternary weights */
    uint32_t weight_shape[] = {out_features, in_features};
    rtka_tensor_t* weight = rtka_tensor_unknown(weight_shape, 2);
    
    /* Ternary initialization */
    for (uint32_t i = 0; i < weight->size; i++) {
        float r = rtka_random_float();
        if (r < 0.333f) {
            weight->data[i] = rtka_make_state(RTKA_FALSE, 1.0f);
        } else if (r < 0.667f) {
            weight->data[i] = rtka_make_state(RTKA_UNKNOWN, 0.5f);
        } else {
            weight->data[i] = rtka_make_state(RTKA_TRUE, 1.0f);
        }
    }
    
    layer->base.weight = rtka_grad_node_create(weight, true);
    layer->base.forward = (void*)rtka_nn_ternary_forward;
    
    return layer;
}

/* Linear forward pass */
rtka_grad_node_t* rtka_nn_linear_forward(rtka_linear_layer_t* layer, rtka_grad_node_t* input) {
    rtka_grad_node_t* output = rtka_grad_matmul(input, layer->base.weight);
    
    if (layer->use_bias && layer->base.bias) {
        /* Add bias */
        rtka_tensor_t* out_data = output->data;
        rtka_tensor_t* bias_data = layer->base.bias->data;
        
        for (uint32_t i = 0; i < out_data->shape[0]; i++) {
            for (uint32_t j = 0; j < out_data->shape[1]; j++) {
                uint32_t idx = i * out_data->shape[1] + j;
                out_data->data[idx] = rtka_combine_or(out_data->data[idx], bias_data->data[j]);
            }
        }
    }
    
    return output;
}

/* Ternary forward pass with quantization */
rtka_grad_node_t* rtka_nn_ternary_forward(rtka_ternary_layer_t* layer, rtka_grad_node_t* input) {
    /* Quantize input if needed */
    if (layer->quantize_activations) {
        rtka_tensor_t* in_data = input->data;
        for (uint32_t i = 0; i < in_data->size; i++) {
            in_data->data[i] = rtka_ternary_sigmoid(in_data->data[i], layer->threshold);
        }
    }
    
    /* Matrix multiply with ternary weights */
    rtka_grad_node_t* output = rtka_grad_matmul(input, layer->base.weight);
    
    /* Apply ternary activation */
    rtka_tensor_t* out_data = output->data;
    for (uint32_t i = 0; i < out_data->size; i++) {
        out_data->data[i] = rtka_ternary_sigmoid(out_data->data[i], layer->threshold);
    }
    
    return output;
}

/* Sequential model */
rtka_sequential_t* rtka_nn_sequential(void) {
    rtka_sequential_t* model = (rtka_sequential_t*)rtka_alloc_state();
    if (!model) return NULL;
    
    model->capacity = 16;
    model->layers = (rtka_layer_t**)rtka_alloc_states(model->capacity * sizeof(void*) / sizeof(rtka_state_t));
    model->num_layers = 0;
    
    return model;
}

void rtka_nn_sequential_add(rtka_sequential_t* model, rtka_layer_t* layer) {
    if (model->num_layers >= model->capacity) {
        /* Expand capacity */
        model->capacity *= 2;
        rtka_layer_t** new_layers = (rtka_layer_t**)rtka_alloc_states(
            model->capacity * sizeof(void*) / sizeof(rtka_state_t));
        memcpy(new_layers, model->layers, model->num_layers * sizeof(rtka_layer_t*));
        model->layers = new_layers;
    }
    
    model->layers[model->num_layers++] = layer;
}

rtka_grad_node_t* rtka_nn_sequential_forward(rtka_sequential_t* model, rtka_grad_node_t* input) {
    rtka_grad_node_t* output = input;
    
    for (uint32_t i = 0; i < model->num_layers; i++) {
        output = model->layers[i]->forward(model->layers[i], output);
        if (!output) return NULL;
    }
    
    return output;
}

/* Ternary cross entropy loss */
rtka_confidence_t rtka_nn_ternary_cross_entropy(rtka_grad_node_t* output, rtka_tensor_t* target) {
    rtka_tensor_t* out_data = output->data;
    rtka_confidence_t loss = 0.0f;
    
    for (uint32_t i = 0; i < out_data->size; i++) {
        rtka_state_t pred = out_data->data[i];
        rtka_state_t tgt = target->data[i];
        
        /* Cross entropy for ternary: -log(P(correct)) */
        if (pred.value == tgt.value) {
            loss -= logf(pred.confidence + 1e-8f);
        } else {
            loss -= logf(1.0f - pred.confidence + 1e-8f);
        }
    }
    
    return loss / out_data->size;
}

/* Ternary MSE loss */
rtka_confidence_t rtka_nn_ternary_mse(rtka_grad_node_t* output, rtka_tensor_t* target) {
    rtka_tensor_t* out_data = output->data;
    rtka_confidence_t loss = 0.0f;
    
    for (uint32_t i = 0; i < out_data->size; i++) {
        float pred_val = (float)out_data->data[i].value * out_data->data[i].confidence;
        float tgt_val = (float)target->data[i].value * target->data[i].confidence;
        float diff = pred_val - tgt_val;
        loss += diff * diff;
    }
    
    return loss / out_data->size;
}

/* Random number generator stub - should link with rtka_random.c */
float rtka_random_float(void) {
    return (float)rand() / RAND_MAX;
}
