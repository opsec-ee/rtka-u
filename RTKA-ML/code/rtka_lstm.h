/**
 * File: rtka_lstm.h
 * Copyright (c) 2025 - H.Overman opsec.ee@pm.me
 * Email: opsec.ee@pm.me
 * 
 * RTKA LSTM (Long Short-Term Memory) Implementation
 * Provides temporal sequence processing with hidden state management
 * 
 * CHANGELOG:
 * v1.0.0 - 2025-10-12: Initial implementation
 *   - Core LSTM cell with input, forget, output gates
 *   - Hidden and cell state management
 *   - Batch processing support
 *   - Standard weight initialization
 *   - Integration with RTKA tensor operations
 */

#ifndef RTKA_LSTM_H
#define RTKA_LSTM_H

#include "rtka_types.h"
#include "rtka_tensor.h"
#include "rtka_gradient.h"
#include <stdint.h>
#include <stdbool.h>

/* LSTM gate parameters structure */
typedef struct {
    rtka_grad_node_t* weight;  /* (input_size + hidden_size) x hidden_size */
    rtka_grad_node_t* bias;    /* hidden_size */
} rtka_lstm_gate_t;

/* LSTM layer structure */
typedef struct {
    uint32_t input_size;
    uint32_t hidden_size;
    bool batch_first;
    
    /* Gate parameters: input, forget, cell, output */
    rtka_lstm_gate_t gate_i;  /* Input gate */
    rtka_lstm_gate_t gate_f;  /* Forget gate */
    rtka_lstm_gate_t gate_g;  /* Cell gate */
    rtka_lstm_gate_t gate_o;  /* Output gate */
    
    /* Hidden states */
    rtka_tensor_t* h_prev;    /* Previous hidden state */
    rtka_tensor_t* c_prev;    /* Previous cell state */
    
    bool initialized;
} rtka_lstm_layer_t;

/* LSTM output structure */
typedef struct {
    rtka_grad_node_t* output;      /* Output sequence */
    rtka_tensor_t* hidden_state;   /* Final hidden state */
    rtka_tensor_t* cell_state;     /* Final cell state */
} rtka_lstm_output_t;

/**
 * Create LSTM layer
 * 
 * @param input_size  Size of input features
 * @param hidden_size Size of hidden state
 * @param batch_first If true, input shape is (batch, seq, feature)
 * @return Pointer to initialized LSTM layer or NULL on error
 */
rtka_lstm_layer_t* rtka_lstm_create(uint32_t input_size, 
                                    uint32_t hidden_size,
                                    bool batch_first);

/**
 * Initialize LSTM hidden and cell states
 * 
 * @param lstm       LSTM layer
 * @param batch_size Number of sequences in batch
 * @return true on success, false on error
 */
bool rtka_lstm_init_hidden(rtka_lstm_layer_t* lstm, uint32_t batch_size);

/**
 * Reset LSTM gates with Xavier initialization
 * 
 * @param lstm LSTM layer to reset
 */
void rtka_lstm_reset_parameters(rtka_lstm_layer_t* lstm);

/**
 * Forward pass through LSTM
 * 
 * @param lstm  LSTM layer
 * @param input Input tensor (batch_size, seq_len, input_size) if batch_first
 * @param h_0   Initial hidden state or NULL to use layer's h_prev
 * @param c_0   Initial cell state or NULL to use layer's c_prev
 * @return LSTM output structure containing output sequence and final states
 */
rtka_lstm_output_t rtka_lstm_forward(rtka_lstm_layer_t* lstm,
                                     rtka_grad_node_t* input,
                                     rtka_tensor_t* h_0,
                                     rtka_tensor_t* c_0);

/**
 * Single LSTM cell step
 * 
 * @param lstm LSTM layer
 * @param x_t  Input at time t (batch_size, input_size)
 * @param h_t  Hidden state at time t-1
 * @param c_t  Cell state at time t-1
 * @param h_next Output: hidden state at time t
 * @param c_next Output: cell state at time t
 * @return true on success, false on error
 */
bool rtka_lstm_cell_forward(rtka_lstm_layer_t* lstm,
                            rtka_tensor_t* x_t,
                            rtka_tensor_t* h_t,
                            rtka_tensor_t* c_t,
                            rtka_tensor_t** h_next,
                            rtka_tensor_t** c_next);

/**
 * Free LSTM layer and all associated resources
 * 
 * @param lstm LSTM layer to free
 */
void rtka_lstm_free(rtka_lstm_layer_t* lstm);

/**
 * Get parameter count for LSTM layer
 * 
 * @param lstm LSTM layer
 * @return Total number of parameters
 */
uint32_t rtka_lstm_param_count(const rtka_lstm_layer_t* lstm);

/**
 * Sigmoid activation for gates
 * Inline for performance
 */
RTKA_INLINE rtka_confidence_t rtka_lstm_sigmoid(rtka_confidence_t x) {
    return 1.0f / (1.0f + expf(-x));
}

/**
 * Tanh activation for cell state
 * Inline for performance
 */
RTKA_INLINE rtka_confidence_t rtka_lstm_tanh(rtka_confidence_t x) {
    return tanhf(x);
}

#endif /* RTKA_LSTM_H */
