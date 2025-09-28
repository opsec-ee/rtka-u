/**
 * File: rtka_evolution.c
 * Copyright (c) 2025 - H.Overman opsec.ee@pm.me
 * 
 * RTKA Evolutionary Algorithm Implementation
 */

#include "rtka_evolution.h"
#include "rtka_memory.h"
#include <stdlib.h>
#include <math.h>
#include <string.h>

/* Create population */
rtka_population_t* rtka_evolution_create_population(uint32_t pop_size, uint32_t gene_length) {
    rtka_population_t* pop = (rtka_population_t*)rtka_alloc_state();
    if (!pop) return NULL;
    
    pop->size = pop_size;
    pop->generation = 0;
    pop->mutation_rate = 0.01f;
    pop->crossover_rate = 0.7f;
    pop->elite_ratio = 0.1f;
    
    pop->individuals = (rtka_chromosome_t**)rtka_alloc_states(
        pop_size * sizeof(void*) / sizeof(rtka_state_t));
    
    for (uint32_t i = 0; i < pop_size; i++) {
        pop->individuals[i] = (rtka_chromosome_t*)rtka_alloc_state();
        pop->individuals[i]->genes = rtka_alloc_states(gene_length);
        pop->individuals[i]->length = gene_length;
        pop->individuals[i]->fitness = 0.0f;
        pop->individuals[i]->age = 0;
    }
    
    return pop;
}

/* Initialize with random ternary genes */
void rtka_evolution_initialize_random(rtka_population_t* pop) {
    for (uint32_t i = 0; i < pop->size; i++) {
        rtka_chromosome_t* chrom = pop->individuals[i];
        
        for (uint32_t g = 0; g < chrom->length; g++) {
            float r = rtka_random_float();
            if (r < 0.333f) {
                chrom->genes[g] = rtka_make_state(RTKA_FALSE, rtka_random_float());
            } else if (r < 0.667f) {
                chrom->genes[g] = rtka_make_state(RTKA_UNKNOWN, 0.5f);
            } else {
                chrom->genes[g] = rtka_make_state(RTKA_TRUE, rtka_random_float());
            }
        }
    }
}

/* Ternary crossover */
rtka_chromosome_t* rtka_evolution_crossover(const rtka_chromosome_t* parent1,
                                           const rtka_chromosome_t* parent2,
                                           rtka_confidence_t rate) {
    if (rtka_random_float() > rate) {
        /* Clone parent1 */
        rtka_chromosome_t* child = (rtka_chromosome_t*)rtka_alloc_state();
        child->genes = rtka_alloc_states(parent1->length);
        child->length = parent1->length;
        memcpy(child->genes, parent1->genes, parent1->length * sizeof(rtka_state_t));
        return child;
    }
    
    rtka_chromosome_t* child = (rtka_chromosome_t*)rtka_alloc_state();
    child->genes = rtka_alloc_states(parent1->length);
    child->length = parent1->length;
    child->age = 0;
    
    /* Ternary crossover: use UNKNOWN states as crossover points */
    for (uint32_t i = 0; i < parent1->length; i++) {
        if (parent1->genes[i].value == RTKA_UNKNOWN || 
            parent2->genes[i].value == RTKA_UNKNOWN) {
            /* Crossover point - blend confidences */
            rtka_state_t blended = rtka_combine_or(parent1->genes[i], parent2->genes[i]);
            child->genes[i] = blended;
        } else if (rtka_random_float() < 0.5f) {
            child->genes[i] = parent1->genes[i];
        } else {
            child->genes[i] = parent2->genes[i];
        }
    }
    
    return child;
}

/* Mutate chromosome */
void rtka_evolution_mutate(rtka_chromosome_t* chrom, rtka_confidence_t rate) {
    for (uint32_t i = 0; i < chrom->length; i++) {
        if (rtka_random_float() < rate) {
            rtka_mutate_ternary(&chrom->genes[i]);
        }
    }
}

