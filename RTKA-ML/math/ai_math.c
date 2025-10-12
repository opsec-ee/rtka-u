/**
 * File: ai_math.c
 * 2025 - H.Overman
 * Email: opsec.ee@pm.me
 * 
 * AI Mathematical Operations Library - Implementation
 * 
 * CHANGELOG v1.0.0:
 * - Activation functions implementation
 * - Statistical functions with validation
 * - Distance and similarity metrics
 * - Loss functions with numerical stability
 * - Information theory functions
 * - Optimization helpers
 * - Linear algebra interfaces
 * - Utility functions
 * 
 * IMPLEMENTATION NOTES:
 * - Uses standard math.h functions for exp, log, sqrt
 * - Numerical stability techniques applied (log-sum-exp for softmax)
 * - Early validation for error conditions
 * - All operations validated for NaN and Inf
 */

#include "ai_math.h"
#include <math.h>
#include <stdlib.h>
#include <string.h>
#include <float.h>

/* ============================================================================
 * CONSTANTS
 * ============================================================================ */

#define AIM_EPSILON 1e-10
#define AIM_LOG2 0.693147180559945309417
#define AIM_SQRT_2PI 2.506628274631000502415

/* ============================================================================
 * ACTIVATION FUNCTIONS
 * ============================================================================ */

double aim_sigmoid(double x) {
    /* Numerically stable sigmoid using standard library exp */
    if (x >= 0.0) {
        double exp_neg_x = exp(-x);
        return 1.0 / (1.0 + exp_neg_x);
    } else {
        double exp_x = exp(x);
        return exp_x / (1.0 + exp_x);
    }
}

void aim_sigmoid_batch(const double* AIM_RESTRICT input,
                       double* AIM_RESTRICT output,
                       size_t size) {
    for (size_t i = 0; i < size; i++) {
        output[i] = aim_sigmoid(input[i]);
    }
}

void aim_relu_batch(const double* AIM_RESTRICT input,
                    double* AIM_RESTRICT output,
                    size_t size) {
    for (size_t i = 0; i < size; i++) {
        output[i] = aim_relu(input[i]);
    }
}

aim_error_t aim_softmax(const double* AIM_RESTRICT input,
                        double* AIM_RESTRICT output,
                        size_t size) {
    if (input == NULL || output == NULL) {
        return AIM_ERROR_NULL_PARAM;
    }
    if (size == 0) {
        return AIM_ERROR_INVALID_SIZE;
    }
    if (input == output) {
        return AIM_ERROR_INVALID_SIZE; /* Must use different arrays */
    }
    
    /* Find maximum for numerical stability (log-sum-exp trick) */
    double max_val = input[0];
    for (size_t i = 1; i < size; i++) {
        if (input[i] > max_val) {
            max_val = input[i];
        }
    }
    
    /* Compute exp(x - max) and sum */
    double sum_exp = 0.0;
    for (size_t i = 0; i < size; i++) {
        output[i] = exp(input[i] - max_val);
        sum_exp += output[i];
    }
    
    if (sum_exp < AIM_EPSILON) {
        return AIM_ERROR_DOMAIN;
    }
    
    /* Normalize */
    for (size_t i = 0; i < size; i++) {
        output[i] /= sum_exp;
    }
    
    return AIM_SUCCESS;
}

/* ============================================================================
 * STATISTICAL FUNCTIONS
 * ============================================================================ */

aim_error_t aim_normal_pdf(double x, double mu, double sigma, double* result) {
    if (result == NULL) {
        return AIM_ERROR_NULL_PARAM;
    }
    if (sigma <= 0.0) {
        return AIM_ERROR_DOMAIN;
    }
    
    double diff = x - mu;
    double exponent = -(diff * diff) / (2.0 * sigma * sigma);
    double coefficient = 1.0 / (sigma * AIM_SQRT_2PI);
    
    *result = coefficient * exp(exponent);
    
    if (!aim_is_valid(*result)) {
        return AIM_ERROR_DOMAIN;
    }
    
    return AIM_SUCCESS;
}

aim_error_t aim_z_score(double x, double mu, double sigma, double* result) {
    if (result == NULL) {
        return AIM_ERROR_NULL_PARAM;
    }
    if (fabs(sigma) < AIM_EPSILON) {
        return AIM_ERROR_DIVISION_BY_ZERO;
    }
    
    *result = (x - mu) / sigma;
    
    if (!aim_is_valid(*result)) {
        return AIM_ERROR_DOMAIN;
    }
    
    return AIM_SUCCESS;
}

