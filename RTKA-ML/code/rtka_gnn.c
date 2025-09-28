/**
 * File: rtka_gnn.c
 * Copyright (c) 2025 - H.Overman opsec.ee@pm.me
 * 
 * RTKA Graph Neural Network Implementation
 */

#include "rtka_gnn.h"
#include "rtka_memory.h"
#include <string.h>
#include <math.h>

/* Create graph */
rtka_graph_t* rtka_graph_create(uint32_t num_nodes, uint32_t num_edges) {
    rtka_graph_t* graph = (rtka_graph_t*)rtka_alloc_state();
    if (!graph) return NULL;
    
    graph->num_nodes = num_nodes;
    graph->num_edges = 0;
    graph->edge_index = (uint32_t*)rtka_alloc_states(2 * num_edges * sizeof(uint32_t) / sizeof(rtka_state_t));
    
    /* Initialize node features */
    uint32_t shape[] = {num_nodes, 1};
    graph->node_features = rtka_tensor_unknown(shape, 2);
    
    return graph;
}

/* Add edge */
void rtka_graph_add_edge(rtka_graph_t* graph, uint32_t src, uint32_t dst) {
    if (graph->num_edges >= graph->num_nodes * graph->num_nodes) return;
    
    graph->edge_index[2 * graph->num_edges] = src;
    graph->edge_index[2 * graph->num_edges + 1] = dst;
    graph->num_edges++;
}

/* Compute adjacency matrix */
void rtka_graph_compute_adjacency(rtka_graph_t* graph) {
    uint32_t shape[] = {graph->num_nodes, graph->num_nodes};
    graph->adjacency = rtka_tensor_zeros(shape, 2);
    
    for (uint32_t e = 0; e < graph->num_edges; e++) {
        uint32_t src = graph->edge_index[2 * e];
        uint32_t dst = graph->edge_index[2 * e + 1];
        uint32_t idx[] = {src, dst};
        
        rtka_tensor_set(graph->adjacency, idx, rtka_make_state(RTKA_TRUE, 1.0f));
    }
}

/* Create GCN layer */
rtka_gcn_layer_t* rtka_gnn_gcn(uint32_t in_features, uint32_t out_features, bool bias) {
    (void)bias;  /* Unused for now */
    rtka_gcn_layer_t* layer = (rtka_gcn_layer_t*)rtka_alloc_state();
    if (!layer) return NULL;
    
    layer->base.base.type = LAYER_LINEAR;
    layer->base.base.in_features = in_features;
    layer->base.base.out_features = out_features;
    layer->base.aggregation = MSG_SUM;
    layer->add_self_loops = true;
    layer->normalize = true;
    
    /* Initialize weights */
    uint32_t weight_shape[] = {in_features, out_features};
    rtka_tensor_t* weight = rtka_tensor_unknown(weight_shape, 2);
    
    /* Xavier init */
    float scale = sqrtf(2.0f / (in_features + out_features));
    for (uint32_t i = 0; i < weight->size; i++) {
        weight->data[i].confidence = ((float)rand() / RAND_MAX - 0.5f) * 2.0f * scale;
        weight->data[i].value = RTKA_UNKNOWN;
    }
    
    layer->weight_neighbors = rtka_grad_node_create(weight, true);
    
    if (layer->add_self_loops) {
        rtka_tensor_t* self_weight = rtka_tensor_unknown(weight_shape, 2);
        memcpy(self_weight->data, weight->data, weight->size * sizeof(rtka_state_t));
        layer->weight_self = rtka_grad_node_create(self_weight, true);
    }
    
    return layer;
}

