/**
 * File: rtka_correlation.c
 * Copyright (c) 2025 - H.Overman opsec.ee@pm.me
 * Email: opsec.ee@pm.me
 * 
 * RTKA Correlation Analysis - Implementation
 * 
 * Implementation of Pearson's correlation coefficient with optimized
 * algorithms using standard library functions for mathematical operations.
 * 
 * Optimizations:
 * - Single-pass algorithms where possible
 * - Kahan summation for numerical stability
 * - Early validation to avoid unnecessary computation
 * - Leverages standard library math functions (sqrt, etc.)
 * - Cache-friendly access patterns
 * 
 * CHANGELOG:
 * 2025-01-15: Initial implementation
 *   - All core statistical functions implemented
 *   - Pearson correlation with full result metadata
 *   - Correlation matrix computation
 *   - Numerical stability through Kahan summation
 *   - Comprehensive error handling
 */

#include "rtka_correlation.h"
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <stdbool.h>

/* ============================================================================
 * INTERNAL HELPER FUNCTIONS
 * ============================================================================ */

/**
 * Kahan summation algorithm for numerically stable sum
 * OPT pattern for numerical stability
 */
static inline double kahan_sum(const double* data, size_t count) {
    double sum = 0.0;
    double compensation = 0.0;
    
    for (size_t idx = 0U; idx < count; idx++) {
        double corrected = data[idx] - compensation;
        double new_sum = sum + corrected;
        compensation = (new_sum - sum) - corrected;
        sum = new_sum;
    }
    
    return sum;
}

/**
 * Validate input parameters common to most functions
 */
static inline rtka_corr_error_t validate_inputs(
    const double* data,
    size_t count
) {
    if (data == NULL) {
        return RTKA_CORR_ERROR_NULL_PTR;
    }
    
    if (count < RTKA_CORR_MIN_SAMPLES) {
        return RTKA_CORR_ERROR_INVALID_SIZE;
    }
    
    return RTKA_CORR_SUCCESS;
}

/* ============================================================================
 * CORE STATISTICS FUNCTIONS - IMPLEMENTATION
 * ============================================================================ */

rtka_corr_error_t rtka_corr_mean(
    const double* data,
    size_t count,
    double* out_mean
) {
    rtka_corr_error_t err = validate_inputs(data, count);
    if (err != RTKA_CORR_SUCCESS) {
        return err;
    }
    
    if (out_mean == NULL) {
        return RTKA_CORR_ERROR_NULL_PTR;
    }
    
    /* Use Kahan summation for numerical stability */
    double sum = kahan_sum(data, count);
    *out_mean = sum / (double)count;
    
    return RTKA_CORR_SUCCESS;
}

rtka_corr_error_t rtka_corr_variance(
    const double* data,
    size_t count,
    double mean,
    double* out_variance,
    double* out_std_dev
) {
    rtka_corr_error_t err = validate_inputs(data, count);
    if (err != RTKA_CORR_SUCCESS) {
        return err;
    }
    
    /* Calculate mean if not provided */
    double mean_val = mean;
    if (isnan(mean)) {
        err = rtka_corr_mean(data, count, &mean_val);
        if (err != RTKA_CORR_SUCCESS) {
            return err;
        }
    }
    
    /* Calculate sum of squared deviations using Kahan summation */
    double sum_sq = 0.0;
    double compensation = 0.0;
    
    for (size_t idx = 0U; idx < count; idx++) {
        double deviation = data[idx] - mean_val;
        double sq_dev = deviation * deviation;
        double corrected = sq_dev - compensation;
        double new_sum = sum_sq + corrected;
        compensation = (new_sum - sum_sq) - corrected;
        sum_sq = new_sum;
    }
    
    /* Sample variance uses (n-1) denominator */
    double variance = sum_sq / (double)(count - 1U);
    
    if (out_variance != NULL) {
        *out_variance = variance;
    }
    
    if (out_std_dev != NULL) {
        /* Use standard library sqrt for optimal performance */
        *out_std_dev = sqrt(variance);
    }
    
    return RTKA_CORR_SUCCESS;
}

