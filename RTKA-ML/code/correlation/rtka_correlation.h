/**
 * File: rtka_correlation.h
 * Copyright (c) 2025 - H.Overman opsec.ee@pm.me
 * Email: opsec.ee@pm.me
 * 
 * RTKA Correlation Analysis - Pearson's Method
 * 
 * Implements mathematically rigorous correlation coefficient calculations
 * using Pearson's method with optimized algorithms for single pairs and
 * full correlation matrices.
 * 
 * Mathematical Foundation:
 *   r_xy = Cov(x,y) / (S_x * S_y)
 *   r_xy = Σ(x_i - x̄)(y_i - ȳ) / √[Σ(x_i - x̄)² * Σ(y_i - ȳ)²]
 * 
 * where:
 *   - r_xy is correlation coefficient between x and y
 *   - Cov(x,y) is covariance of x and y
 *   - S_x, S_y are standard deviations of x and y
 *   - x̄, ȳ are means of x and y
 *   - Result range: [-1, 1]
 * 
 * CHANGELOG:
 * 2025-01-15: Initial implementation
 *   - Pearson correlation coefficient (single pair)
 *   - Covariance calculation
 *   - Standard deviation calculation
 *   - Mean calculation
 *   - Full correlation matrix computation
 *   - Optimized single-pass algorithms where possible
 *   - Error handling for edge cases
 */

#ifndef RTKA_CORRELATION_H
#define RTKA_CORRELATION_H

#include <stddef.h>
#include <stdbool.h>
#include <math.h>

/* ============================================================================
 * CONFIGURATION
 * ============================================================================ */

/* Minimum sample size for valid correlation */
#define RTKA_CORR_MIN_SAMPLES 2U

/* Epsilon for numerical stability checks */
#define RTKA_CORR_EPSILON 1e-10

/* Error codes */
typedef enum {
    RTKA_CORR_SUCCESS = 0,
    RTKA_CORR_ERROR_NULL_PTR = -1,
    RTKA_CORR_ERROR_INVALID_SIZE = -2,
    RTKA_CORR_ERROR_ZERO_VARIANCE = -3,
    RTKA_CORR_ERROR_ALLOCATION = -4,
    RTKA_CORR_ERROR_INVALID_RANGE = -5
} rtka_corr_error_t;

/* ============================================================================
 * DATA STRUCTURES
 * ============================================================================ */

/**
 * Statistical summary for a dataset
 */
typedef struct {
    double mean;              /* Arithmetic mean */
    double variance;          /* Sample variance */
    double std_dev;           /* Standard deviation */
    size_t count;             /* Number of samples */
} rtka_stats_t;

/**
 * Correlation result with metadata
 */
typedef struct {
    double coefficient;       /* Pearson correlation coefficient [-1, 1] */
    double covariance;        /* Covariance of x and y */
    rtka_stats_t x_stats;     /* Statistics for x dataset */
    rtka_stats_t y_stats;     /* Statistics for y dataset */
    size_t sample_count;      /* Number of samples used */
    rtka_corr_error_t status; /* Computation status */
} rtka_corr_result_t;

/**
 * Correlation matrix result
 */
typedef struct {
    double* matrix;           /* Flattened correlation matrix (n x n) */
    size_t dimension;         /* Matrix dimension (number of variables) */
    size_t sample_count;      /* Number of samples per variable */
    rtka_corr_error_t status; /* Computation status */
} rtka_corr_matrix_t;

/* ============================================================================
 * CORE STATISTICS FUNCTIONS
 * ============================================================================ */

/**
 * Calculate mean of a dataset
 * 
 * @param data Pointer to data array
 * @param count Number of elements
 * @param out_mean Pointer to store calculated mean
 * @return Error code
 * 
 * Complexity: O(n)
 * Uses: Kahan summation for numerical stability
 */
rtka_corr_error_t rtka_corr_mean(
    const double* data,
    size_t count,
    double* out_mean
);

/**
 * Calculate variance and standard deviation
 * 
 * @param data Pointer to data array
 * @param count Number of elements
 * @param mean Pre-calculated mean (or NAN to calculate)
 * @param out_variance Pointer to store sample variance (can be NULL)
 * @param out_std_dev Pointer to store standard deviation (can be NULL)
 * @return Error code
 * 
 * Complexity: O(n)
 * Note: Uses sample variance (n-1 denominator)
 */
rtka_corr_error_t rtka_corr_variance(
    const double* data,
    size_t count,
    double mean,
    double* out_variance,
    double* out_std_dev
);

/**
 * Calculate covariance between two datasets
 * 
 * @param x_data First dataset
 * @param y_data Second dataset
 * @param count Number of paired samples
 * @param x_mean Pre-calculated mean of x (or NAN to calculate)
 * @param y_mean Pre-calculated mean of y (or NAN to calculate)
 * @param out_cov Pointer to store covariance
 * @return Error code
 * 
 * Complexity: O(n)
 * Note: Uses sample covariance (n-1 denominator)
 */
rtka_corr_error_t rtka_corr_covariance(
    const double* x_data,
    const double* y_data,
    size_t count,
    double x_mean,
    double y_mean,
    double* out_cov
);