aim_error_t aim_mean(const double* data, size_t size, double* result) {
    if (data == NULL || result == NULL) {
        return AIM_ERROR_NULL_PARAM;
    }
    if (size == 0) {
        return AIM_ERROR_INVALID_SIZE;
    }
    
    double sum = 0.0;
    for (size_t i = 0; i < size; i++) {
        if (!aim_is_valid(data[i])) {
            return AIM_ERROR_DOMAIN;
        }
        sum += data[i];
    }
    
    *result = sum / (double)size;
    
    return AIM_SUCCESS;
}

aim_error_t aim_std_dev(const double* data, size_t size, double mean, double* result) {
    if (data == NULL || result == NULL) {
        return AIM_ERROR_NULL_PARAM;
    }
    if (size <= 1) {
        return AIM_ERROR_INVALID_SIZE;
    }
    
    double sum_sq_diff = 0.0;
    for (size_t i = 0; i < size; i++) {
        if (!aim_is_valid(data[i])) {
            return AIM_ERROR_DOMAIN;
        }
        double diff = data[i] - mean;
        sum_sq_diff += diff * diff;
    }
    
    double variance = sum_sq_diff / (double)(size - 1);
    *result = sqrt(variance);
    
    if (!aim_is_valid(*result)) {
        return AIM_ERROR_DOMAIN;
    }
    
    return AIM_SUCCESS;
}

aim_error_t aim_correlation(const double* x, const double* y,
                           size_t size, double* result) {
    if (x == NULL || y == NULL || result == NULL) {
        return AIM_ERROR_NULL_PARAM;
    }
    if (size <= 1) {
        return AIM_ERROR_INVALID_SIZE;
    }
    
    /* Calculate means */
    double mean_x = 0.0;
    double mean_y = 0.0;
    aim_error_t err = aim_mean(x, size, &mean_x);
    if (err != AIM_SUCCESS) {
        return err;
    }
    err = aim_mean(y, size, &mean_y);
    if (err != AIM_SUCCESS) {
        return err;
    }
    
    /* Calculate covariance and standard deviations */
    double cov = 0.0;
    double sum_sq_x = 0.0;
    double sum_sq_y = 0.0;
    
    for (size_t i = 0; i < size; i++) {
        double dx = x[i] - mean_x;
        double dy = y[i] - mean_y;
        cov += dx * dy;
        sum_sq_x += dx * dx;
        sum_sq_y += dy * dy;
    }
    
    double std_x = sqrt(sum_sq_x);
    double std_y = sqrt(sum_sq_y);
    
    if (std_x < AIM_EPSILON || std_y < AIM_EPSILON) {
        return AIM_ERROR_DIVISION_BY_ZERO;
    }
    
    *result = cov / (std_x * std_y);
    
    /* Clamp to [-1, 1] due to floating point errors */
    if (*result > 1.0) *result = 1.0;
    if (*result < -1.0) *result = -1.0;
    
    return AIM_SUCCESS;
}

/* ============================================================================
 * DISTANCE AND SIMILARITY METRICS
 * ============================================================================ */

aim_error_t aim_cosine_similarity(const double* a, const double* b,
                                 size_t size, double* result) {
    if (a == NULL || b == NULL || result == NULL) {
        return AIM_ERROR_NULL_PARAM;
    }
    if (size == 0) {
        return AIM_ERROR_INVALID_SIZE;
    }
    
    double dot_product = 0.0;
    double norm_a = 0.0;
    double norm_b = 0.0;
    
    for (size_t i = 0; i < size; i++) {
        if (!aim_is_valid(a[i]) || !aim_is_valid(b[i])) {
            return AIM_ERROR_DOMAIN;
        }
        dot_product += a[i] * b[i];
        norm_a += a[i] * a[i];
        norm_b += b[i] * b[i];
    }
    
    norm_a = sqrt(norm_a);
    norm_b = sqrt(norm_b);
    
    if (norm_a < AIM_EPSILON || norm_b < AIM_EPSILON) {
        return AIM_ERROR_DIVISION_BY_ZERO;
    }
    
    *result = dot_product / (norm_a * norm_b);
    
    /* Clamp to [-1, 1] due to floating point errors */
    if (*result > 1.0) *result = 1.0;
    if (*result < -1.0) *result = -1.0;
    
    return AIM_SUCCESS;
}