rtka_corr_error_t rtka_corr_covariance(
    const double* x_data,
    const double* y_data,
    size_t count,
    double x_mean,
    double y_mean,
    double* out_cov
) {
    rtka_corr_error_t err = validate_inputs(x_data, count);
    if (err != RTKA_CORR_SUCCESS) {
        return err;
    }
    
    if (y_data == NULL || out_cov == NULL) {
        return RTKA_CORR_ERROR_NULL_PTR;
    }
    
    /* Calculate means if not provided */
    double x_mean_val = x_mean;
    double y_mean_val = y_mean;
    
    if (isnan(x_mean)) {
        err = rtka_corr_mean(x_data, count, &x_mean_val);
        if (err != RTKA_CORR_SUCCESS) {
            return err;
        }
    }
    
    if (isnan(y_mean)) {
        err = rtka_corr_mean(y_data, count, &y_mean_val);
        if (err != RTKA_CORR_SUCCESS) {
            return err;
        }
    }
    
    /* Calculate sum of products of deviations using Kahan summation */
    double sum_prod = 0.0;
    double compensation = 0.0;
    
    for (size_t idx = 0U; idx < count; idx++) {
        double x_dev = x_data[idx] - x_mean_val;
        double y_dev = y_data[idx] - y_mean_val;
        double product = x_dev * y_dev;
        double corrected = product - compensation;
        double new_sum = sum_prod + corrected;
        compensation = (new_sum - sum_prod) - corrected;
        sum_prod = new_sum;
    }
    
    /* Sample covariance uses (n-1) denominator */
    *out_cov = sum_prod / (double)(count - 1U);
    
    return RTKA_CORR_SUCCESS;
}

rtka_corr_error_t rtka_corr_statistics(
    const double* data,
    size_t count,
    rtka_stats_t* out_stats
) {
    rtka_corr_error_t err = validate_inputs(data, count);
    if (err != RTKA_CORR_SUCCESS) {
        return err;
    }
    
    if (out_stats == NULL) {
        return RTKA_CORR_ERROR_NULL_PTR;
    }
    
    /* Single-pass calculation of mean */
    double mean = 0.0;
    err = rtka_corr_mean(data, count, &mean);
    if (err != RTKA_CORR_SUCCESS) {
        return err;
    }
    
    /* Calculate variance and standard deviation */
    double variance = 0.0;
    double std_dev = 0.0;
    err = rtka_corr_variance(data, count, mean, &variance, &std_dev);
    if (err != RTKA_CORR_SUCCESS) {
        return err;
    }
    
    /* Fill output structure */
    out_stats->mean = mean;
    out_stats->variance = variance;
    out_stats->std_dev = std_dev;
    out_stats->count = count;
    
    return RTKA_CORR_SUCCESS;
}

/* ============================================================================
 * CORRELATION FUNCTIONS - IMPLEMENTATION
 * ============================================================================ */

