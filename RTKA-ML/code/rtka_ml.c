/**
 * File: rtka_ml.c
 * Copyright (c) 2025 - H.Overman opsec.ee@pm.me
 * 
 * PROPRIETARY AND CONFIDENTIAL
 * This file is proprietary to H. Overman and may not be reproduced,
 * distributed, or used without explicit written permission.
 *
 * For licensing inquiries: opsec.ee@pm.me
 *
 * RTKA Machine Learning Implementation
 */

#include "rtka_ml.h"
#include "rtka_memory.h"
#include <string.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>

/* Create model */
rtka_model_t* rtka_ml_create_model(rtka_model_type_t type) {
    rtka_model_t* model = (rtka_model_t*)rtka_alloc_state();
    if (!model) return NULL;
    
    model->type = type;
    model->network = rtka_nn_sequential();
    model->tape = rtka_grad_tape_create();
    model->epochs_trained = 0;
    model->compiled = false;
    model->trained = false;
    
    return model;
}

/* Add layer */
void rtka_ml_add_layer(rtka_model_t* model, rtka_layer_t* layer) {
    rtka_nn_sequential_add(model->network, layer);
}

/* Compile model */
void rtka_ml_compile(rtka_model_t* model, rtka_optimizer_t* optimizer) {
    model->optimizer = optimizer;
    model->compiled = true;
}

/* Training step */
static rtka_confidence_t train_step(rtka_model_t* model,
                                   rtka_tensor_t* batch_features,
                                   rtka_tensor_t* batch_labels) {
    /* Forward pass */
    rtka_grad_tape_begin(model->tape);
    rtka_grad_node_t* input_node = rtka_grad_node_create(batch_features, false);
    rtka_grad_node_t* output = rtka_nn_sequential_forward(model->network, input_node);
    
    /* Compute loss */
    rtka_confidence_t loss = 0.0f;
    if (model->type == MODEL_CLASSIFIER || model->type == MODEL_TERNARY_NET) {
        loss = rtka_nn_ternary_cross_entropy(output, batch_labels);
    } else {
        loss = rtka_nn_ternary_mse(output, batch_labels);
    }
    
    /* Backward pass */
    rtka_grad_backward(output);
    
    /* Update parameters */
    rtka_grad_node_t* params[128];
    uint32_t param_count = 0;
    
    for (uint32_t i = 0; i < model->network->num_layers; i++) {
        rtka_layer_t* layer = model->network->layers[i];
        if (layer->weight) params[param_count++] = layer->weight;
        if (layer->bias) params[param_count++] = layer->bias;
    }
    
    rtka_optimizer_step(model->optimizer, params, param_count);
    rtka_optimizer_zero_grad(model->optimizer, params, param_count);
    
    return loss;
}

/* Fit model */
void rtka_ml_fit(rtka_model_t* model,
                rtka_dataset_t* train_data,
                rtka_dataset_t* val_data,
                rtka_training_config_t* config) {
    if (!model->compiled) return;
    
    uint32_t num_batches = (train_data->num_samples + config->batch_size - 1) / config->batch_size;
    
    model->train_loss = (rtka_confidence_t*)rtka_alloc_states(
        config->epochs * sizeof(rtka_confidence_t) / sizeof(rtka_state_t));
    model->val_loss = (rtka_confidence_t*)rtka_alloc_states(
        config->epochs * sizeof(rtka_confidence_t) / sizeof(rtka_state_t));
    
    for (uint32_t epoch = 0; epoch < config->epochs; epoch++) {
        rtka_confidence_t epoch_loss = 0.0f;
        
        /* Shuffle if requested */
        if (config->shuffle) {
            rtka_ml_shuffle_dataset(train_data);
        }
        
        /* Training batches */
        for (uint32_t batch = 0; batch < num_batches; batch++) {
            uint32_t start = batch * config->batch_size;
            uint32_t end = start + config->batch_size;
            if (end > train_data->num_samples) end = train_data->num_samples;
            
            /* Extract batch */
            uint32_t batch_shape[] = {end - start, train_data->num_features};
            rtka_tensor_t* batch_features = rtka_tensor_create(batch_shape, 2);
            rtka_tensor_t* batch_labels = rtka_tensor_create(batch_shape, 2);
            
            for (uint32_t i = start; i < end; i++) {
                for (uint32_t j = 0; j < train_data->num_features; j++) {
                    uint32_t src_idx[] = {i, j};
                    uint32_t dst_idx[] = {i - start, j};
                    rtka_tensor_set(batch_features, dst_idx, 
                                   *rtka_tensor_get(train_data->features, src_idx));
                    rtka_tensor_set(batch_labels, dst_idx,
                                   *rtka_tensor_get(train_data->labels, src_idx));
                }
            }
            
            rtka_confidence_t loss = train_step(model, batch_features, batch_labels);
            epoch_loss += loss;
            
            rtka_tensor_free(batch_features);
            rtka_tensor_free(batch_labels);
        }
        
        model->train_loss[epoch] = epoch_loss / num_batches;
        
        /* Validation */
        if (val_data) {
            model->val_loss[epoch] = rtka_ml_evaluate(model, val_data);
        }
        
        if (config->verbose) {
            printf("Epoch %d/%d - loss: %.4f", epoch + 1, config->epochs, model->train_loss[epoch]);
            if (val_data) {
                printf(" - val_loss: %.4f", model->val_loss[epoch]);
            }
            printf("\n");
        }
        
        model->epochs_trained++;
    }
    
    model->trained = true;
}