/* Create Ternary GNN layer */
rtka_tgn_layer_t* rtka_gnn_ternary(uint32_t in_features, uint32_t out_features, rtka_confidence_t threshold) {
    rtka_tgn_layer_t* layer = (rtka_tgn_layer_t*)rtka_alloc_state();
    if (!layer) return NULL;
    
    layer->base.base.type = LAYER_TERNARY;
    layer->base.base.in_features = in_features;
    layer->base.base.out_features = out_features;
    layer->base.aggregation = MSG_TERNARY;
    layer->threshold = threshold;
    
    /* Message function weights */
    uint32_t msg_shape[] = {in_features * 2, out_features};
    rtka_tensor_t* msg_weight = rtka_tensor_unknown(msg_shape, 2);
    
    /* Ternary initialization */
    for (uint32_t i = 0; i < msg_weight->size; i++) {
        float r = (float)rand() / RAND_MAX;
        if (r < 0.333f) {
            msg_weight->data[i] = rtka_make_state(RTKA_FALSE, 1.0f);
        } else if (r < 0.667f) {
            msg_weight->data[i] = rtka_make_state(RTKA_UNKNOWN, 0.5f);
        } else {
            msg_weight->data[i] = rtka_make_state(RTKA_TRUE, 1.0f);
        }
    }
    
    layer->message_fn = rtka_grad_node_create(msg_weight, true);
    
    /* Update function weights */
    uint32_t upd_shape[] = {out_features * 2, out_features};
    rtka_tensor_t* upd_weight = rtka_tensor_unknown(upd_shape, 2);
    memcpy(upd_weight->data, msg_weight->data, 
           (upd_weight->size < msg_weight->size ? upd_weight->size : msg_weight->size) * sizeof(rtka_state_t));
    
    layer->update_fn = rtka_grad_node_create(upd_weight, true);
    
    return layer;
}

/* Message passing for GCN */
static rtka_grad_node_t* gcn_forward(rtka_gcn_layer_t* layer, 
                                     rtka_grad_node_t* node_features,
                                     rtka_graph_t* graph) {
    rtka_tensor_t* adj = graph->adjacency;
    
    /* Compute degree normalization */
    if (layer->normalize) {
        rtka_tensor_t* degree = rtka_tensor_zeros((uint32_t[]){graph->num_nodes}, 1);
        
        for (uint32_t i = 0; i < graph->num_nodes; i++) {
            rtka_confidence_t d = 0.0f;
            for (uint32_t j = 0; j < graph->num_nodes; j++) {
                uint32_t idx[] = {i, j};
                d += rtka_tensor_get(adj, idx)->confidence;
            }
            degree->data[i].confidence = 1.0f / sqrtf(d + 1.0f);
        }
    }
    
    /* Message passing: AXW */
    rtka_grad_node_t* messages = rtka_grad_matmul(node_features, layer->weight_neighbors);
    
    /* Aggregate messages */
    uint32_t out_shape[] = {graph->num_nodes, layer->base.base.out_features};
    rtka_tensor_t* output = rtka_tensor_zeros(out_shape, 2);
    
    for (uint32_t dst = 0; dst < graph->num_nodes; dst++) {
        for (uint32_t feat = 0; feat < layer->base.base.out_features; feat++) {
            rtka_state_t agg = rtka_make_state(RTKA_FALSE, 0.0f);
            
            for (uint32_t src = 0; src < graph->num_nodes; src++) {
                uint32_t adj_idx[] = {src, dst};
                rtka_state_t edge = *rtka_tensor_get(adj, adj_idx);
                
                if (edge.value != RTKA_FALSE) {
                    uint32_t msg_idx[] = {src, feat};
                    rtka_state_t msg = *rtka_tensor_get(messages->data, msg_idx);
                    agg = rtka_combine_or(agg, rtka_combine_and(edge, msg));
                }
            }
            
            uint32_t out_idx[] = {dst, feat};
            rtka_tensor_set(output, out_idx, agg);
        }
    }
    
    return rtka_grad_node_create(output, true);
}

