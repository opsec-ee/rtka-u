/**
 * File: rtka_graph.c
 * Copyright (c) 2025 - H.Overman opsec.ee@pm.me
 * 
 * PROPRIETARY AND CONFIDENTIAL
 * This file is proprietary to H. Overman and may not be reproduced,
 * distributed, or used without explicit written permission.
 *
 * RTKA Graph Implementation - Confidence Propagation Networks
 */

#include "rtka_graph.h"
#include "rtka_memory.h"
#include <string.h>
#include <math.h>

/* Create sparse graph */
rtka_graph_sparse_t* rtka_graph_create_sparse(uint32_t vertices) {
    rtka_graph_sparse_t* graph = (rtka_graph_sparse_t*)rtka_alloc_state();
    if (!graph) return NULL;
    
    graph->num_vertices = vertices;
    graph->num_edges = 0;
    graph->adjacency_list = (uint32_t*)rtka_alloc_states((vertices + 1) * sizeof(uint32_t) / sizeof(rtka_state_t));
    graph->vertex_states = rtka_alloc_states(vertices);
    graph->degree = (uint32_t*)rtka_alloc_states(vertices * sizeof(uint32_t) / sizeof(rtka_state_t));
    
    /* Initialize vertices to UNKNOWN */
    for (uint32_t i = 0; i < vertices; i++) {
        graph->vertex_states[i] = rtka_make_state(RTKA_UNKNOWN, 0.5f);
        graph->degree[i] = 0;
    }
    
    return graph;
}

/* Initialize PageRank for ternary graph */
rtka_pagerank_t* rtka_pagerank_init(rtka_graph_sparse_t* graph) {
    rtka_pagerank_t* pr = (rtka_pagerank_t*)rtka_alloc_state();
    if (!pr) return NULL;
    
    pr->ranks = rtka_alloc_states(graph->num_vertices);
    pr->importance = (rtka_confidence_t*)rtka_alloc_states(graph->num_vertices * sizeof(rtka_confidence_t) / sizeof(rtka_state_t));
    pr->iterations = 0;
    pr->convergence_threshold = 0.001f;
    
    /* Initialize with uniform distribution */
    rtka_confidence_t initial = 1.0f / graph->num_vertices;
    for (uint32_t i = 0; i < graph->num_vertices; i++) {
        pr->ranks[i] = rtka_make_state(RTKA_UNKNOWN, initial);
        pr->importance[i] = initial;
    }
    
    return pr;
}

/* PageRank iteration with ternary logic */
void rtka_pagerank_iterate(rtka_pagerank_t* pr, rtka_graph_sparse_t* graph) {
    rtka_confidence_t damping = 0.85f;
    rtka_confidence_t teleport = (1.0f - damping) / graph->num_vertices;
    
    rtka_state_t* new_ranks = rtka_alloc_states(graph->num_vertices);
    rtka_confidence_t* new_importance = (rtka_confidence_t*)rtka_alloc_states(
        graph->num_vertices * sizeof(rtka_confidence_t) / sizeof(rtka_state_t));
    
    for (uint32_t v = 0; v < graph->num_vertices; v++) {
        rtka_state_t rank_sum = rtka_make_state(RTKA_FALSE, 0.0f);
        rtka_confidence_t importance_sum = 0.0f;
        
        /* Iterate through incoming edges */
        for (uint32_t u = 0; u < graph->num_vertices; u++) {
            if (u == v) continue;
            
            /* Check edge existence in adjacency */
            uint32_t start = graph->adjacency_list[u];
            uint32_t end = graph->adjacency_list[u + 1];
            
            for (uint32_t e = start; e < end; e++) {
                if (graph->edge_indices[e] == v) {
                    /* Propagate confidence through edge */
                    rtka_state_t contribution = pr->ranks[u];
                    
                    if (graph->degree[u] > 0) {
                        contribution.confidence /= graph->degree[u];
                        
                        /* Ternary logic: combine contributions */
                        rank_sum = rtka_combine_or(rank_sum, contribution);
                        importance_sum += pr->importance[u] / graph->degree[u];
                    }
                    break;
                }
            }
        }
        
        /* Apply damping and teleportation */
        new_ranks[v].value = rank_sum.value;
        new_ranks[v].confidence = damping * rank_sum.confidence + teleport;
        new_importance[v] = damping * importance_sum + teleport;
        
        /* Ternary state transition based on confidence */
        if (new_ranks[v].confidence > 0.66f) {
            new_ranks[v].value = RTKA_TRUE;
        } else if (new_ranks[v].confidence < 0.33f) {
            new_ranks[v].value = RTKA_FALSE;
        } else {
            new_ranks[v].value = RTKA_UNKNOWN;
        }
    }
    
    memcpy(pr->ranks, new_ranks, graph->num_vertices * sizeof(rtka_state_t));
    memcpy(pr->importance, new_importance, graph->num_vertices * sizeof(rtka_confidence_t));
    pr->iterations++;
}

