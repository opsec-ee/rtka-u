/**
 * File: test_astar.c
 * Copyright (c) 2025 - H.Overman opsec.ee@pm.me
 * 
 * PROPRIETARY AND CONFIDENTIAL
 * This file is proprietary to H. Overman and may not be reproduced,
 * distributed, or used without explicit written permission.
 *
 * For licensing inquiries: opsec.ee@pm.me
 *
 * Test RTKA A* Search with Grid Pathfinding
 * 
 * CHANGELOG:
 * v1.0.1 - Fixed memory management for path states
 *          All path states are cloned so all must be freed
 */

#include "rtka_astar.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

#define GRID_WIDTH 10
#define GRID_HEIGHT 10

/* Grid state with ternary values:
 * TRUE = passable, FALSE = wall, UNKNOWN = difficult terrain */
typedef struct {
    rtka_state_t cells[GRID_HEIGHT][GRID_WIDTH];
    uint32_t x, y;
} grid_state_t;

/* Clone grid state */
void* clone_grid(void* state) {
    grid_state_t* s = (grid_state_t*)state;
    grid_state_t* clone = malloc(sizeof(grid_state_t));
    memcpy(clone, s, sizeof(grid_state_t));
    return clone;
}

/* Free grid state */
void free_grid(void* state) {
    free(state);
}

/* Check if states are equal */
bool equals_grid(void* a, void* b) {
    grid_state_t* ga = (grid_state_t*)a;
    grid_state_t* gb = (grid_state_t*)b;
    return ga->x == gb->x && ga->y == gb->y;
}

/* Check if goal */
bool is_goal_grid(void* state) {
    grid_state_t* s = (grid_state_t*)state;
    return s->x == GRID_WIDTH - 1 && s->y == GRID_HEIGHT - 1;
}

/* Get neighbors - returns static array */
void* get_neighbors_grid(void* state, uint32_t* count) {
    grid_state_t* s = (grid_state_t*)state;
    static grid_state_t* neighbors[4];
    *count = 0;
    
    /* 4-directional movement */
    int dx[] = {0, 1, 0, -1};
    int dy[] = {-1, 0, 1, 0};
    
    for (int i = 0; i < 4; i++) {
        int nx = s->x + dx[i];
        int ny = s->y + dy[i];
        
        if (nx >= 0 && nx < GRID_WIDTH && ny >= 0 && ny < GRID_HEIGHT) {
            /* Check if passable using ternary logic */
            if (s->cells[ny][nx].value != RTKA_FALSE) {
                grid_state_t* n = malloc(sizeof(grid_state_t));
                memcpy(n, s, sizeof(grid_state_t));
                n->x = nx;
                n->y = ny;
                neighbors[(*count)++] = n;
            }
        }
    }
    
    return neighbors;
}

/* Ternary heuristic */
rtka_state_t heuristic_grid(void* state, void* goal) {
    grid_state_t* s = (grid_state_t*)state;
    grid_state_t* g = (grid_state_t*)goal;
    
    int dx = (s->x > g->x) ? (s->x - g->x) : (g->x - s->x);
    int dy = (s->y > g->y) ? (s->y - g->y) : (g->y - s->y);
    float dist = (float)(dx + dy);
    
    /* Use ternary confidence for heuristic quality */
    rtka_confidence_t conf = 1.0f / (1.0f + dist);
    rtka_value_t val = (dist < 5) ? RTKA_TRUE : 
                      (dist > 10) ? RTKA_FALSE : RTKA_UNKNOWN;
    
    return rtka_make_state(val, conf);
}