/* Tournament selection */
rtka_chromosome_t* rtka_evolution_select(rtka_population_t* pop,
                                        rtka_selection_t method,
                                        uint32_t tournament_size) {
    switch (method) {
        case SELECT_TOURNAMENT: {
            rtka_chromosome_t* best = pop->individuals[rand() % pop->size];
            
            for (uint32_t i = 1; i < tournament_size; i++) {
                rtka_chromosome_t* competitor = pop->individuals[rand() % pop->size];
                if (competitor->fitness > best->fitness) {
                    best = competitor;
                }
            }
            return best;
        }
        
        case SELECT_TERNARY: {
            /* Ternary selection based on state values */
            uint32_t true_count = 0, false_count = 0, unknown_count = 0;
            
            for (uint32_t i = 0; i < pop->size; i++) {
                rtka_state_t first_gene = pop->individuals[i]->genes[0];
                switch (first_gene.value) {
                    case RTKA_TRUE: true_count++; break;
                    case RTKA_FALSE: false_count++; break;
                    case RTKA_UNKNOWN: unknown_count++; break;
                }
            }
            
            /* Select from dominant group */
            rtka_value_t target;
            if (true_count >= false_count && true_count >= unknown_count) {
                target = RTKA_TRUE;
            } else if (false_count >= unknown_count) {
                target = RTKA_FALSE;
            } else {
                target = RTKA_UNKNOWN;
            }
            
            for (uint32_t i = 0; i < pop->size; i++) {
                if (pop->individuals[i]->genes[0].value == target) {
                    return pop->individuals[i];
                }
            }
            break;
        }
        
        default:
            return pop->individuals[rand() % pop->size];
    }
    
    return pop->individuals[0];
}

/* Evolution step */
void rtka_evolution_step(rtka_population_t* pop,
                        rtka_fitness_fn fitness,
                        rtka_selection_t selection) {
    /* Evaluate fitness */
    for (uint32_t i = 0; i < pop->size; i++) {
        pop->individuals[i]->fitness = fitness(pop->individuals[i]);
        pop->individuals[i]->age++;
    }
    
    /* Sort by fitness */
    for (uint32_t i = 0; i < pop->size - 1; i++) {
        for (uint32_t j = i + 1; j < pop->size; j++) {
            if (pop->individuals[j]->fitness > pop->individuals[i]->fitness) {
                rtka_chromosome_t* temp = pop->individuals[i];
                pop->individuals[i] = pop->individuals[j];
                pop->individuals[j] = temp;
            }
        }
    }
    
    /* New generation */
    rtka_chromosome_t** new_pop = (rtka_chromosome_t**)rtka_alloc_states(
        pop->size * sizeof(void*) / sizeof(rtka_state_t));
    
    /* Elitism */
    uint32_t elite_count = (uint32_t)(pop->size * pop->elite_ratio);
    for (uint32_t i = 0; i < elite_count; i++) {
        new_pop[i] = pop->individuals[i];
    }
    
    /* Generate offspring */
    for (uint32_t i = elite_count; i < pop->size; i++) {
        rtka_chromosome_t* parent1 = rtka_evolution_select(pop, selection, 3);
        rtka_chromosome_t* parent2 = rtka_evolution_select(pop, selection, 3);
        
        rtka_chromosome_t* child = rtka_evolution_crossover(parent1, parent2, pop->crossover_rate);
        rtka_evolution_mutate(child, pop->mutation_rate);
        new_pop[i] = child;
    }
    
    pop->individuals = new_pop;
    pop->generation++;
}

/* Check convergence */
bool rtka_evolution_converged(rtka_population_t* pop, rtka_confidence_t threshold) {
    rtka_confidence_t best = pop->individuals[0]->fitness;
    rtka_confidence_t worst = pop->individuals[pop->size - 1]->fitness;
    
    return (best - worst) < threshold;
}

/* Get statistics */
rtka_evolution_stats_t rtka_evolution_get_stats(rtka_population_t* pop) {
    rtka_evolution_stats_t stats = {0};
    
    stats.best_fitness = pop->individuals[0]->fitness;
    
    rtka_confidence_t sum = 0.0f;
    for (uint32_t i = 0; i < pop->size; i++) {
        sum += pop->individuals[i]->fitness;
    }
    stats.avg_fitness = sum / pop->size;
    
    /* Diversity based on ternary states */
    uint32_t true_count = 0, false_count = 0, unknown_count = 0;
    
    for (uint32_t i = 0; i < pop->size; i++) {
        for (uint32_t g = 0; g < pop->individuals[i]->length; g++) {
            switch (pop->individuals[i]->genes[g].value) {
                case RTKA_TRUE: true_count++; break;
                case RTKA_FALSE: false_count++; break;
                case RTKA_UNKNOWN: unknown_count++; break;
            }
        }
    }
    
    uint32_t total = true_count + false_count + unknown_count;
    float p_true = (float)true_count / total;
    float p_false = (float)false_count / total;
    float p_unknown = (float)unknown_count / total;
    
    /* Shannon entropy for diversity */
    stats.diversity = 0.0f;
    if (p_true > 0) stats.diversity -= p_true * logf(p_true);
    if (p_false > 0) stats.diversity -= p_false * logf(p_false);
    if (p_unknown > 0) stats.diversity -= p_unknown * logf(p_unknown);
    stats.diversity /= logf(3.0f);  /* Normalize to [0,1] */
    
    return stats;
}

/* Random float implementation */
float rtka_random_float(void) {
    return (float)rand() / RAND_MAX;
}
