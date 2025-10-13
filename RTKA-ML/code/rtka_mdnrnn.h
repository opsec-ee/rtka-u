/**
 * File: rtka_mdnrnn.h
 * Copyright (c) 2025 - H.Overman opsec.ee@pm.me
 * Email: opsec.ee@pm.me
 * 
 * RTKA MDNRNN (Mixture Density Network + Recurrent Neural Network)
 * Combines LSTM temporal processing with MDN probabilistic outputs
 * 
 * Architecture:
 * Input -> LSTM -> MDN -> (pi, mu, sigma)
 * 
 * CHANGELOG:
 * v1.0.0 - 2025-10-12: Initial implementation
 *   - Combined LSTM+MDN architecture
 *   - Sequence-to-sequence processing
 *   - Probabilistic trajectory prediction
 *   - Integrated forward pass
 */

#ifndef RTKA_MDNRNN_H
#define RTKA_MDNRNN_H

#include "rtka_types.h"
#include "rtka_tensor.h"
#include "rtka_lstm.h"
#include "rtka_mdn.h"
#include <stdint.h>
#include <stdbool.h>

/* MDNRNN combined model */
typedef struct {
    /* Architecture parameters */
    uint32_t z_size;           /* Latent state size (input) */
    uint32_t action_size;      /* Action space size */
    uint32_t hidden_size;      /* LSTM hidden dimension */
    uint32_t num_gaussians;    /* Number of mixture components */
    
    /* Network components */
    rtka_lstm_layer_t* lstm;   /* LSTM for temporal dynamics */
    rtka_mdn_layer_t* mdn;     /* MDN for probabilistic output */
    
    bool batch_first;
    bool initialized;
} rtka_mdnrnn_t;

/* MDNRNN output */
typedef struct {
    rtka_mdn_output_t mdn_params;    /* pi, mu, sigma from MDN */
    rtka_tensor_t* hidden_state;     /* Final LSTM hidden state */
    rtka_tensor_t* cell_state;       /* Final LSTM cell state */
} rtka_mdnrnn_output_t;

/**
 * Create MDNRNN model
 * 
 * @param z_size        Latent state dimension
 * @param action_size   Action dimension
 * @param hidden_size   LSTM hidden dimension (default 256)
 * @param num_gaussians Number of Gaussian components (default 5)
 * @return Pointer to initialized MDNRNN or NULL on error
 */
rtka_mdnrnn_t* rtka_mdnrnn_create(uint32_t z_size,
                                  uint32_t action_size,
                                  uint32_t hidden_size,
                                  uint32_t num_gaussians);

/**
 * Initialize MDNRNN hidden states for a batch
 * 
 * @param model      MDNRNN model
 * @param batch_size Number of sequences in batch
 * @return true on success, false on error
 */
bool rtka_mdnrnn_init_hidden(rtka_mdnrnn_t* model, uint32_t batch_size);

/**
 * Reset all model parameters
 * 
 * @param model MDNRNN model
 */
void rtka_mdnrnn_reset_parameters(rtka_mdnrnn_t* model);

/**
 * Forward pass through MDNRNN
 * 
 * @param model   MDNRNN model
 * @param z       Latent states (batch, seq_len, z_size)
 * @param actions Actions (batch, seq_len, action_size)
 * @param h_0     Initial hidden state or NULL
 * @param c_0     Initial cell state or NULL
 * @return MDNRNN output with MDN parameters and final states
 */
rtka_mdnrnn_output_t rtka_mdnrnn_forward(rtka_mdnrnn_t* model,
                                         rtka_tensor_t* z,
                                         rtka_tensor_t* actions,
                                         rtka_tensor_t* h_0,
                                         rtka_tensor_t* c_0);

/**
 * Single-step prediction
 * Used for autoregressive generation
 * 
 * @param model  MDNRNN model
 * @param z_t    Current latent state (batch, z_size)
 * @param a_t    Current action (batch, action_size)
 * @return MDN parameters for next state prediction
 */
rtka_mdn_output_t rtka_mdnrnn_step(rtka_mdnrnn_t* model,
                                   rtka_tensor_t* z_t,
                                   rtka_tensor_t* a_t);

/**
 * Free MDNRNN model and all resources
 * 
 * @param model MDNRNN model to free
 */
void rtka_mdnrnn_free(rtka_mdnrnn_t* model);

/**
 * Free MDNRNN output structure
 * 
 * @param output MDNRNN output to free
 */
void rtka_mdnrnn_output_free(rtka_mdnrnn_output_t* output);

/**
 * Get total parameter count
 * 
 * @param model MDNRNN model
 * @return Total number of trainable parameters
 */
uint32_t rtka_mdnrnn_param_count(const rtka_mdnrnn_t* model);

/**
 * Sample next latent state from MDNRNN output
 * 
 * @param output    MDNRNN output
 * @param batch_idx Index in batch to sample from
 * @return Sampled next latent state or NULL on error
 */
rtka_tensor_t* rtka_mdnrnn_sample_next(const rtka_mdnrnn_output_t* output, 
                                       uint32_t batch_idx);

#endif /* RTKA_MDNRNN_H */
