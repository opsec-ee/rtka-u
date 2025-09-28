/**
 * File: rtka_evolution.h
 * Copyright (c) 2025 - H.Overman opsec.ee@pm.me
 * 
 * RTKA Evolutionary Algorithms with Ternary Genetics
 */

#ifndef RTKA_EVOLUTION_H
#define RTKA_EVOLUTION_H

#include "rtka_types.h"
#include "rtka_tensor.h"

/* Chromosome with ternary genes */
typedef struct {
    rtka_state_t* genes;
    uint32_t length;
    rtka_confidence_t fitness;
    uint32_t age;
} rtka_chromosome_t;

/* Population */
typedef struct {
    rtka_chromosome_t** individuals;
    uint32_t size;
    uint32_t generation;
    rtka_confidence_t mutation_rate;
    rtka_confidence_t crossover_rate;
    rtka_confidence_t elite_ratio;
} rtka_population_t;

/* Fitness function signature */
typedef rtka_confidence_t (*rtka_fitness_fn)(const rtka_chromosome_t*);

/* Random float forward declaration */
float rtka_random_float(void);

/* Selection strategies */
typedef enum {
    SELECT_ROULETTE,
    SELECT_TOURNAMENT,
    SELECT_RANK,
    SELECT_TERNARY  /* Ternary-aware selection */
} rtka_selection_t;

/* Population management */
rtka_population_t* rtka_evolution_create_population(uint32_t pop_size, uint32_t gene_length);
void rtka_evolution_free_population(rtka_population_t* pop);
void rtka_evolution_initialize_random(rtka_population_t* pop);

/* Genetic operators */
rtka_chromosome_t* rtka_evolution_crossover(const rtka_chromosome_t* parent1,
                                           const rtka_chromosome_t* parent2,
                                           rtka_confidence_t rate);

void rtka_evolution_mutate(rtka_chromosome_t* chrom, rtka_confidence_t rate);

/* Ternary-specific mutation */
RTKA_INLINE void rtka_mutate_ternary(rtka_state_t* gene) {
    /* Rotate through ternary states */
    switch (gene->value) {
        case RTKA_FALSE:
            gene->value = RTKA_UNKNOWN;
            gene->confidence *= 0.5f;
            break;
        case RTKA_UNKNOWN:
            gene->value = (rtka_random_float() > 0.5f) ? RTKA_TRUE : RTKA_FALSE;
            gene->confidence = rtka_random_float();
            break;
        case RTKA_TRUE:
            gene->value = RTKA_FALSE;
            gene->confidence *= 0.5f;
            break;
    }
}

/* Selection */
rtka_chromosome_t* rtka_evolution_select(rtka_population_t* pop, 
                                        rtka_selection_t method,
                                        uint32_t tournament_size);

/* Evolution step */
void rtka_evolution_step(rtka_population_t* pop, 
                        rtka_fitness_fn fitness,
                        rtka_selection_t selection);

/* Convergence detection */
bool rtka_evolution_converged(rtka_population_t* pop, rtka_confidence_t threshold);

/* Statistics */
typedef struct {
    rtka_confidence_t best_fitness;
    rtka_confidence_t avg_fitness;
    rtka_confidence_t diversity;
    uint32_t unique_solutions;
} rtka_evolution_stats_t;

rtka_evolution_stats_t rtka_evolution_get_stats(rtka_population_t* pop);

/* Multi-objective optimization */
typedef struct {
    rtka_chromosome_t** pareto_front;
    uint32_t front_size;
    rtka_fitness_fn* objectives;
    uint32_t num_objectives;
} rtka_multiobjective_t;

void rtka_evolution_nsga2(rtka_population_t* pop, 
                         rtka_fitness_fn* objectives,
                         uint32_t num_objectives);

/* Island model for parallel evolution */
typedef struct {
    rtka_population_t** islands;
    uint32_t num_islands;
    rtka_confidence_t migration_rate;
    uint32_t migration_interval;
} rtka_island_model_t;

rtka_island_model_t* rtka_evolution_create_islands(uint32_t num_islands,
                                                   uint32_t pop_per_island,
                                                   uint32_t gene_length);
void rtka_evolution_migrate(rtka_island_model_t* islands);

/* Coevolution */
void rtka_evolution_coevolve(rtka_population_t* pop1, 
                            rtka_population_t* pop2,
                            rtka_fitness_fn interaction_fitness);

#endif /* RTKA_EVOLUTION_H */
