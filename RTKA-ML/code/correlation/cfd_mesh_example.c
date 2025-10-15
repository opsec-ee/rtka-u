/**
 * File: cfd_mesh_example.c
 * Copyright (c) 2025 - H.Overman opsec.ee@pm.me
 * Email: opsec.ee@pm.me
 * 
 * CFD Mesh Quality Analysis - Practical Example
 * 
 * Demonstrates using correlation analysis for CFD mesh validation
 * and quality assessment, based on real mesh statistics.
 * 
 * Example uses mesh from uploaded image:
 * - nnodes = 4096
 * - nedges = 14880
 * - nfaces = 17310
 * - ncells = 6525
 * - Euler characteristic = 1
 */

#include "rtka_correlation.h"
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <time.h>

/* ============================================================================
 * MESH TOPOLOGY VALIDATION (EULER CHARACTERISTIC)
 * ============================================================================ */

typedef struct {
    size_t vertices;
    size_t edges;
    size_t faces;
    size_t cells;
} mesh_topology_t;

/**
 * Validate mesh topology using Euler characteristic
 * χ = V - E + F - C
 */
static bool validate_euler_characteristic(
    const mesh_topology_t* mesh,
    int expected,
    int* out_euler
) {
    /* Calculate Euler characteristic */
    int euler = (int)mesh->vertices - (int)mesh->edges + 
                (int)mesh->faces - (int)mesh->cells;
    
    if (out_euler != NULL) {
        *out_euler = euler;
    }
    
    printf("Euler Characteristic Validation:\n");
    printf("  V (vertices) = %zu\n", mesh->vertices);
    printf("  E (edges)    = %zu\n", mesh->edges);
    printf("  F (faces)    = %zu\n", mesh->faces);
    printf("  C (cells)    = %zu\n", mesh->cells);
    printf("  χ = V - E + F - C = %d\n", euler);
    printf("  Expected χ = %d\n", expected);
    printf("  Status: %s\n\n", 
           (euler == expected) ? "VALID" : "INVALID");
    
    return (euler == expected);
}

/* ============================================================================
 * MESH QUALITY METRICS GENERATION
 * ============================================================================ */

/**
 * Generate realistic mesh quality metrics
 * Based on typical CFD mesh characteristics
 */
static void generate_mesh_quality_data(
    double* aspect_ratios,
    double* skewness,
    double* volumes,
    size_t num_cells,
    double base_quality
) {
    /* Seed for reproducibility */
    srand(42U);
    
    for (size_t idx = 0U; idx < num_cells; idx++) {
        /* Aspect ratio: typically 1-10 for good meshes, up to 100 for poor */
        double ar_noise = ((double)rand() / (double)RAND_MAX - 0.5) * 2.0;
        aspect_ratios[idx] = base_quality * 5.0 + ar_noise * 3.0;
        if (aspect_ratios[idx] < 1.0) {
            aspect_ratios[idx] = 1.0;
        }
        
        /* Skewness: 0 (ideal) to 1 (degenerate) */
        double skew_noise = ((double)rand() / (double)RAND_MAX);
        skewness[idx] = (1.0 - base_quality) * 0.3 + skew_noise * 0.1;
        if (skewness[idx] < 0.0) {
            skewness[idx] = 0.0;
        }
        if (skewness[idx] > 1.0) {
            skewness[idx] = 1.0;
        }
        
        /* Cell volume: normalized around 1.0 */
        double vol_noise = ((double)rand() / (double)RAND_MAX - 0.5);
        volumes[idx] = 1.0 + vol_noise * 0.3;
        if (volumes[idx] < 0.1) {
            volumes[idx] = 0.1;
        }
    }
}

/**
 * Generate solution quality metrics
 * Correlated with mesh quality (realistic physical behavior)
 */
static void generate_solution_quality_data(
    const double* aspect_ratios,
    const double* skewness,
    size_t num_cells,
    double* residuals,
    double* errors
) {
    for (size_t idx = 0U; idx < num_cells; idx++) {
        /* Residuals increase with poor mesh quality */
        double ar_factor = log10(aspect_ratios[idx]) / log10(100.0);
        double skew_factor = skewness[idx];
        
        double noise = ((double)rand() / (double)RAND_MAX - 0.5) * 0.2;
        
        /* Residual magnitude correlated with mesh quality */
        residuals[idx] = 1e-6 * (1.0 + ar_factor * 10.0 + skew_factor * 5.0 + noise);
        
        /* Numerical error also correlates with mesh quality */
        errors[idx] = 1e-3 * (1.0 + ar_factor * 8.0 + skew_factor * 12.0 + noise);
    }
}

/* ============================================================================
 * CORRELATION ANALYSIS
 * ============================================================================ */