/* Check convergence */
bool rtka_pagerank_converged(rtka_pagerank_t* pr) {
    static rtka_confidence_t* prev_importance = NULL;
    
    if (!prev_importance) {
        prev_importance = (rtka_confidence_t*)rtka_alloc_states(
            pr->iterations * sizeof(rtka_confidence_t) / sizeof(rtka_state_t));
    }
    
    if (pr->iterations < 2) return false;
    
    rtka_confidence_t diff = 0.0f;
    for (uint32_t i = 0; i < pr->iterations; i++) {
        diff += fabsf(pr->importance[i] - prev_importance[i]);
    }
    
    memcpy(prev_importance, pr->importance, pr->iterations * sizeof(rtka_confidence_t));
    return diff < pr->convergence_threshold;
}

/* Markov transition matrix from graph */
rtka_transition_matrix_t* rtka_markov_from_graph(rtka_graph_sparse_t* graph) {
    rtka_transition_matrix_t* tm = (rtka_transition_matrix_t*)rtka_alloc_state();
    if (!tm) return NULL;
    
    tm->size = graph->num_vertices;
    tm->damping_factor = 0.85f;
    tm->matrix = (rtka_state_t**)rtka_alloc_states(tm->size * sizeof(void*) / sizeof(rtka_state_t));
    
    for (uint32_t i = 0; i < tm->size; i++) {
        tm->matrix[i] = rtka_alloc_states(tm->size);
        
        for (uint32_t j = 0; j < tm->size; j++) {
            /* Initialize transition probability */
            if (i == j) {
                tm->matrix[i][j] = rtka_make_state(RTKA_UNKNOWN, 0.1f);
            } else {
                tm->matrix[i][j] = rtka_make_state(RTKA_FALSE, 0.0f);
            }
        }
        
        /* Set transitions based on edges */
        uint32_t start = graph->adjacency_list[i];
        uint32_t end = graph->adjacency_list[i + 1];
        uint32_t out_degree = end - start;
        
        if (out_degree > 0) {
            rtka_confidence_t prob = 1.0f / out_degree;
            
            for (uint32_t e = start; e < end; e++) {
                uint32_t j = graph->edge_indices[e];
                rtka_state_t edge_weight = graph->edge_weights ? 
                    graph->edge_weights[e] : rtka_make_state(RTKA_TRUE, prob);
                
                /* Ternary transition probability */
                tm->matrix[i][j] = edge_weight;
                tm->matrix[i][j].confidence *= prob;
            }
        }
    }
    
    return tm;
}

/* Markov step with ternary states */
void rtka_markov_step(rtka_state_t* current, rtka_transition_matrix_t* tm, rtka_state_t* next) {
    for (uint32_t j = 0; j < tm->size; j++) {
        rtka_state_t new_state = rtka_make_state(RTKA_FALSE, 0.0f);
        
        for (uint32_t i = 0; i < tm->size; i++) {
            /* Ternary matrix multiplication */
            rtka_state_t contribution = rtka_combine_and(current[i], tm->matrix[i][j]);
            new_state = rtka_combine_or(new_state, contribution);
        }
        
        /* Apply damping for stability */
        new_state.confidence = tm->damping_factor * new_state.confidence + 
                               (1.0f - tm->damping_factor) / tm->size;
        
        /* Quantize to ternary */
        if (new_state.confidence > 0.66f) {
            new_state.value = RTKA_TRUE;
        } else if (new_state.confidence < 0.33f) {
            new_state.value = RTKA_FALSE;
        } else {
            new_state.value = RTKA_UNKNOWN;
        }
        
        next[j] = new_state;
    }
}

