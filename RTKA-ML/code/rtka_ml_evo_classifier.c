/**
 * File: rtka_ml_evo_classifier.c
 * Author H. Overman
 * Date 2025-09-18
 * Copyright (c) 2025 - H. Overman <opsec.ee@pm.me>
 */

/*
CHANGELOG:
v1.2 - Enhanced for classifier tasks with recursive refinement
- Added batch convergence for fitness updates (lines 80-90): Balances responsiveness in noisy datasets like MNIST.
- Applied unsigned for counts/indices: Prevents signed bugs in bitwise ops.
- Used void casts for ignored printf: Documents intentional discard.
- Split eval into helper: One responsibility per function.
- Hoisted invariants in loops: Minimizes branches.
- Integrated recursive UNKNOWN refinement on low-fitness: Ties to infinite recursion for ambiguous weights.
*/

#include "rtka_ml_evolutionary.h"  // Base evo
#include "rtka_u_recursive_data.c"  // Recursive nodes

// Helper: Evaluate single member
float eval_member_fitness(population_member_t* member,
                          ternary_tensor_t** val_inputs,
                          ternary_tensor_t** val_labels,
                          size_t num_val) {
    return evaluate_fitness(member, val_inputs, val_labels, num_val);  // From base
}

population_member_t* evolutionary_train_classifier(const size_t* layer_sizes,
                                                   size_t num_layers,
                                                   ternary_tensor_t** train_inputs,
                                                   ternary_tensor_t** train_labels,
                                                   size_t num_train,
                                                   ternary_tensor_t** val_inputs,
                                                   ternary_tensor_t** val_labels,
                                                   size_t num_val,
                                                   evolutionary_config_t* config) {
    evolutionary_population_t* pop = create_population(layer_sizes, num_layers, config);
    if (!pop) return NULL;

    size_t pop_size = POPULATION_SIZE;  // Hoist
    for (uint32_t gen = 0U; gen < config->max_generations; gen++) {
        // Eval population (unrolled for throughput)
        for (uint32_t i = 0U; i < pop_size; i += 4U) {
            pop->members[i]->fitness = eval_member_fitness(pop->members[i], val_inputs, val_labels, num_val);
            if (i + 1U < pop_size) pop->members[i + 1U]->fitness = eval_member_fitness(pop->members[i + 1U], val_inputs, val_labels, num_val);
            if (i + 2U < pop_size) pop->members[i + 2U]->fitness = eval_member_fitness(pop->members[i + 2U], val_inputs, val_labels, num_val);
            if (i + 3U < pop_size) pop->members[i + 3U]->fitness = eval_member_fitness(pop->members[i + 3U], val_inputs, val_labels, num_val);
        }

        // Batch fitness convergence
        float fitness_values[POPULATION_SIZE] = {0};
        for (uint32_t i = 0U; i < pop_size; i++) {
            fitness_values[i] = pop->members[i]->fitness;
        }
        vector_convergence_t vc = {.values = fitness_values, .count = pop_size, .convergence_rate = 0.9f};
        update_vector_convergence(&vc, fitness_values);  // Adapt batch

        // Selection/crossover/mutation (base logic)...

        // Refine low-fitness with recursion
        for (uint32_t i = 0U; i < pop_size; i++) {
            if (pop->members[i]->fitness < 0.5f) {
                recursive_unknown_node_t* node = init_recursive_node(RTKA_UNKNOWN, pop->members[i]->fitness, NULL, 5U);
                rtka_state_t refined = evaluate_recursive_unknown(node, logf(0.1f));
                pop->members[i]->fitness = refined.confidence;  // Update
                free_recursive_node(node);
            }
        }

        (void)printf("Gen %u: Best %.4f\n", gen, pop->best_fitness);
    }

    // Extract best (base)...
    return best_member;
}
