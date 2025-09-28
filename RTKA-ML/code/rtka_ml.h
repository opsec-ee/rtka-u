/**
 * File: rtka_ml.h
 * Copyright (c) 2025 - H.Overman opsec.ee@pm.me
 * 
 * PROPRIETARY AND CONFIDENTIAL
 * This file is proprietary to H. Overman and may not be reproduced,
 * distributed, or used without explicit written permission.
 *
 * For licensing inquiries: opsec.ee@pm.me
 *
 * RTKA Machine Learning - High-level Interface
 */

#ifndef RTKA_ML_H
#define RTKA_ML_H

#include "rtka_types.h"
#include "rtka_tensor.h"
#include "rtka_nn.h"
#include "rtka_optimizer.h"
#include "rtka_gradient.h"

/* Model types */
typedef enum {
    MODEL_CLASSIFIER,
    MODEL_REGRESSOR,
    MODEL_AUTOENCODER,
    MODEL_GAN,
    MODEL_TERNARY_NET
} rtka_model_type_t;

/* Training configuration */
typedef struct {
    uint32_t epochs;
    uint32_t batch_size;
    rtka_confidence_t learning_rate;
    rtka_confidence_t validation_split;
    uint32_t early_stopping_patience;
    bool shuffle;
    bool verbose;
} rtka_training_config_t;

/* Dataset */
typedef struct {
    rtka_tensor_t* features;
    rtka_tensor_t* labels;
    uint32_t num_samples;
    uint32_t num_features;
    uint32_t num_classes;
    bool is_ternary;
} rtka_dataset_t;

/* Model structure */
typedef struct {
    rtka_model_type_t type;
    rtka_sequential_t* network;
    rtka_optimizer_t* optimizer;
    rtka_grad_tape_t* tape;
    
    /* Training history */
    rtka_confidence_t* train_loss;
    rtka_confidence_t* val_loss;
    rtka_confidence_t* train_acc;
    rtka_confidence_t* val_acc;
    uint32_t epochs_trained;
    
    /* Model state */
    bool compiled;
    bool trained;
} rtka_model_t;

/* Model creation and management */
rtka_model_t* rtka_ml_create_model(rtka_model_type_t type);
void rtka_ml_add_layer(rtka_model_t* model, rtka_layer_t* layer);
void rtka_ml_compile(rtka_model_t* model, rtka_optimizer_t* optimizer);
void rtka_ml_free_model(rtka_model_t* model);

/* Training */
void rtka_ml_fit(rtka_model_t* model, 
                rtka_dataset_t* train_data,
                rtka_dataset_t* val_data,
                rtka_training_config_t* config);

/* Prediction */
rtka_tensor_t* rtka_ml_predict(rtka_model_t* model, rtka_tensor_t* input);
rtka_confidence_t rtka_ml_evaluate(rtka_model_t* model, rtka_dataset_t* test_data);

/* Dataset operations */
rtka_dataset_t* rtka_ml_load_dataset(const char* path, bool normalize);
void rtka_ml_split_dataset(rtka_dataset_t* data, 
                          rtka_dataset_t** train, 
                          rtka_dataset_t** test,
                          rtka_confidence_t test_ratio);
void rtka_ml_shuffle_dataset(rtka_dataset_t* data);

/* Ternary quantization for model compression */
void rtka_ml_quantize_model(rtka_model_t* model, rtka_confidence_t threshold);
size_t rtka_ml_model_size(rtka_model_t* model);
rtka_confidence_t rtka_ml_compression_ratio(rtka_model_t* model);

/* Model persistence */
bool rtka_ml_save_model(rtka_model_t* model, const char* path);
rtka_model_t* rtka_ml_load_model(const char* path);

/* Metrics */
typedef struct {
    rtka_confidence_t accuracy;
    rtka_confidence_t precision;
    rtka_confidence_t recall;
    rtka_confidence_t f1_score;
    rtka_confidence_t ternary_ratio;  /* Ratio of UNKNOWN states */
} rtka_metrics_t;

rtka_metrics_t rtka_ml_compute_metrics(rtka_tensor_t* predictions, rtka_tensor_t* labels);

/* Cross-validation */
rtka_confidence_t rtka_ml_cross_validate(rtka_model_t* model,
                                        rtka_dataset_t* data,
                                        uint32_t k_folds);

/* Feature engineering for ternary */
rtka_tensor_t* rtka_ml_extract_features(rtka_tensor_t* input, uint32_t num_features);
rtka_tensor_t* rtka_ml_encode_ternary(rtka_tensor_t* input);
rtka_tensor_t* rtka_ml_decode_ternary(rtka_tensor_t* encoded);

/* Common models */
rtka_model_t* rtka_ml_create_mlp(uint32_t input_dim, 
                                 uint32_t* hidden_sizes, 
                                 uint32_t num_hidden,
                                 uint32_t output_dim);

rtka_model_t* rtka_ml_create_ternary_classifier(uint32_t input_dim,
                                               uint32_t num_classes,
                                               rtka_confidence_t threshold);

#endif /* RTKA_ML_H */
