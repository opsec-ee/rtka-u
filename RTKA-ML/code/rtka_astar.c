/**
 * File: rtka_astar.c
 * Copyright (c) 2025 - H.Overman opsec.ee@pm.me
 * 
 * RTKA A* Search Implementation
 * 
 * CHANGELOG:
 * v1.0.1 - Fixed memory management for neighbors
 *          Removed free(neighbors) for static arrays (line 144)
 *          Fixed neighbor state access (line 130)
 */

#include "rtka_astar.h"
#include <stdlib.h>
#include <string.h>

/* Create A* context */
rtka_astar_t* rtka_astar_create(void) {
    rtka_astar_t* astar = malloc(sizeof(rtka_astar_t));
    if (!astar) return NULL;
    
    memset(astar, 0, sizeof(rtka_astar_t));
    return astar;
}

/* Compare nodes using ternary logic */
static int compare_nodes(rtka_astar_node_t* a, rtka_astar_node_t* b) {
    /* Compare f_scores with ternary values */
    if (a->f_score.value != b->f_score.value) {
        return a->f_score.value - b->f_score.value;
    }
    return (a->f_score.confidence < b->f_score.confidence) ? -1 : 1;
}

/* Push to open set (min heap) */
void rtka_astar_push(rtka_astar_t* astar, rtka_astar_node_t* node) {
    if (astar->open_count >= ASTAR_MAX_NODES) return;
    
    astar->open_set[astar->open_count] = node;
    
    /* Bubble up */
    uint32_t idx = astar->open_count;
    while (idx > 0) {
        uint32_t parent_idx = (idx - 1) / 2;
        if (compare_nodes(astar->open_set[idx], astar->open_set[parent_idx]) < 0) {
            rtka_astar_node_t* temp = astar->open_set[idx];
            astar->open_set[idx] = astar->open_set[parent_idx];
            astar->open_set[parent_idx] = temp;
            idx = parent_idx;
        } else {
            break;
        }
    }
    
    astar->open_count++;
    astar->rtka_transitions++;
}

/* Pop from open set */
rtka_astar_node_t* rtka_astar_pop(rtka_astar_t* astar) {
    if (astar->open_count == 0) return NULL;
    
    rtka_astar_node_t* result = astar->open_set[0];
    astar->open_set[0] = astar->open_set[--astar->open_count];
    
    /* Bubble down */
    uint32_t idx = 0;
    while (2 * idx + 1 < astar->open_count) {
        uint32_t left = 2 * idx + 1;
        uint32_t right = 2 * idx + 2;
        uint32_t smallest = idx;
        
        if (compare_nodes(astar->open_set[left], astar->open_set[smallest]) < 0) {
            smallest = left;
        }
        if (right < astar->open_count && 
            compare_nodes(astar->open_set[right], astar->open_set[smallest]) < 0) {
            smallest = right;
        }
        
        if (smallest != idx) {
            rtka_astar_node_t* temp = astar->open_set[idx];
            astar->open_set[idx] = astar->open_set[smallest];
            astar->open_set[smallest] = temp;
            idx = smallest;
        } else {
            break;
        }
    }
    
    return result;
}

/* Find node in set */
static rtka_astar_node_t* find_in_set(rtka_astar_node_t** set, uint32_t count, 
                                      void* state, rtka_astar_t* astar) {
    for (uint32_t i = 0; i < count; i++) {
        if (astar->equals(set[i]->state, state)) {
            return set[i];
        }
    }
    return NULL;
}