aim_error_t aim_euclidean_distance(const double* a, const double* b,
                                  size_t size, double* result) {
    if (a == NULL || b == NULL || result == NULL) {
        return AIM_ERROR_NULL_PARAM;
    }
    if (size == 0) {
        return AIM_ERROR_INVALID_SIZE;
    }
    
    double sum_sq = 0.0;
    for (size_t i = 0; i < size; i++) {
        if (!aim_is_valid(a[i]) || !aim_is_valid(b[i])) {
            return AIM_ERROR_DOMAIN;
        }
        double diff = a[i] - b[i];
        sum_sq += diff * diff;
    }
    
    *result = sqrt(sum_sq);
    
    if (!aim_is_valid(*result)) {
        return AIM_ERROR_DOMAIN;
    }
    
    return AIM_SUCCESS;
}

/* ============================================================================
 * LOSS AND METRIC FUNCTIONS
 * ============================================================================ */

aim_error_t aim_mse(const double* y_true, const double* y_pred,
                   size_t size, double* result) {
    if (y_true == NULL || y_pred == NULL || result == NULL) {
        return AIM_ERROR_NULL_PARAM;
    }
    if (size == 0) {
        return AIM_ERROR_INVALID_SIZE;
    }
    
    double sum_sq_error = 0.0;
    for (size_t i = 0; i < size; i++) {
        if (!aim_is_valid(y_true[i]) || !aim_is_valid(y_pred[i])) {
            return AIM_ERROR_DOMAIN;
        }
        double error = y_true[i] - y_pred[i];
        sum_sq_error += error * error;
    }
    
    *result = sum_sq_error / (double)size;
    
    return AIM_SUCCESS;
}

aim_error_t aim_mse_l2(const double* y_true, const double* y_pred, size_t size,
                      const double* weights, size_t num_weights,
                      double lambda, double* result) {
    if (y_true == NULL || y_pred == NULL || weights == NULL || result == NULL) {
        return AIM_ERROR_NULL_PARAM;
    }
    if (size == 0 || num_weights == 0) {
        return AIM_ERROR_INVALID_SIZE;
    }
    if (lambda < 0.0) {
        return AIM_ERROR_DOMAIN;
    }
    
    /* Calculate MSE */
    double mse_val = 0.0;
    aim_error_t err = aim_mse(y_true, y_pred, size, &mse_val);
    if (err != AIM_SUCCESS) {
        return err;
    }
    
    /* Calculate L2 regularization term */
    double l2_term = 0.0;
    for (size_t i = 0; i < num_weights; i++) {
        if (!aim_is_valid(weights[i])) {
            return AIM_ERROR_DOMAIN;
        }
        l2_term += weights[i] * weights[i];
    }
    
    *result = mse_val + lambda * l2_term;
    
    return AIM_SUCCESS;
}

aim_error_t aim_r2_score(const double* y_true, const double* y_pred,
                        size_t size, double* result) {
    if (y_true == NULL || y_pred == NULL || result == NULL) {
        return AIM_ERROR_NULL_PARAM;
    }
    if (size <= 1) {
        return AIM_ERROR_INVALID_SIZE;
    }
    
    /* Calculate mean of y_true */
    double mean_true = 0.0;
    aim_error_t err = aim_mean(y_true, size, &mean_true);
    if (err != AIM_SUCCESS) {
        return err;
    }
    
    /* Calculate SS_res and SS_tot */
    double ss_res = 0.0;
    double ss_tot = 0.0;
    
    for (size_t i = 0; i < size; i++) {
        if (!aim_is_valid(y_true[i]) || !aim_is_valid(y_pred[i])) {
            return AIM_ERROR_DOMAIN;
        }
        double residual = y_true[i] - y_pred[i];
        double deviation = y_true[i] - mean_true;
        ss_res += residual * residual;
        ss_tot += deviation * deviation;
    }
    
    if (ss_tot < AIM_EPSILON) {
        return AIM_ERROR_DIVISION_BY_ZERO;
    }
    
    *result = 1.0 - (ss_res / ss_tot);
    
    return AIM_SUCCESS;
}