/* Ternary message passing */
static rtka_grad_node_t* ternary_forward(rtka_tgn_layer_t* layer,
                                         rtka_grad_node_t* node_features,
                                         rtka_graph_t* graph) {
    uint32_t num_nodes = graph->num_nodes;
    uint32_t hidden_dim = layer->base.base.out_features;
    
    /* Allocate message buffer */
    uint32_t msg_shape[] = {num_nodes, hidden_dim};
    rtka_tensor_t* messages = rtka_tensor_unknown(msg_shape, 2);
    
    /* Compute messages for each edge */
    for (uint32_t e = 0; e < graph->num_edges; e++) {
        uint32_t src = graph->edge_index[2 * e];
        uint32_t dst = graph->edge_index[2 * e + 1];
        
        /* Get source and destination features */
        for (uint32_t f = 0; f < hidden_dim; f++) {
            uint32_t src_idx[] = {src, f};
            uint32_t dst_idx[] = {dst, f};
            
            rtka_state_t src_feat = *rtka_tensor_get(node_features->data, src_idx);
            rtka_state_t dst_feat = *rtka_tensor_get(messages, dst_idx);
            
            /* Ternary message computation */
            rtka_state_t msg = rtka_combine_and(src_feat, 
                                               rtka_make_state(RTKA_TRUE, layer->threshold));
            
            /* Aggregate with existing messages */
            rtka_state_t aggregated = rtka_gnn_aggregate_ternary(&msg, 1);
            rtka_tensor_set(messages, dst_idx, 
                          rtka_combine_or(dst_feat, aggregated));
        }
    }
    
    /* Update node features */
    rtka_tensor_t* updated = rtka_tensor_unknown(msg_shape, 2);
    
    for (uint32_t n = 0; n < num_nodes; n++) {
        for (uint32_t f = 0; f < hidden_dim; f++) {
            uint32_t idx[] = {n, f};
            rtka_state_t old_feat = *rtka_tensor_get(node_features->data, idx);
            rtka_state_t msg = *rtka_tensor_get(messages, idx);
            
            /* Ternary update rule */
            rtka_state_t updated_feat = rtka_combine_or(
                rtka_combine_and(old_feat, rtka_make_state(RTKA_TRUE, 0.5f)),
                rtka_combine_and(msg, rtka_make_state(RTKA_TRUE, 0.5f))
            );
            
            rtka_tensor_set(updated, idx, updated_feat);
        }
    }
    
    return rtka_grad_node_create(updated, true);
}

/* Main message passing interface */
rtka_grad_node_t* rtka_gnn_message_pass(rtka_gnn_layer_t* layer,
                                        rtka_grad_node_t* node_features,
                                        rtka_graph_t* graph) {
    switch (layer->base.type) {
        case LAYER_LINEAR:
            return gcn_forward((rtka_gcn_layer_t*)layer, node_features, graph);
        case LAYER_TERNARY:
            return ternary_forward((rtka_tgn_layer_t*)layer, node_features, graph);
        default:
            return NULL;
    }
}

/* Global pooling */
rtka_tensor_t* rtka_gnn_global_pool(rtka_tensor_t* node_features, 
                                    rtka_aggregation_t method) {
    uint32_t num_nodes = node_features->shape[0];
    uint32_t num_features = node_features->shape[1];
    uint32_t out_shape[] = {1, num_features};
    rtka_tensor_t* pooled = rtka_tensor_zeros(out_shape, 2);
    
    for (uint32_t f = 0; f < num_features; f++) {
        rtka_state_t agg = rtka_make_state(RTKA_FALSE, 0.0f);
        
        for (uint32_t n = 0; n < num_nodes; n++) {
            uint32_t idx[] = {n, f};
            rtka_state_t feat = *rtka_tensor_get(node_features, idx);
            
            switch (method) {
                case MSG_SUM:
                case MSG_MEAN:
                    agg = rtka_combine_or(agg, feat);
                    break;
                case MSG_MAX:
                    if (feat.confidence > agg.confidence) {
                        agg = feat;
                    }
                    break;
                case MSG_TERNARY: {
                    rtka_state_t msgs[] = {agg, feat};
                    agg = rtka_gnn_aggregate_ternary(msgs, 2);
                    break;
                }
            }
        }
        
        if (method == MSG_MEAN) {
            agg.confidence /= num_nodes;
        }
        
        uint32_t out_idx[] = {0, f};
        rtka_tensor_set(pooled, out_idx, agg);
    }
    
    return pooled;
}