/* Predict */
rtka_tensor_t* rtka_ml_predict(rtka_model_t* model, rtka_tensor_t* input) {
    if (!model->trained) return NULL;
    
    rtka_grad_node_t* input_node = rtka_grad_node_create(input, false);
    rtka_grad_node_t* output = rtka_nn_sequential_forward(model->network, input_node);
    
    /* Clone output tensor */
    rtka_tensor_t* predictions = rtka_tensor_create(output->data->shape, output->data->ndim);
    memcpy(predictions->data, output->data->data, output->data->size * sizeof(rtka_state_t));
    
    return predictions;
}

/* Evaluate */
rtka_confidence_t rtka_ml_evaluate(rtka_model_t* model, rtka_dataset_t* test_data) {
    rtka_tensor_t* predictions = rtka_ml_predict(model, test_data->features);
    if (!predictions) return 0.0f;
    
    rtka_confidence_t loss = 0.0f;
    if (model->type == MODEL_CLASSIFIER || model->type == MODEL_TERNARY_NET) {
        for (uint32_t i = 0; i < predictions->size; i++) {
            if (predictions->data[i].value == test_data->labels->data[i].value) {
                loss += 1.0f;
            }
        }
        loss /= predictions->size;  /* Accuracy */
    } else {
        for (uint32_t i = 0; i < predictions->size; i++) {
            rtka_confidence_t diff = predictions->data[i].confidence - 
                                    test_data->labels->data[i].confidence;
            loss += diff * diff;
        }
        loss = sqrtf(loss / predictions->size);  /* RMSE */
    }
    
    rtka_tensor_free(predictions);
    return loss;
}

/* Quantize model to ternary */
void rtka_ml_quantize_model(rtka_model_t* model, rtka_confidence_t threshold) {
    for (uint32_t i = 0; i < model->network->num_layers; i++) {
        rtka_layer_t* layer = model->network->layers[i];
        
        if (layer->weight && layer->weight->data) {
            rtka_tensor_t* weight = layer->weight->data;
            
            for (uint32_t j = 0; j < weight->size; j++) {
                rtka_confidence_t conf = weight->data[j].confidence;
                
                if (conf > threshold) {
                    weight->data[j].value = RTKA_TRUE;
                    weight->data[j].confidence = 1.0f;
                } else if (conf < -threshold) {
                    weight->data[j].value = RTKA_FALSE;
                    weight->data[j].confidence = 1.0f;
                } else {
                    weight->data[j].value = RTKA_UNKNOWN;
                    weight->data[j].confidence = fabsf(conf) / threshold;
                }
            }
        }
    }
}

/* Compute metrics */
rtka_metrics_t rtka_ml_compute_metrics(rtka_tensor_t* predictions, rtka_tensor_t* labels) {
    rtka_metrics_t metrics = {0};
    
    uint32_t true_pos = 0, true_neg = 0, false_pos = 0, false_neg = 0;
    uint32_t unknown_count = 0;
    
    for (uint32_t i = 0; i < predictions->size; i++) {
        rtka_value_t pred = predictions->data[i].value;
        rtka_value_t label = labels->data[i].value;
        
        if (pred == RTKA_UNKNOWN) {
            unknown_count++;
        } else if (pred == RTKA_TRUE && label == RTKA_TRUE) {
            true_pos++;
        } else if (pred == RTKA_FALSE && label == RTKA_FALSE) {
            true_neg++;
        } else if (pred == RTKA_TRUE && label == RTKA_FALSE) {
            false_pos++;
        } else if (pred == RTKA_FALSE && label == RTKA_TRUE) {
            false_neg++;
        }
    }
    
    metrics.accuracy = (rtka_confidence_t)(true_pos + true_neg) / predictions->size;
    metrics.precision = (true_pos > 0) ? (rtka_confidence_t)true_pos / (true_pos + false_pos) : 0.0f;
    metrics.recall = (true_pos > 0) ? (rtka_confidence_t)true_pos / (true_pos + false_neg) : 0.0f;
    metrics.f1_score = (metrics.precision + metrics.recall > 0) ? 
        2.0f * metrics.precision * metrics.recall / (metrics.precision + metrics.recall) : 0.0f;
    metrics.ternary_ratio = (rtka_confidence_t)unknown_count / predictions->size;
    
    return metrics;
}