/* Cost with ternary terrain */
rtka_state_t cost_grid(void* from, void* to) {
    (void)from; /* Not used in this simple example */
    grid_state_t* t = (grid_state_t*)to;
    
    /* Cost depends on terrain type */
    rtka_state_t terrain = t->cells[t->y][t->x];
    
    if (terrain.value == RTKA_TRUE) {
        return rtka_make_state(RTKA_TRUE, 1.0f);  /* Easy terrain */
    } else if (terrain.value == RTKA_UNKNOWN) {
        return rtka_make_state(RTKA_UNKNOWN, 2.0f);  /* Difficult terrain */
    } else {
        return rtka_make_state(RTKA_FALSE, 10.0f);  /* Should not happen */
    }
}

int main(void) {
    printf("RTKA A* Search Test\n");
    printf("===================\n\n");
    
    /* Create grid with ternary terrain */
    grid_state_t* start = malloc(sizeof(grid_state_t));
    start->x = 0;
    start->y = 0;
    
    /* Initialize grid */
    for (int y = 0; y < GRID_HEIGHT; y++) {
        for (int x = 0; x < GRID_WIDTH; x++) {
            if ((x == 5 && y < 8) || (y == 5 && x > 2)) {
                /* Walls */
                start->cells[y][x] = rtka_make_state(RTKA_FALSE, 1.0f);
            } else if (x >= 3 && x <= 7 && y >= 3 && y <= 7) {
                /* Difficult terrain */
                start->cells[y][x] = rtka_make_state(RTKA_UNKNOWN, 0.5f);
            } else {
                /* Clear path */
                start->cells[y][x] = rtka_make_state(RTKA_TRUE, 1.0f);
            }
        }
    }
    
    /* Print grid */
    printf("Grid (S=start, G=goal, #=wall, ?=difficult, .=clear):\n");
    for (int y = 0; y < GRID_HEIGHT; y++) {
        for (int x = 0; x < GRID_WIDTH; x++) {
            if (x == 0 && y == 0) printf("S");
            else if (x == GRID_WIDTH-1 && y == GRID_HEIGHT-1) printf("G");
            else if (start->cells[y][x].value == RTKA_FALSE) printf("#");
            else if (start->cells[y][x].value == RTKA_UNKNOWN) printf("?");
            else printf(".");
        }
        printf("\n");
    }
    
    /* Setup A* */
    rtka_astar_t* astar = rtka_astar_create();
    astar->is_goal = is_goal_grid;
    astar->get_neighbors = get_neighbors_grid;
    astar->heuristic = heuristic_grid;
    astar->cost = cost_grid;
    astar->equals = equals_grid;
    astar->clone_state = clone_grid;
    astar->free_state = free_grid;
    
    /* Create goal */
    grid_state_t* goal = malloc(sizeof(grid_state_t));
    memcpy(goal, start, sizeof(grid_state_t));
    goal->x = GRID_WIDTH - 1;
    goal->y = GRID_HEIGHT - 1;
    
    /* Search */
    printf("\nSearching...\n");
    rtka_astar_node_t* result = rtka_astar_search(astar, start, goal);
    
    if (result) {
        uint32_t path_length;
        void** path = rtka_astar_get_path(result, &path_length);
        
        printf("✅ Path found!\n");
        printf("Path Length: %d\n", path_length);
        printf("Nodes expanded: %d\n", astar->nodes_expanded);
        printf("RTKA transitions: %d\n", astar->rtka_transitions);
        
        /* Print path */
        printf("\nPath with ternary confidence:\n");
        for (uint32_t i = 0; i < path_length; i++) {
            grid_state_t* s = (grid_state_t*)path[i];
            rtka_state_t terrain = s->cells[s->y][s->x];
            printf("(%d,%d) - terrain: %s, conf: %.2f\n", 
                   s->x, s->y,
                   terrain.value == RTKA_TRUE ? "clear" :
                   terrain.value == RTKA_UNKNOWN ? "difficult" : "wall",
                   terrain.confidence);
            /* Don't free path states - they're owned by A* nodes */
        }
        
        free(path);
    } else {
        printf("❌ No path found\n");
    }
    
    rtka_astar_free(astar);
    free(start);
    free(goal);
    
    return 0;
}
