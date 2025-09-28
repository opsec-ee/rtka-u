/**
 * File: rtka_graph.h
 * Copyright (c) 2025 - H.Overman opsec.ee@pm.me
 * 
 * PROPRIETARY AND CONFIDENTIAL
 * This file is proprietary to H. Overman and may not be reproduced,
 * distributed, or used without explicit written permission.
 *
 * RTKA Graph Framework - PageRank-inspired Confidence Propagation
 */

#ifndef RTKA_GRAPH_H
#define RTKA_GRAPH_H

#include "rtka_types.h"
#include "rtka_tensor.h"
#include "rtka_vector.h"

/* Graph representation */
typedef struct {
    uint32_t num_vertices;
    uint32_t num_edges;
    uint32_t* adjacency_list;  /* CSR format */
    uint32_t* edge_indices;
    rtka_state_t* vertex_states;
    rtka_state_t* edge_weights;
    uint32_t* degree;
} rtka_graph_sparse_t;

/* Transition matrix for Markov-like processes */
typedef struct {
    rtka_state_t** matrix;  /* Ternary transition probabilities */
    uint32_t size;
    rtka_confidence_t damping_factor;  /* Like PageRank damping */
} rtka_transition_matrix_t;

/* PageRank-style algorithms for ternary */
typedef struct {
    rtka_state_t* ranks;
    rtka_confidence_t* importance;
    uint32_t iterations;
    rtka_confidence_t convergence_threshold;
} rtka_pagerank_t;

/* Graph creation */
rtka_graph_sparse_t* rtka_graph_create_sparse(uint32_t vertices);
void rtka_graph_add_edge_weighted(rtka_graph_sparse_t* graph, 
                                  uint32_t from, uint32_t to, 
                                  rtka_state_t weight);
void rtka_graph_free_sparse(rtka_graph_sparse_t* graph);

/* Ternary PageRank - confidence flows through graph */
rtka_pagerank_t* rtka_pagerank_init(rtka_graph_sparse_t* graph);
void rtka_pagerank_iterate(rtka_pagerank_t* pr, rtka_graph_sparse_t* graph);
bool rtka_pagerank_converged(rtka_pagerank_t* pr);

/* Markov chain operations with ternary states */
rtka_transition_matrix_t* rtka_markov_from_graph(rtka_graph_sparse_t* graph);
rtka_state_t* rtka_markov_steady_state(rtka_transition_matrix_t* tm);
void rtka_markov_step(rtka_state_t* current, rtka_transition_matrix_t* tm, rtka_state_t* next);

/* Confidence propagation - recursive like belief propagation */
void rtka_propagate_confidence(rtka_graph_sparse_t* graph, 
                              uint32_t source,
                              uint32_t max_depth);

/* Random walk with ternary decisions */
typedef struct {
    uint32_t current_vertex;
    rtka_state_t path_confidence;
    uint32_t* path;
    uint32_t path_length;
} rtka_random_walk_t;

rtka_random_walk_t* rtka_random_walk_init(rtka_graph_sparse_t* graph, uint32_t start);
uint32_t rtka_random_walk_step(rtka_random_walk_t* walk, rtka_graph_sparse_t* graph);

/* Graph algorithms with ternary logic */
rtka_state_t* rtka_graph_bfs_ternary(rtka_graph_sparse_t* graph, uint32_t start);
rtka_state_t* rtka_graph_dijkstra_ternary(rtka_graph_sparse_t* graph, uint32_t start);

/* Community detection with ternary states */
uint32_t* rtka_graph_louvain_ternary(rtka_graph_sparse_t* graph);

/* Centrality measures */
rtka_confidence_t* rtka_graph_betweenness(rtka_graph_sparse_t* graph);
rtka_confidence_t* rtka_graph_closeness(rtka_graph_sparse_t* graph);
rtka_confidence_t* rtka_graph_eigenvector_centrality(rtka_graph_sparse_t* graph);

/* Graph coloring with ternary constraints */
rtka_value_t* rtka_graph_color(rtka_graph_sparse_t* graph, uint32_t max_colors);

/* Maximum flow with ternary capacities */
rtka_state_t rtka_graph_max_flow(rtka_graph_sparse_t* graph, uint32_t source, uint32_t sink);

#endif /* RTKA_GRAPH_H */