/* A* search with ternary logic */
rtka_astar_node_t* rtka_astar_search(rtka_astar_t* astar, void* start, void* goal) {
    /* Initialize start node */
    rtka_astar_node_t* start_node = malloc(sizeof(rtka_astar_node_t));
    start_node->state = astar->clone_state(start);
    start_node->parent = NULL;
    start_node->g_score = rtka_make_state(RTKA_TRUE, 0.0f);
    start_node->h_score = astar->heuristic(start, goal);
    start_node->f_score = rtka_combine_or(start_node->g_score, start_node->h_score);
    start_node->id = 0;
    
    rtka_astar_push(astar, start_node);
    
    while (astar->open_count > 0) {
        rtka_astar_node_t* current = rtka_astar_pop(astar);
        astar->nodes_expanded++;
        
        /* Goal test */
        if (astar->is_goal(current->state)) {
            return current;
        }
        
        /* Add to closed set */
        astar->closed_set[astar->closed_count++] = current;
        
        /* Expand neighbors */
        uint32_t neighbor_count = 0;
        void* neighbors = astar->get_neighbors(current->state, &neighbor_count);
        
        /* Cast neighbors to array of pointers */
        void** neighbor_array = (void**)neighbors;
        
        for (uint32_t i = 0; i < neighbor_count; i++) {
            void* neighbor_state = neighbor_array[i];
            
            /* Check if in closed set */
            rtka_astar_node_t* closed_node = find_in_set(astar->closed_set, 
                                                        astar->closed_count,
                                                        neighbor_state, astar);
            if (closed_node) {
                astar->free_state(neighbor_state);
                continue;
            }
            
            /* Calculate tentative g_score */
            rtka_state_t cost = astar->cost(current->state, neighbor_state);
            rtka_state_t tentative_g = rtka_combine_or(current->g_score, cost);
            
            /* Check if in open set */
            rtka_astar_node_t* neighbor_node = find_in_set(astar->open_set,
                                                          astar->open_count,
                                                          neighbor_state, astar);
            
            if (!neighbor_node) {
                /* New node */
                neighbor_node = malloc(sizeof(rtka_astar_node_t));
                neighbor_node->state = neighbor_state;
                neighbor_node->parent = current;
                neighbor_node->g_score = tentative_g;
                neighbor_node->h_score = astar->heuristic(neighbor_state, goal);
                neighbor_node->f_score = rtka_combine_or(neighbor_node->g_score,
                                                        neighbor_node->h_score);
                neighbor_node->id = ++astar->nodes_expanded;
                
                rtka_astar_push(astar, neighbor_node);
                astar->rtka_transitions++;
            } else if (tentative_g.confidence < neighbor_node->g_score.confidence) {
                /* Found better path */
                astar->free_state(neighbor_state); /* Free duplicate */
                neighbor_node->parent = current;
                neighbor_node->g_score = tentative_g;
                neighbor_node->f_score = rtka_combine_or(neighbor_node->g_score,
                                                         neighbor_node->h_score);
                astar->rtka_transitions++;
            } else {
                astar->free_state(neighbor_state); /* Free unused */
            }
        }
        
        /* Don't free static array - neighbors is managed by caller */
    }
    
    return NULL;  /* No path found */
}

/* Reconstruct path */
void** rtka_astar_get_path(rtka_astar_node_t* goal_node, uint32_t* path_length) {
    if (!goal_node) return NULL;
    
    /* Count path length */
    *path_length = 0;
    rtka_astar_node_t* node = goal_node;
    while (node) {
        (*path_length)++;
        node = node->parent;
    }
    
    /* Build path array */
    void** path = malloc(*path_length * sizeof(void*));
    node = goal_node;
    for (int i = *path_length - 1; i >= 0; i--) {
        path[i] = node->state; /* Direct reference - caller must handle cloning if needed */
        node = node->parent;
    }
    
    return path;
}

/* Free A* context */
void rtka_astar_free(rtka_astar_t* astar) {
    if (!astar) return;
    
    for (uint32_t i = 0; i < astar->open_count; i++) {
        astar->free_state(astar->open_set[i]->state);
        free(astar->open_set[i]);
    }
    
    for (uint32_t i = 0; i < astar->closed_count; i++) {
        astar->free_state(astar->closed_set[i]->state);
        free(astar->closed_set[i]);
    }
    
    free(astar);
}
