/**
 * File: rtka_cfd_mesh_analysis.h
 * Copyright (c) 2025 - H.Overman opsec.ee@pm.me
 * Email: opsec.ee@pm.me
 * 
 * RTKA CFD Mesh Quality Analysis
 * 
 * Applies correlation analysis to CFD mesh quality metrics to understand
 * relationships between mesh properties and solution characteristics.
 * 
 * Practical Applications:
 * - Correlate mesh quality with solution accuracy
 * - Identify which mesh metrics most affect convergence
 * - Guide adaptive mesh refinement strategies
 * - Validate mesh generation parameters
 * - Optimize mesh for specific flow phenomena
 * 
 * CHANGELOG:
 * 2025-01-15: Initial implementation
 *   - Euler characteristic validation (V - E + F - C)
 *   - Mesh quality metrics (aspect ratio, skewness, volume)
 *   - Correlation with solution metrics (residuals, errors)
 *   - Multi-mesh comparison analysis
 */

#ifndef RTKA_CFD_MESH_ANALYSIS_H
#define RTKA_CFD_MESH_ANALYSIS_H

#include "rtka_correlation.h"
#include <stddef.h>
#include <stdbool.h>

/* ============================================================================
 * MESH TOPOLOGY STRUCTURES
 * ============================================================================ */

/**
 * Euler characteristic components
 * For 3D mesh: χ = V - E + F - C
 * Should equal number of interior voids + 1
 */
typedef struct {
    size_t vertices;    /* V: Number of nodes */
    size_t edges;       /* E: Number of edges */
    size_t faces;       /* F: Number of faces */
    size_t cells;       /* C: Number of cells/volumes */
    int euler_char;     /* χ = V - E + F - C */
    int expected_char;  /* Expected value (interior voids + 1) */
    bool is_valid;      /* Topology validation result */
} rtka_mesh_topology_t;

/**
 * Mesh quality metrics per cell
 */
typedef struct {
    double aspect_ratio;    /* Max edge / min edge */
    double skewness;        /* Deviation from ideal shape [0,1] */
    double volume;          /* Cell volume */
    double orthogonality;   /* Face normal alignment [0,1] */
    double stretch;         /* Cell elongation measure */
} rtka_cell_quality_t;

/**
 * Solution quality metrics per cell
 */
typedef struct {
    double residual;        /* Local residual magnitude */
    double gradient_error;  /* Gradient reconstruction error */
    double truncation_err;  /* Local truncation error estimate */
    double y_plus;          /* Wall distance metric (if applicable) */
} rtka_solution_quality_t;

/* ============================================================================
 * MESH STATISTICS
 * ============================================================================ */

/**
 * Statistical summary of mesh quality
 */
typedef struct {
    rtka_stats_t aspect_ratio_stats;
    rtka_stats_t skewness_stats;
    rtka_stats_t volume_stats;
    rtka_stats_t orthogonality_stats;
    
    size_t num_cells;
    size_t poor_quality_cells;  /* Cells below quality threshold */
    double quality_threshold;   /* Threshold for poor quality */
} rtka_mesh_stats_t;

/**
 * Correlation analysis results
 */
typedef struct {
    /* Mesh quality correlations */
    double aspect_vs_residual;
    double skewness_vs_residual;
    double volume_vs_error;
    double orthog_vs_convergence;
    
    /* Statistical significance */
    bool aspect_significant;
    bool skewness_significant;
    bool volume_significant;
    bool orthog_significant;
    
    /* Recommendations */
    char primary_issue[128];      /* Main mesh quality issue */
    char recommendation[256];     /* Suggested improvement */
} rtka_cfd_correlation_analysis_t;

/* ============================================================================
 * EULER CHARACTERISTIC VALIDATION
 * ============================================================================ */

/**
 * Calculate Euler characteristic for mesh topology
 * 
 * @param topology Mesh topology structure to populate
 * @return Error code
 * 
 * Formula: χ = V - E + F - C
 * For closed 3D mesh: χ = 2 (sphere topology)
 * For mesh with voids: χ = # of voids + 1
 */
rtka_corr_error_t rtka_mesh_euler_characteristic(
    rtka_mesh_topology_t* topology
);

/**
 * Validate mesh topology using Euler characteristic
 * 
 * @param topology Mesh topology structure
 * @param expected Expected Euler characteristic
 * @return true if valid, false otherwise
 * 
 * This is a fundamental mesh validation check used in CFD preprocessing.
 */
bool rtka_mesh_validate_topology(
    const rtka_mesh_topology_t* topology,
    int expected
);

/* ============================================================================
 * MESH QUALITY ANALYSIS
 * ============================================================================ */

/**
 * Calculate mesh quality statistics
 * 
 * @param quality_data Array of cell quality metrics
 * @param num_cells Number of cells
 * @param quality_threshold Threshold for poor quality (typical: 0.3-0.5)
 * @param out_stats Output statistics structure
 * @return Error code
 */