/* Confidence propagation through graph */
void rtka_propagate_confidence(rtka_graph_sparse_t* graph, uint32_t source, uint32_t max_depth) {
    if (source >= graph->num_vertices) return;
    
    /* BFS-like propagation with confidence decay */
    rtka_state_t* visited = rtka_alloc_states(graph->num_vertices);
    uint32_t* queue = (uint32_t*)rtka_alloc_states(graph->num_vertices * sizeof(uint32_t) / sizeof(rtka_state_t));
    uint32_t* depths = (uint32_t*)rtka_alloc_states(graph->num_vertices * sizeof(uint32_t) / sizeof(rtka_state_t));
    
    uint32_t head = 0, tail = 0;
    queue[tail++] = source;
    visited[source] = rtka_make_state(RTKA_TRUE, 1.0f);
    depths[source] = 0;
    
    while (head < tail && depths[queue[head]] < max_depth) {
        uint32_t v = queue[head++];
        uint32_t current_depth = depths[v];
        rtka_state_t v_state = graph->vertex_states[v];
        
        /* Propagate to neighbors */
        uint32_t start = graph->adjacency_list[v];
        uint32_t end = graph->adjacency_list[v + 1];
        
        for (uint32_t e = start; e < end; e++) {
            uint32_t u = graph->edge_indices[e];
            
            if (visited[u].value == RTKA_FALSE) {
                /* Calculate propagated confidence */
                rtka_state_t edge_weight = graph->edge_weights ? 
                    graph->edge_weights[e] : rtka_make_state(RTKA_TRUE, 1.0f);
                
                /* Ternary propagation rule */
                rtka_state_t propagated = rtka_combine_and(v_state, edge_weight);
                
                /* Decay with depth */
                rtka_confidence_t decay = powf(0.9f, current_depth + 1);
                propagated.confidence *= decay;
                
                /* Update neighbor state */
                graph->vertex_states[u] = rtka_combine_or(graph->vertex_states[u], propagated);
                
                if (current_depth + 1 < max_depth) {
                    queue[tail++] = u;
                    depths[u] = current_depth + 1;
                    visited[u] = rtka_make_state(RTKA_TRUE, propagated.confidence);
                }
            }
        }
    }
}

/* BFS with ternary states */
rtka_state_t* rtka_graph_bfs_ternary(rtka_graph_sparse_t* graph, uint32_t start) {
    rtka_state_t* distances = rtka_alloc_states(graph->num_vertices);
    
    for (uint32_t i = 0; i < graph->num_vertices; i++) {
        distances[i] = rtka_make_state(RTKA_UNKNOWN, 0.0f);
    }
    
    distances[start] = rtka_make_state(RTKA_TRUE, 1.0f);
    
    uint32_t* queue = (uint32_t*)rtka_alloc_states(graph->num_vertices * sizeof(uint32_t) / sizeof(rtka_state_t));
    uint32_t head = 0, tail = 0;
    queue[tail++] = start;
    
    while (head < tail) {
        uint32_t v = queue[head++];
        rtka_state_t v_dist = distances[v];
        
        uint32_t start_edge = graph->adjacency_list[v];
        uint32_t end_edge = graph->adjacency_list[v + 1];
        
        for (uint32_t e = start_edge; e < end_edge; e++) {
            uint32_t u = graph->edge_indices[e];
            
            if (distances[u].value == RTKA_UNKNOWN) {
                /* Propagate distance with confidence decay */
                distances[u].value = RTKA_TRUE;
                distances[u].confidence = v_dist.confidence * 0.9f;
                queue[tail++] = u;
            }
        }
    }
    
    return distances;
}