/**
 * Calculate complete statistics for a dataset
 * 
 * @param data Pointer to data array
 * @param count Number of elements
 * @param out_stats Pointer to store statistics
 * @return Error code
 * 
 * Complexity: O(n)
 * More efficient than calling individual functions
 */
rtka_corr_error_t rtka_corr_statistics(
    const double* data,
    size_t count,
    rtka_stats_t* out_stats
);

/* ============================================================================
 * CORRELATION FUNCTIONS
 * ============================================================================ */

/**
 * Calculate Pearson correlation coefficient
 * 
 * @param x_data First dataset
 * @param y_data Second dataset
 * @param count Number of paired samples
 * @param out_result Pointer to store complete correlation result
 * @return Error code
 * 
 * Complexity: O(n)
 * 
 * Formula: r_xy = Σ(x_i - x̄)(y_i - ȳ) / √[Σ(x_i - x̄)² * Σ(y_i - ȳ)²]
 * 
 * Returns:
 *   - RTKA_CORR_SUCCESS on success
 *   - RTKA_CORR_ERROR_NULL_PTR if any pointer is NULL
 *   - RTKA_CORR_ERROR_INVALID_SIZE if count < RTKA_CORR_MIN_SAMPLES
 *   - RTKA_CORR_ERROR_ZERO_VARIANCE if either dataset has zero variance
 */
rtka_corr_error_t rtka_corr_pearson(
    const double* x_data,
    const double* y_data,
    size_t count,
    rtka_corr_result_t* out_result
);

/**
 * Calculate Pearson correlation coefficient (simplified interface)
 * 
 * @param x_data First dataset
 * @param y_data Second dataset
 * @param count Number of paired samples
 * @param out_coefficient Pointer to store correlation coefficient
 * @return Error code
 * 
 * Complexity: O(n)
 * Simplified version that only returns the coefficient
 */
rtka_corr_error_t rtka_corr_pearson_simple(
    const double* x_data,
    const double* y_data,
    size_t count,
    double* out_coefficient
);

/* ============================================================================
 * CORRELATION MATRIX FUNCTIONS
 * ============================================================================ */

/**
 * Create correlation matrix structure
 * 
 * @param dimension Number of variables
 * @param sample_count Number of samples per variable
 * @return Allocated correlation matrix or NULL on error
 * 
 * Note: Caller must free with rtka_corr_matrix_free()
 */
rtka_corr_matrix_t* rtka_corr_matrix_create(
    size_t dimension,
    size_t sample_count
);

/**
 * Free correlation matrix structure
 * 
 * @param matrix Pointer to matrix to free
 */
void rtka_corr_matrix_free(rtka_corr_matrix_t* matrix);

/**
 * Calculate full correlation matrix
 * 
 * @param data 2D array of variables (row-major: [variable][sample])
 * @param num_variables Number of variables (rows)
 * @param num_samples Number of samples per variable (columns)
 * @param out_matrix Pre-allocated matrix structure
 * @return Error code
 * 
 * Complexity: O(n²m) where n = num_variables, m = num_samples
 * 
 * Matrix format: Symmetric matrix where entry (i,j) contains
 * correlation between variable i and variable j.
 * Diagonal entries are always 1.0 (perfect self-correlation)
 * 
 * Note: data must be organized as data[variable_index][sample_index]
 */
rtka_corr_error_t rtka_corr_matrix_compute(
    const double* const* data,
    size_t num_variables,
    size_t num_samples,
    rtka_corr_matrix_t* out_matrix
);

/**
 * Get correlation value from matrix
 * 
 * @param matrix Correlation matrix
 * @param row Row index (0-based)
 * @param col Column index (0-based)
 * @param out_value Pointer to store correlation value
 * @return Error code
 * 
 * Complexity: O(1)
 */
rtka_corr_error_t rtka_corr_matrix_get(
    const rtka_corr_matrix_t* matrix,
    size_t row,
    size_t col,
    double* out_value
);

/* ============================================================================
 * UTILITY FUNCTIONS
 * ============================================================================ */

/**
 * Interpret correlation coefficient strength
 * 
 * @param coefficient Correlation coefficient value
 * @return String description of correlation strength
 * 
 * Classification:
 *   |r| >= 0.9: Very strong
 *   |r| >= 0.7: Strong
 *   |r| >= 0.5: Moderate
 *   |r| >= 0.3: Weak
 *   |r| <  0.3: Very weak or none
 */
const char* rtka_corr_interpret(double coefficient);

/**
 * Check if correlation is statistically significant
 * (Simplified check based on sample size)
 * 
 * @param coefficient Correlation coefficient
 * @param sample_count Number of samples
 * @param confidence_level Confidence level (0.90, 0.95, 0.99)
 * @return true if significant, false otherwise
 * 
 * Note: This is a simplified significance test.
 * For rigorous statistical testing, use proper hypothesis testing.
 */
bool rtka_corr_is_significant(
    double coefficient,
    size_t sample_count,
    double confidence_level
);

#endif /* RTKA_CORRELATION_H */