rtka_corr_error_t rtka_mesh_quality_statistics(
    const rtka_cell_quality_t* quality_data,
    size_t num_cells,
    double quality_threshold,
    rtka_mesh_stats_t* out_stats
);

/* ============================================================================
 * CORRELATION ANALYSIS FOR CFD
 * ============================================================================ */

/**
 * Correlate mesh quality with solution quality
 * 
 * @param mesh_quality Array of mesh quality metrics
 * @param solution_quality Array of solution quality metrics
 * @param num_cells Number of cells (must match for both arrays)
 * @param out_analysis Output correlation analysis
 * @return Error code
 * 
 * Analyzes relationships between:
 * - Aspect ratio vs residual magnitude
 * - Skewness vs convergence rate
 * - Cell volume vs numerical error
 * - Orthogonality vs gradient accuracy
 * 
 * Provides actionable recommendations for mesh improvement.
 */
rtka_corr_error_t rtka_cfd_correlate_mesh_solution(
    const rtka_cell_quality_t* mesh_quality,
    const rtka_solution_quality_t* solution_quality,
    size_t num_cells,
    rtka_cfd_correlation_analysis_t* out_analysis
);

/**
 * Compare multiple meshes using correlation analysis
 * 
 * @param meshes Array of mesh statistics
 * @param convergence_rates Convergence rate for each mesh
 * @param num_meshes Number of meshes to compare
 * @param out_matrix Correlation matrix output
 * @return Error code
 * 
 * Use case: Grid convergence study
 * Correlates mesh refinement with solution improvement
 */
rtka_corr_error_t rtka_cfd_compare_meshes(
    const rtka_mesh_stats_t* meshes,
    const double* convergence_rates,
    size_t num_meshes,
    rtka_corr_matrix_t* out_matrix
);

/* ============================================================================
 * ADAPTIVE MESH REFINEMENT ANALYSIS
 * ============================================================================ */

/**
 * Identify cells for refinement based on correlation analysis
 * 
 * @param mesh_quality Array of mesh quality metrics
 * @param solution_quality Array of solution quality metrics
 * @param num_cells Number of cells
 * @param refinement_fraction Fraction of cells to mark (0.0-1.0)
 * @param out_refine_flags Output array of refinement flags (caller allocated)
 * @return Error code
 * 
 * Uses correlation analysis to prioritize cells for refinement:
 * 1. High residual magnitude
 * 2. Poor mesh quality
 * 3. Large gradient errors
 * 
 * Combines multiple indicators using correlation weights.
 */
rtka_corr_error_t rtka_cfd_identify_refinement_cells(
    const rtka_cell_quality_t* mesh_quality,
    const rtka_solution_quality_t* solution_quality,
    size_t num_cells,
    double refinement_fraction,
    bool* out_refine_flags
);

/* ============================================================================
 * REPORTING AND VISUALIZATION
 * ============================================================================ */

/**
 * Generate mesh quality report
 * 
 * @param topology Mesh topology
 * @param stats Mesh statistics
 * @param analysis Correlation analysis results
 * @param output_file File to write report (or NULL for stdout)
 * @return Error code
 * 
 * Generates human-readable report with:
 * - Euler characteristic validation
 * - Mesh quality statistics
 * - Correlation analysis findings
 * - Recommendations for improvement
 */
rtka_corr_error_t rtka_cfd_generate_report(
    const rtka_mesh_topology_t* topology,
    const rtka_mesh_stats_t* stats,
    const rtka_cfd_correlation_analysis_t* analysis,
    const char* output_file
);

/**
 * Export correlation data for visualization
 * 
 * @param mesh_quality Array of mesh quality metrics
 * @param solution_quality Array of solution quality metrics
 * @param num_cells Number of cells
 * @param output_file CSV file for export
 * @return Error code
 * 
 * Exports data in format suitable for plotting:
 * aspect_ratio,skewness,volume,residual,error
 */
rtka_corr_error_t rtka_cfd_export_correlation_data(
    const rtka_cell_quality_t* mesh_quality,
    const rtka_solution_quality_t* solution_quality,
    size_t num_cells,
    const char* output_file
);

/* ============================================================================
 * UTILITY FUNCTIONS
 * ============================================================================ */

/**
 * Interpret correlation strength in CFD context
 * 
 * @param coefficient Correlation coefficient
 * @param metric_name Name of mesh metric
 * @return Interpretation string with CFD-specific guidance
 */
const char* rtka_cfd_interpret_correlation(
    double coefficient,
    const char* metric_name
);

/**
 * Calculate recommended refinement level based on correlation
 * 
 * @param current_cells Current number of cells
 * @param correlation Correlation between mesh and solution quality
 * @param target_accuracy Desired accuracy improvement
 * @return Recommended number of cells
 */
size_t rtka_cfd_recommend_refinement(
    size_t current_cells,
    double correlation,
    double target_accuracy
);

#endif /* RTKA_CFD_MESH_ANALYSIS_H */