/* Create MLP */
rtka_model_t* rtka_ml_create_mlp(uint32_t input_dim,
                                 uint32_t* hidden_sizes,
                                 uint32_t num_hidden,
                                 uint32_t output_dim) {
    rtka_model_t* model = rtka_ml_create_model(MODEL_CLASSIFIER);
    
    uint32_t prev_dim = input_dim;
    for (uint32_t i = 0; i < num_hidden; i++) {
        rtka_linear_layer_t* layer = rtka_nn_linear(prev_dim, hidden_sizes[i], true);
        rtka_ml_add_layer(model, (rtka_layer_t*)layer);
        prev_dim = hidden_sizes[i];
    }
    
    rtka_linear_layer_t* output_layer = rtka_nn_linear(prev_dim, output_dim, true);
    rtka_ml_add_layer(model, (rtka_layer_t*)output_layer);
    
    return model;
}

/* Create ternary classifier */
rtka_model_t* rtka_ml_create_ternary_classifier(uint32_t input_dim,
                                               uint32_t num_classes,
                                               rtka_confidence_t threshold) {
    rtka_model_t* model = rtka_ml_create_model(MODEL_TERNARY_NET);
    
    /* Hidden layers with ternary weights */
    rtka_ternary_layer_t* hidden1 = rtka_nn_ternary(input_dim, 128, threshold);
    rtka_ml_add_layer(model, (rtka_layer_t*)hidden1);
    
    rtka_ternary_layer_t* hidden2 = rtka_nn_ternary(128, 64, threshold);
    rtka_ml_add_layer(model, (rtka_layer_t*)hidden2);
    
    rtka_ternary_layer_t* output = rtka_nn_ternary(64, num_classes, threshold);
    rtka_ml_add_layer(model, (rtka_layer_t*)output);
    
    return model;
}

/* Shuffle dataset */
void rtka_ml_shuffle_dataset(rtka_dataset_t* data) {
    for (uint32_t i = data->num_samples - 1; i > 0; i--) {
        uint32_t j = rand() % (i + 1);
        
        /* Swap rows i and j */
        for (uint32_t k = 0; k < data->num_features; k++) {
            uint32_t idx_i[] = {i, k};
            uint32_t idx_j[] = {j, k};
            
            rtka_state_t temp = *rtka_tensor_get(data->features, idx_i);
            rtka_tensor_set(data->features, idx_i, *rtka_tensor_get(data->features, idx_j));
            rtka_tensor_set(data->features, idx_j, temp);
            
            temp = *rtka_tensor_get(data->labels, idx_i);
            rtka_tensor_set(data->labels, idx_i, *rtka_tensor_get(data->labels, idx_j));
            rtka_tensor_set(data->labels, idx_j, temp);
        }
    }
}

/* Model size calculation */
size_t rtka_ml_model_size(rtka_model_t* model) {
    size_t total_params = 0;
    
    for (uint32_t i = 0; i < model->network->num_layers; i++) {
        rtka_layer_t* layer = model->network->layers[i];
        if (layer->weight && layer->weight->data) {
            total_params += layer->weight->data->size;
        }
        if (layer->bias && layer->bias->data) {
            total_params += layer->bias->data->size;
        }
    }
    
    /* Ternary states need only 2 bits per weight */
    return (total_params * 2) / 8;  /* Bytes */
}

/* Compression ratio */
rtka_confidence_t rtka_ml_compression_ratio(rtka_model_t* model) {
    size_t ternary_size = rtka_ml_model_size(model);
    size_t float32_size = 0;
    
    for (uint32_t i = 0; i < model->network->num_layers; i++) {
        rtka_layer_t* layer = model->network->layers[i];
        if (layer->weight && layer->weight->data) {
            float32_size += layer->weight->data->size * sizeof(float);
        }
        if (layer->bias && layer->bias->data) {
            float32_size += layer->bias->data->size * sizeof(float);
        }
    }
    
    return (rtka_confidence_t)float32_size / ternary_size;
}
