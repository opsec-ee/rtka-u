/**
 * File: rtka_astar.h
 * Copyright (c) 2025 - H.Overman opsec.ee@pm.me
 * 
 * RTKA A* Search Algorithm
 */

#ifndef RTKA_ASTAR_H
#define RTKA_ASTAR_H

#include "rtka_u_core.h"
#include "rtka_types.h"

#define ASTAR_MAX_NODES 10000
#define ASTAR_MAX_NEIGHBORS 12

typedef struct rtka_astar_node {
    void* state;
    struct rtka_astar_node* parent;
    rtka_state_t g_score;  /* Cost from start - ternary */
    rtka_state_t h_score;  /* Heuristic - ternary */
    rtka_state_t f_score;  /* Total - ternary */
    uint32_t id;
} rtka_astar_node_t;

typedef struct {
    /* Problem-specific functions */
    bool (*is_goal)(void* state);
    void* (*get_neighbors)(void* state, uint32_t* count);
    rtka_state_t (*heuristic)(void* state, void* goal);
    rtka_state_t (*cost)(void* from, void* to);
    bool (*equals)(void* a, void* b);
    void* (*clone_state)(void* state);
    void (*free_state)(void* state);
    
    /* Search state */
    rtka_astar_node_t* open_set[ASTAR_MAX_NODES];
    rtka_astar_node_t* closed_set[ASTAR_MAX_NODES];
    uint32_t open_count;
    uint32_t closed_count;
    
    /* Statistics */
    uint32_t nodes_expanded;
    uint32_t rtka_transitions;
} rtka_astar_t;

/* A* operations */
rtka_astar_t* rtka_astar_create(void);
rtka_astar_node_t* rtka_astar_search(rtka_astar_t* astar, void* start, void* goal);
void rtka_astar_free(rtka_astar_t* astar);

/* Path reconstruction */
void** rtka_astar_get_path(rtka_astar_node_t* goal_node, uint32_t* path_length);

/* Priority queue operations with ternary ordering */
void rtka_astar_push(rtka_astar_t* astar, rtka_astar_node_t* node);
rtka_astar_node_t* rtka_astar_pop(rtka_astar_t* astar);

#endif