static void analyze_mesh_solution_correlation(
    const double* mesh_metric,
    const double* solution_metric,
    size_t num_cells,
    const char* mesh_name,
    const char* solution_name
) {
    double correlation = 0.0;
    rtka_corr_error_t err = rtka_corr_pearson_simple(
        mesh_metric, solution_metric, num_cells, &correlation
    );
    
    if (err == RTKA_CORR_SUCCESS) {
        const char* strength = rtka_corr_interpret(correlation);
        bool significant = rtka_corr_is_significant(correlation, num_cells, 0.95);
        
        printf("  %s vs %s:\n", mesh_name, solution_name);
        printf("    Correlation: %.4f (%s)\n", correlation, strength);
        printf("    Significant (95%%): %s\n", significant ? "Yes" : "No");
        
        /* Provide CFD-specific interpretation */
        if (fabs(correlation) > 0.7) {
            printf("    Impact: HIGH - %s strongly affects %s\n",
                   mesh_name, solution_name);
            printf("    Action: Prioritize improving %s\n", mesh_name);
        } else if (fabs(correlation) > 0.4) {
            printf("    Impact: MODERATE - %s has noticeable effect on %s\n",
                   mesh_name, solution_name);
            printf("    Action: Consider improving %s for better accuracy\n",
                   mesh_name);
        } else {
            printf("    Impact: LOW - %s has minimal effect on %s\n",
                   mesh_name, solution_name);
        }
        printf("\n");
    } else {
        printf("  Error analyzing %s vs %s: %d\n\n",
               mesh_name, solution_name, err);
    }
}

/* ============================================================================
 * MESH QUALITY STATISTICS
 * ============================================================================ */

static void compute_mesh_statistics(
    const double* aspect_ratios,
    const double* skewness,
    size_t num_cells,
    double ar_threshold,
    double skew_threshold
) {
    rtka_stats_t ar_stats = {0};
    rtka_stats_t skew_stats = {0};
    
    rtka_corr_statistics(aspect_ratios, num_cells, &ar_stats);
    rtka_corr_statistics(skewness, num_cells, &skew_stats);
    
    /* Count poor quality cells */
    size_t poor_ar = 0U;
    size_t poor_skew = 0U;
    
    for (size_t idx = 0U; idx < num_cells; idx++) {
        if (aspect_ratios[idx] > ar_threshold) {
            poor_ar++;
        }
        if (skewness[idx] > skew_threshold) {
            poor_skew++;
        }
    }
    
    printf("Mesh Quality Statistics:\n");
    printf("  Aspect Ratio:\n");
    printf("    Mean:   %.2f\n", ar_stats.mean);
    printf("    Std Dev: %.2f\n", ar_stats.std_dev);
    printf("    Poor cells (AR > %.0f): %zu (%.1f%%)\n",
           ar_threshold, poor_ar, 100.0 * (double)poor_ar / (double)num_cells);
    printf("\n");
    
    printf("  Skewness:\n");
    printf("    Mean:   %.3f\n", skew_stats.mean);
    printf("    Std Dev: %.3f\n", skew_stats.std_dev);
    printf("    Poor cells (Skew > %.2f): %zu (%.1f%%)\n",
           skew_threshold, poor_skew,
           100.0 * (double)poor_skew / (double)num_cells);
    printf("\n");
}

/* ============================================================================
 * MESH COMPARISON (GRID CONVERGENCE STUDY)
 * ============================================================================ */

static void grid_convergence_study(void) {
    printf("========================================\n");
    printf("Grid Convergence Study\n");
    printf("========================================\n\n");
    
    /* Simulate 4 meshes: coarse, medium, fine, very fine */
    const size_t num_meshes = 4U;
    double cell_counts[4] = {1000.0, 4000.0, 16000.0, 64000.0};
    double convergence_rates[4] = {1e-3, 3e-4, 8e-5, 2e-5};  /* Improving with refinement */
    double mean_aspect_ratios[4] = {8.5, 6.2, 4.1, 2.8};     /* Improving with refinement */
    
    printf("Mesh Refinement Levels:\n");
    for (size_t idx = 0U; idx < num_meshes; idx++) {
        printf("  Level %zu: %.0f cells, AR=%.1f, Conv=%.2e\n",
               idx, cell_counts[idx], mean_aspect_ratios[idx],
               convergence_rates[idx]);
    }
    printf("\n");
    
    /* Correlate mesh refinement with convergence */
    double corr_cells_conv = 0.0;
    double corr_ar_conv = 0.0;
    
    rtka_corr_pearson_simple(
        cell_counts, convergence_rates, num_meshes, &corr_cells_conv
    );
    
    rtka_corr_pearson_simple(
        mean_aspect_ratios, convergence_rates, num_meshes, &corr_ar_conv
    );
    
    printf("Grid Convergence Correlations:\n");
    printf("  Cell Count vs Convergence Rate: %.4f\n", corr_cells_conv);
    printf("  Mean Aspect Ratio vs Convergence: %.4f\n", corr_ar_conv);
    printf("\n");
    
    if (corr_cells_conv < -0.8) {
        printf("Finding: Strong negative correlation confirms mesh refinement\n");
        printf("         improves convergence (lower residuals with more cells).\n");
    }
    
    if (corr_ar_conv > 0.7) {
        printf("Finding: Better aspect ratios (lower values) correlate with\n");
        printf("         better convergence, confirming mesh quality importance.\n");
    }
    printf("\n");
}