aim_error_t aim_log_loss(const double* y_true, const double* y_pred,
                        size_t size, double* result) {
    if (y_true == NULL || y_pred == NULL || result == NULL) {
        return AIM_ERROR_NULL_PARAM;
    }
    if (size == 0) {
        return AIM_ERROR_INVALID_SIZE;
    }
    
    double sum_loss = 0.0;
    
    for (size_t i = 0; i < size; i++) {
        if (!aim_is_valid(y_true[i]) || !aim_is_valid(y_pred[i])) {
            return AIM_ERROR_DOMAIN;
        }
        
        /* Validate binary labels and probabilities */
        if (y_true[i] < 0.0 || y_true[i] > 1.0 ||
            y_pred[i] < 0.0 || y_pred[i] > 1.0) {
            return AIM_ERROR_DOMAIN;
        }
        
        /* Clip predictions to avoid log(0) */
        double pred_clipped = y_pred[i];
        if (pred_clipped < AIM_EPSILON) {
            pred_clipped = AIM_EPSILON;
        }
        if (pred_clipped > 1.0 - AIM_EPSILON) {
            pred_clipped = 1.0 - AIM_EPSILON;
        }
        
        sum_loss += y_true[i] * log(pred_clipped) +
                   (1.0 - y_true[i]) * log(1.0 - pred_clipped);
    }
    
    *result = -sum_loss / (double)size;
    
    return AIM_SUCCESS;
}

aim_error_t aim_f1_score(double precision, double recall, double* result) {
    if (result == NULL) {
        return AIM_ERROR_NULL_PARAM;
    }
    
    if (precision < 0.0 || precision > 1.0 ||
        recall < 0.0 || recall > 1.0) {
        return AIM_ERROR_DOMAIN;
    }
    
    double sum = precision + recall;
    if (sum < AIM_EPSILON) {
        *result = 0.0;
        return AIM_SUCCESS;
    }
    
    *result = (2.0 * precision * recall) / sum;
    
    return AIM_SUCCESS;
}

/* ============================================================================
 * INFORMATION THEORY
 * ============================================================================ */

aim_error_t aim_entropy(const double* probabilities, size_t size, double* result) {
    if (probabilities == NULL || result == NULL) {
        return AIM_ERROR_NULL_PARAM;
    }
    if (size == 0) {
        return AIM_ERROR_INVALID_SIZE;
    }
    
    double entropy = 0.0;
    double sum_prob = 0.0;
    
    for (size_t i = 0; i < size; i++) {
        if (!aim_is_valid(probabilities[i])) {
            return AIM_ERROR_DOMAIN;
        }
        if (probabilities[i] < 0.0 || probabilities[i] > 1.0) {
            return AIM_ERROR_DOMAIN;
        }
        
        sum_prob += probabilities[i];
        
        if (probabilities[i] > AIM_EPSILON) {
            /* Use log2 for bits */
            entropy -= probabilities[i] * log2(probabilities[i]);
        }
    }
    
    /* Validate probability distribution */
    if (fabs(sum_prob - 1.0) > AIM_EPSILON) {
        return AIM_ERROR_DOMAIN;
    }
    
    *result = entropy;
    
    return AIM_SUCCESS;
}

aim_error_t aim_kl_divergence(const double* p, const double* q,
                             size_t size, double* result) {
    if (p == NULL || q == NULL || result == NULL) {
        return AIM_ERROR_NULL_PARAM;
    }
    if (size == 0) {
        return AIM_ERROR_INVALID_SIZE;
    }
    
    double divergence = 0.0;
    double sum_p = 0.0;
    double sum_q = 0.0;
    
    for (size_t i = 0; i < size; i++) {
        if (!aim_is_valid(p[i]) || !aim_is_valid(q[i])) {
            return AIM_ERROR_DOMAIN;
        }
        if (p[i] < 0.0 || p[i] > 1.0 || q[i] < 0.0 || q[i] > 1.0) {
            return AIM_ERROR_DOMAIN;
        }
        
        sum_p += p[i];
        sum_q += q[i];
        
        if (p[i] > AIM_EPSILON) {
            if (q[i] < AIM_EPSILON) {
                /* KL divergence is infinite if Q(x) = 0 where P(x) > 0 */
                return AIM_ERROR_DOMAIN;
            }
            divergence += p[i] * log(p[i] / q[i]);
        }
    }
    
    /* Validate probability distributions */
    if (fabs(sum_p - 1.0) > AIM_EPSILON || fabs(sum_q - 1.0) > AIM_EPSILON) {
        return AIM_ERROR_DOMAIN;
    }
    
    *result = divergence;
    
    return AIM_SUCCESS;
}

/* ============================================================================
 * OPTIMIZATION
 * ============================================================================ */

