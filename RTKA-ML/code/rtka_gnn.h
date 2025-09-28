/**
 * File: rtka_gnn.h
 * Copyright (c) 2025 - H.Overman opsec.ee@pm.me
 * 
 * PROPRIETARY AND CONFIDENTIAL
 * This file is proprietary to H. Overman and may not be reproduced,
 * distributed, or used without explicit written permission.
 *
 * RTKA Graph Neural Networks
 */

#ifndef RTKA_GNN_H
#define RTKA_GNN_H

#include "rtka_types.h"
#include "rtka_tensor.h"
#include "rtka_gradient.h"
#include "rtka_nn.h"

/* Graph structure */
typedef struct {
    uint32_t num_nodes;
    uint32_t num_edges;
    uint32_t* edge_index;  /* 2 x num_edges */
    rtka_tensor_t* node_features;
    rtka_tensor_t* edge_features;
    rtka_tensor_t* adjacency;  /* Sparse or dense */
} rtka_graph_t;

/* Message passing types */
typedef enum {
    MSG_SUM,
    MSG_MEAN,
    MSG_MAX,
    MSG_TERNARY  /* Ternary logic aggregation */
} rtka_aggregation_t;

/* GNN layer base */
typedef struct {
    rtka_layer_t base;
    rtka_aggregation_t aggregation;
    uint32_t hidden_dim;
} rtka_gnn_layer_t;

/* Graph Convolutional Network (GCN) layer */
typedef struct {
    rtka_gnn_layer_t base;
    rtka_grad_node_t* weight_self;
    rtka_grad_node_t* weight_neighbors;
    bool add_self_loops;
    bool normalize;
} rtka_gcn_layer_t;

/* Graph Attention Network (GAT) layer */
typedef struct {
    rtka_gnn_layer_t base;
    rtka_grad_node_t* weight_linear;
    rtka_grad_node_t* attention_weights;
    uint32_t num_heads;
    rtka_confidence_t dropout;
} rtka_gat_layer_t;

/* Ternary Graph Network layer */
typedef struct {
    rtka_gnn_layer_t base;
    rtka_grad_node_t* message_fn;
    rtka_grad_node_t* update_fn;
    rtka_confidence_t threshold;
} rtka_tgn_layer_t;

/* Graph creation */
rtka_graph_t* rtka_graph_create(uint32_t num_nodes, uint32_t num_edges);
void rtka_graph_add_edge(rtka_graph_t* graph, uint32_t src, uint32_t dst);
void rtka_graph_compute_adjacency(rtka_graph_t* graph);
void rtka_graph_free(rtka_graph_t* graph);

/* Layer creation */
rtka_gcn_layer_t* rtka_gnn_gcn(uint32_t in_features, uint32_t out_features, bool bias);
rtka_gat_layer_t* rtka_gnn_gat(uint32_t in_features, uint32_t out_features, uint32_t num_heads);
rtka_tgn_layer_t* rtka_gnn_ternary(uint32_t in_features, uint32_t out_features, rtka_confidence_t threshold);

/* Message passing */
rtka_grad_node_t* rtka_gnn_message_pass(rtka_gnn_layer_t* layer, 
                                        rtka_grad_node_t* node_features,
                                        rtka_graph_t* graph);

/* Ternary message aggregation */
RTKA_INLINE rtka_state_t rtka_gnn_aggregate_ternary(const rtka_state_t* messages, uint32_t count) {
    if (count == 0) return rtka_make_state(RTKA_UNKNOWN, 0.5f);
    
    /* Count votes */
    uint32_t true_count = 0, false_count = 0, unknown_count = 0;
    rtka_confidence_t total_conf = 0.0f;
    
    for (uint32_t i = 0; i < count; i++) {
        switch (messages[i].value) {
            case RTKA_TRUE: true_count++; break;
            case RTKA_FALSE: false_count++; break;
            case RTKA_UNKNOWN: unknown_count++; break;
        }
        total_conf += messages[i].confidence;
    }
    
    /* Majority voting with confidence */
    rtka_value_t result;
    if (true_count > false_count && true_count > unknown_count) {
        result = RTKA_TRUE;
    } else if (false_count > true_count && false_count > unknown_count) {
        result = RTKA_FALSE;
    } else {
        result = RTKA_UNKNOWN;
    }
    
    return rtka_make_state(result, total_conf / count);
}

/* Graph attention mechanism */
rtka_tensor_t* rtka_gnn_compute_attention(rtka_tensor_t* query, 
                                          rtka_tensor_t* key,
                                          rtka_tensor_t* value,
                                          rtka_graph_t* graph);

/* Graph pooling operations */
rtka_tensor_t* rtka_gnn_global_pool(rtka_tensor_t* node_features, 
                                    rtka_aggregation_t method);
rtka_tensor_t* rtka_gnn_topk_pool(rtka_tensor_t* node_features,
                                  rtka_tensor_t* scores,
                                  uint32_t k);

/* Edge convolution for ternary */
rtka_grad_node_t* rtka_gnn_edge_conv(rtka_grad_node_t* node_features,
                                     rtka_grad_node_t* edge_features,
                                     rtka_graph_t* graph);

#endif /* RTKA_GNN_H */