/* ============================================================================
 * MAIN DEMONSTRATION
 * ============================================================================ */

int main(void) {
    printf("========================================\n");
    printf("CFD Mesh Quality Analysis\n");
    printf("Using RTKA Correlation Library\n");
    printf("Copyright (c) 2025 - H.Overman\n");
    printf("Email: opsec.ee@pm.me\n");
    printf("========================================\n\n");
    
    /* ========================================================================
     * EXAMPLE 1: EULER CHARACTERISTIC VALIDATION
     * ======================================================================== */
    
    printf("========================================\n");
    printf("Example 1: Mesh Topology Validation\n");
    printf("========================================\n\n");
    
    /* Real mesh from uploaded image */
    mesh_topology_t mesh = {
        .vertices = 4096U,
        .edges = 14880U,
        .faces = 17310U,
        .cells = 6525U
    };
    
    int euler = 0;
    bool is_valid = validate_euler_characteristic(&mesh, 1, &euler);
    
    if (is_valid) {
        printf("Result: Mesh topology is VALID for single-domain CFD grid\n");
    } else {
        printf("Warning: Mesh topology may have issues\n");
        printf("         Check for holes, disconnected regions, or\n");
        printf("         incorrect boundary conditions\n");
    }
    printf("\n");
    
    /* ========================================================================
     * EXAMPLE 2: MESH QUALITY ANALYSIS
     * ======================================================================== */
    
    printf("========================================\n");
    printf("Example 2: Mesh Quality Analysis\n");
    printf("========================================\n\n");
    
    const size_t num_cells = 1000U;  /* Subset for demonstration */
    
    double* aspect_ratios = malloc(num_cells * sizeof(double));
    double* skewness = malloc(num_cells * sizeof(double));
    double* volumes = malloc(num_cells * sizeof(double));
    double* residuals = malloc(num_cells * sizeof(double));
    double* errors = malloc(num_cells * sizeof(double));
    
    if (!aspect_ratios || !skewness || !volumes || !residuals || !errors) {
        printf("Error: Memory allocation failed\n");
        return 1;
    }
    
    /* Generate mesh quality data (base_quality: 0.7 = reasonable mesh) */
    generate_mesh_quality_data(
        aspect_ratios, skewness, volumes, num_cells, 0.7
    );
    
    /* Generate correlated solution data */
    generate_solution_quality_data(
        aspect_ratios, skewness, num_cells, residuals, errors
    );
    
    /* Compute statistics */
    compute_mesh_statistics(aspect_ratios, skewness, num_cells, 10.0, 0.5);
    
    /* ========================================================================
     * EXAMPLE 3: CORRELATION ANALYSIS
     * ======================================================================== */
    
    printf("========================================\n");
    printf("Example 3: Mesh-Solution Correlations\n");
    printf("========================================\n\n");
    
    printf("Analyzing relationships between mesh quality and solution:\n\n");
    
    analyze_mesh_solution_correlation(
        aspect_ratios, residuals, num_cells,
        "Aspect Ratio", "Residual Magnitude"
    );
    
    analyze_mesh_solution_correlation(
        skewness, residuals, num_cells,
        "Skewness", "Residual Magnitude"
    );
    
    analyze_mesh_solution_correlation(
        aspect_ratios, errors, num_cells,
        "Aspect Ratio", "Numerical Error"
    );
    
    analyze_mesh_solution_correlation(
        skewness, errors, num_cells,
        "Skewness", "Numerical Error"
    );
    
    /* ========================================================================
     * EXAMPLE 4: GRID CONVERGENCE STUDY
     * ======================================================================== */
    
    grid_convergence_study();
    
    /* ========================================================================
     * CLEANUP
     * ======================================================================== */
    
    free(aspect_ratios);
    free(skewness);
    free(volumes);
    free(residuals);
    free(errors);
    
    printf("========================================\n");
    printf("Analysis Complete\n");
    printf("========================================\n");
    printf("\nKey Findings:\n");
    printf("1. Euler characteristic validates mesh topology\n");
    printf("2. Mesh quality metrics show expected correlations\n");
    printf("3. Poor mesh quality increases residuals and errors\n");
    printf("4. Grid refinement improves convergence predictably\n");
    printf("\nRecommendations:\n");
    printf("- Focus mesh improvement on high-correlation metrics\n");
    printf("- Use correlation analysis to guide adaptive refinement\n");
    printf("- Validate topology before running simulations\n");
    printf("- Monitor mesh-solution correlations during convergence\n");
    
    return 0;
}