rtka_corr_error_t rtka_corr_pearson(
    const double* x_data,
    const double* y_data,
    size_t count,
    rtka_corr_result_t* out_result
) {
    rtka_corr_error_t err = validate_inputs(x_data, count);
    if (err != RTKA_CORR_SUCCESS) {
        return err;
    }
    
    if (y_data == NULL || out_result == NULL) {
        return RTKA_CORR_ERROR_NULL_PTR;
    }
    
    /* Initialize result */
    memset(out_result, 0, sizeof(rtka_corr_result_t));
    out_result->sample_count = count;
    
    /* Calculate statistics for both datasets */
    err = rtka_corr_statistics(x_data, count, &out_result->x_stats);
    if (err != RTKA_CORR_SUCCESS) {
        out_result->status = err;
        return err;
    }
    
    err = rtka_corr_statistics(y_data, count, &out_result->y_stats);
    if (err != RTKA_CORR_SUCCESS) {
        out_result->status = err;
        return err;
    }
    
    /* Check for zero variance */
    if (out_result->x_stats.std_dev < RTKA_CORR_EPSILON ||
        out_result->y_stats.std_dev < RTKA_CORR_EPSILON) {
        out_result->status = RTKA_CORR_ERROR_ZERO_VARIANCE;
        return RTKA_CORR_ERROR_ZERO_VARIANCE;
    }
    
    /* Calculate covariance */
    err = rtka_corr_covariance(
        x_data, y_data, count,
        out_result->x_stats.mean,
        out_result->y_stats.mean,
        &out_result->covariance
    );
    if (err != RTKA_CORR_SUCCESS) {
        out_result->status = err;
        return err;
    }
    
    /* Calculate Pearson correlation coefficient:
     * r = Cov(x,y) / (S_x * S_y)
     */
    out_result->coefficient = out_result->covariance /
        (out_result->x_stats.std_dev * out_result->y_stats.std_dev);
    
    /* Clamp to valid range [-1, 1] to handle numerical errors */
    if (out_result->coefficient > 1.0) {
        out_result->coefficient = 1.0;
    } else if (out_result->coefficient < -1.0) {
        out_result->coefficient = -1.0;
    }
    
    out_result->status = RTKA_CORR_SUCCESS;
    return RTKA_CORR_SUCCESS;
}

rtka_corr_error_t rtka_corr_pearson_simple(
    const double* x_data,
    const double* y_data,
    size_t count,
    double* out_coefficient
) {
    if (out_coefficient == NULL) {
        return RTKA_CORR_ERROR_NULL_PTR;
    }
    
    rtka_corr_result_t result = {0};
    rtka_corr_error_t err = rtka_corr_pearson(
        x_data, y_data, count, &result
    );
    
    if (err == RTKA_CORR_SUCCESS) {
        *out_coefficient = result.coefficient;
    }
    
    return err;
}

/* ============================================================================
 * CORRELATION MATRIX FUNCTIONS - IMPLEMENTATION
 * ============================================================================ */

rtka_corr_matrix_t* rtka_corr_matrix_create(
    size_t dimension,
    size_t sample_count
) {
    if (dimension == 0U || sample_count < RTKA_CORR_MIN_SAMPLES) {
        return NULL;
    }
    
    rtka_corr_matrix_t* matrix = calloc(1U, sizeof(rtka_corr_matrix_t));
    if (matrix == NULL) {
        return NULL;
    }
    
    /* Allocate flattened matrix storage */
    size_t matrix_size = dimension * dimension;
    matrix->matrix = calloc(matrix_size, sizeof(double));
    if (matrix->matrix == NULL) {
        free(matrix);
        return NULL;
    }
    
    matrix->dimension = dimension;
    matrix->sample_count = sample_count;
    matrix->status = RTKA_CORR_SUCCESS;
    
    return matrix;
}

void rtka_corr_matrix_free(rtka_corr_matrix_t* matrix) {
    if (matrix != NULL) {
        free(matrix->matrix);
        free(matrix);
    }
}