aim_error_t aim_gradient_descent_step(double* theta, const double* gradient,
                                     size_t size, double alpha) {
    if (theta == NULL || gradient == NULL) {
        return AIM_ERROR_NULL_PARAM;
    }
    if (size == 0) {
        return AIM_ERROR_INVALID_SIZE;
    }
    if (alpha <= 0.0) {
        return AIM_ERROR_DOMAIN;
    }
    
    for (size_t i = 0; i < size; i++) {
        if (!aim_is_valid(gradient[i])) {
            return AIM_ERROR_DOMAIN;
        }
        theta[i] = theta[i] - alpha * gradient[i];
        
        if (!aim_is_valid(theta[i])) {
            return AIM_ERROR_DOMAIN;
        }
    }
    
    return AIM_SUCCESS;
}

/* ============================================================================
 * LINEAR ALGEBRA OPERATIONS
 * NOTE: These functions require LAPACK/BLAS for actual implementation
 * Providing interface definitions for integration with external libraries
 * ============================================================================ */

aim_error_t aim_ols(const double* X, const double* y,
                   size_t n, size_t p, double* beta) {
    if (X == NULL || y == NULL || beta == NULL) {
        return AIM_ERROR_NULL_PARAM;
    }
    if (n <= p || p == 0) {
        return AIM_ERROR_INVALID_SIZE;
    }
    
    /* 
     * NOTE: Full OLS implementation requires matrix operations:
     * 1. X^T * X (matrix multiplication)
     * 2. (X^T * X)^(-1) (matrix inversion or Cholesky decomposition)
     * 3. (X^T * X)^(-1) * X^T * y (matrix-vector multiplication)
     * 
     * Production implementation should use LAPACK's DGELS (least squares solver)
     * or implement using Cholesky decomposition for numerical stability
     * 
     * This is a stub that indicates the interface.
     * Link against LAPACK and use dgels_ for actual implementation.
     */
    
    return AIM_ERROR_LAPACK_NOT_AVAILABLE;
}

aim_error_t aim_svd(const double* A, size_t m, size_t n,
                   double* U, double* sigma, double* VT) {
    if (A == NULL || U == NULL || sigma == NULL || VT == NULL) {
        return AIM_ERROR_NULL_PARAM;
    }
    if (m == 0 || n == 0) {
        return AIM_ERROR_INVALID_SIZE;
    }
    
    /*
     * NOTE: SVD requires LAPACK's DGESVD routine
     * A = U * Sigma * V^T
     * 
     * Production implementation requires:
     * - LAPACK dgesvd_ function
     * - Proper workspace allocation
     * - Error handling for convergence
     * 
     * This is a stub that indicates the interface.
     */
    
    return AIM_ERROR_LAPACK_NOT_AVAILABLE;
}

aim_error_t aim_eigen_decomposition(const double* A, size_t n,
                                   double* eigenvalues,
                                   double* eigenvectors) {
    if (A == NULL || eigenvalues == NULL || eigenvectors == NULL) {
        return AIM_ERROR_NULL_PARAM;
    }
    if (n == 0) {
        return AIM_ERROR_INVALID_SIZE;
    }
    
    /*
     * NOTE: Eigenvalue decomposition for symmetric matrices requires LAPACK's DSYEV
     * A * v = lambda * v
     * 
     * Production implementation requires:
     * - LAPACK dsyev_ function (or dsyevd_ for divide-and-conquer)
     * - Workspace allocation (LWORK query)
     * - Symmetry validation
     * 
     * This is a stub that indicates the interface.
     */
    
    return AIM_ERROR_LAPACK_NOT_AVAILABLE;
}

/* ============================================================================
 * UTILITY FUNCTIONS
 * ============================================================================ */

const char* aim_error_string(aim_error_t error) {
    switch (error) {
        case AIM_SUCCESS:
            return "Success";
        case AIM_ERROR_NULL_PARAM:
            return "Null parameter provided";
        case AIM_ERROR_INVALID_SIZE:
            return "Invalid size parameter";
        case AIM_ERROR_DIVISION_BY_ZERO:
            return "Division by zero";
        case AIM_ERROR_DOMAIN:
            return "Domain error (invalid input value)";
        case AIM_ERROR_CONVERGENCE:
            return "Algorithm failed to converge";
        case AIM_ERROR_NOMEM:
            return "Memory allocation failed";
        case AIM_ERROR_LAPACK_NOT_AVAILABLE:
            return "LAPACK library not available - link against LAPACK for this function";
        default:
            return "Unknown error";
    }
}