rtka_corr_error_t rtka_corr_matrix_compute(
    const double* const* data,
    size_t num_variables,
    size_t num_samples,
    rtka_corr_matrix_t* out_matrix
) {
    if (data == NULL || out_matrix == NULL || out_matrix->matrix == NULL) {
        return RTKA_CORR_ERROR_NULL_PTR;
    }
    
    if (num_variables == 0U || num_samples < RTKA_CORR_MIN_SAMPLES) {
        return RTKA_CORR_ERROR_INVALID_SIZE;
    }
    
    if (out_matrix->dimension != num_variables ||
        out_matrix->sample_count != num_samples) {
        return RTKA_CORR_ERROR_INVALID_SIZE;
    }
    
    /* Pre-calculate statistics for all variables to avoid redundant computation */
    rtka_stats_t* stats = calloc(num_variables, sizeof(rtka_stats_t));
    if (stats == NULL) {
        return RTKA_CORR_ERROR_ALLOCATION;
    }
    
    /* Calculate statistics for each variable */
    for (size_t var_idx = 0U; var_idx < num_variables; var_idx++) {
        rtka_corr_error_t err = rtka_corr_statistics(
            data[var_idx],
            num_samples,
            &stats[var_idx]
        );
        
        if (err != RTKA_CORR_SUCCESS) {
            free(stats);
            return err;
        }
    }
    
    /* Compute correlation matrix */
    for (size_t row = 0U; row < num_variables; row++) {
        for (size_t col = 0U; col < num_variables; col++) {
            size_t matrix_idx = row * num_variables + col;
            
            /* Diagonal elements are always 1.0 (perfect self-correlation) */
            if (row == col) {
                out_matrix->matrix[matrix_idx] = 1.0;
                continue;
            }
            
            /* Check for zero variance */
            if (stats[row].std_dev < RTKA_CORR_EPSILON ||
                stats[col].std_dev < RTKA_CORR_EPSILON) {
                out_matrix->matrix[matrix_idx] = 0.0;
                continue;
            }
            
            /* Calculate covariance */
            double cov = 0.0;
            rtka_corr_error_t err = rtka_corr_covariance(
                data[row], data[col], num_samples,
                stats[row].mean, stats[col].mean,
                &cov
            );
            
            if (err != RTKA_CORR_SUCCESS) {
                free(stats);
                return err;
            }
            
            /* Calculate correlation coefficient */
            double corr = cov / (stats[row].std_dev * stats[col].std_dev);
            
            /* Clamp to valid range [-1, 1] */
            if (corr > 1.0) {
                corr = 1.0;
            } else if (corr < -1.0) {
                corr = -1.0;
            }
            
            out_matrix->matrix[matrix_idx] = corr;
        }
    }
    
    free(stats);
    out_matrix->status = RTKA_CORR_SUCCESS;
    return RTKA_CORR_SUCCESS;
}

rtka_corr_error_t rtka_corr_matrix_get(
    const rtka_corr_matrix_t* matrix,
    size_t row,
    size_t col,
    double* out_value
) {
    if (matrix == NULL || matrix->matrix == NULL || out_value == NULL) {
        return RTKA_CORR_ERROR_NULL_PTR;
    }
    
    if (row >= matrix->dimension || col >= matrix->dimension) {
        return RTKA_CORR_ERROR_INVALID_RANGE;
    }
    
    size_t idx = row * matrix->dimension + col;
    *out_value = matrix->matrix[idx];
    
    return RTKA_CORR_SUCCESS;
}

/* ============================================================================
 * UTILITY FUNCTIONS - IMPLEMENTATION
 * ============================================================================ */

const char* rtka_corr_interpret(double coefficient) {
    double abs_coef = fabs(coefficient);
    
    if (abs_coef >= 0.9) {
        return "Very strong";
    } else if (abs_coef >= 0.7) {
        return "Strong";
    } else if (abs_coef >= 0.5) {
        return "Moderate";
    } else if (abs_coef >= 0.3) {
        return "Weak";
    } else {
        return "Very weak or none";
    }
}

bool rtka_corr_is_significant(
    double coefficient,
    size_t sample_count,
    double confidence_level
) {
    /* Simplified significance test using critical values
     * For rigorous testing, use proper hypothesis testing frameworks
     */
    
    if (sample_count < RTKA_CORR_MIN_SAMPLES) {
        return false;
    }
    
    /* Calculate degrees of freedom */
    size_t df = sample_count - 2U;
    
    /* Approximate critical values for common confidence levels
     * These are rough approximations for quick checks
     */
    double critical_value = 0.0;
    
    if (confidence_level >= 0.99) {
        /* Very rough approximation for 99% confidence */
        critical_value = 2.576 / sqrt((double)df);
    } else if (confidence_level >= 0.95) {
        /* Approximation for 95% confidence */
        critical_value = 1.96 / sqrt((double)df);
    } else if (confidence_level >= 0.90) {
        /* Approximation for 90% confidence */
        critical_value = 1.645 / sqrt((double)df);
    } else {
        return false;
    }
    
    return fabs(coefficient) >= critical_value;
}
