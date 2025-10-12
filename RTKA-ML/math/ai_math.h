/**
 * File: ai_math.h
 * 2025 - H.Overman
 * Email: opsec.ee@pm.me
 * 
 * AI Mathematical Operations Library
 * Optimized C implementations of 24 essential data science mathematical operations
 * 
 * CHANGELOG v1.0.0:
 * - Initial implementation of core mathematical operations
 * - Activation functions: Sigmoid, ReLU, Softmax
 * - Statistical functions: Normal distribution, Z-score, Correlation
 * - Distance/Similarity: Cosine similarity, Euclidean distance
 * - Loss functions: MSE, Log Loss, R2 Score, F1 Score
 * - Information theory: Entropy, KL Divergence
 * - Optimization: Gradient descent step
 * - Linear algebra interfaces: OLS, SVD, Eigenvalues
 * 
 * DEPENDENCIES:
 * - Standard C library (math.h, stdlib.h, stdbool.h)
 * - Optional: LAPACK/BLAS for advanced linear algebra (SVD, Eigenvalues)
 * 
 * USAGE:
 * Include this header and link against ai_math.c
 * For SVD and Eigenvalue functions, link against LAPACK/BLAS
 */

#ifndef AI_MATH_H
#define AI_MATH_H

#include <stddef.h>
#include <stdbool.h>
#include <stdint.h>

/* ============================================================================
 * COMPILER OPTIMIZATION HINTS
 * ============================================================================ */

#if defined(__GNUC__) || defined(__clang__)
    #define AIM_INLINE static inline __attribute__((always_inline))
    #define AIM_PURE __attribute__((pure))
    #define AIM_CONST __attribute__((const))
    #define AIM_NODISCARD __attribute__((warn_unused_result))
    #define AIM_RESTRICT __restrict__
#else
    #define AIM_INLINE static inline
    #define AIM_PURE
    #define AIM_CONST
    #define AIM_NODISCARD
    #define AIM_RESTRICT
#endif

/* ============================================================================
 * ERROR CODES
 * ============================================================================ */

typedef enum {
    AIM_SUCCESS = 0,
    AIM_ERROR_NULL_PARAM = -1,
    AIM_ERROR_INVALID_SIZE = -2,
    AIM_ERROR_DIVISION_BY_ZERO = -3,
    AIM_ERROR_DOMAIN = -4,
    AIM_ERROR_CONVERGENCE = -5,
    AIM_ERROR_NOMEM = -6,
    AIM_ERROR_LAPACK_NOT_AVAILABLE = -7
} aim_error_t;

/* ============================================================================
 * ACTIVATION FUNCTIONS
 * ============================================================================ */

/**
 * Sigmoid activation: sigma(x) = 1 / (1 + exp(-x))
 * Domain: all real numbers
 * Range: (0, 1)
 * @param x Input value
 * @return Sigmoid of x
 */
AIM_PURE
double aim_sigmoid(double x);

/**
 * Batch sigmoid for array of values
 * @param input Input array
 * @param output Output array (can be same as input for in-place)
 * @param size Number of elements
 */
void aim_sigmoid_batch(const double* AIM_RESTRICT input, 
                       double* AIM_RESTRICT output, 
                       size_t size);

/**
 * ReLU activation: max(0, x)
 * @param x Input value
 * @return ReLU of x
 */
AIM_CONST AIM_INLINE
double aim_relu(double x) {
    return (x > 0.0) ? x : 0.0;
}

/**
 * Batch ReLU for array of values
 * @param input Input array
 * @param output Output array (can be same as input for in-place)
 * @param size Number of elements
 */
void aim_relu_batch(const double* AIM_RESTRICT input,
                    double* AIM_RESTRICT output,
                    size_t size);

/**
 * Softmax activation: exp(x_i) / sum(exp(x_j))
 * Numerically stable implementation using max subtraction
 * @param input Input array
 * @param output Output array (must be different from input)
 * @param size Number of elements
 * @return AIM_SUCCESS or error code
 */
AIM_NODISCARD
aim_error_t aim_softmax(const double* AIM_RESTRICT input,
                        double* AIM_RESTRICT output,
                        size_t size);

/* ============================================================================
 * STATISTICAL FUNCTIONS
 * ============================================================================ */

/**
 * Normal distribution PDF: f(x|mu,sigma^2) = 1/sqrt(2*pi*sigma^2) * exp(-(x-mu)^2/(2*sigma^2))
 * @param x Value to evaluate
 * @param mu Mean
 * @param sigma Standard deviation (must be positive)
 * @param result Pointer to store result
 * @return AIM_SUCCESS or error code
 */
AIM_NODISCARD
aim_error_t aim_normal_pdf(double x, double mu, double sigma, double* result);

/**
 * Z-score: z = (x - mu) / sigma
 * @param x Value
 * @param mu Mean
 * @param sigma Standard deviation (must be non-zero)
 * @param result Pointer to store result
 * @return AIM_SUCCESS or error code
 */
AIM_NODISCARD
aim_error_t aim_z_score(double x, double mu, double sigma, double* result);

/**
 * Calculate mean of array
 * @param data Input array
 * @param size Number of elements (must be > 0)
 * @param result Pointer to store result
 * @return AIM_SUCCESS or error code
 */
AIM_NODISCARD
aim_error_t aim_mean(const double* data, size_t size, double* result);

/**
 * Calculate standard deviation of array
 * @param data Input array
 * @param size Number of elements (must be > 1)
 * @param mean Mean value (pre-computed for efficiency)
 * @param result Pointer to store result
 * @return AIM_SUCCESS or error code
 */
AIM_NODISCARD
aim_error_t aim_std_dev(const double* data, size_t size, double mean, double* result);

/**
 * Pearson correlation coefficient: Cov(X,Y) / (Std(X) * Std(Y))
 * @param x First array
 * @param y Second array
 * @param size Number of elements (must be > 1)
 * @param result Pointer to store result (range: [-1, 1])
 * @return AIM_SUCCESS or error code
 */
AIM_NODISCARD
aim_error_t aim_correlation(const double* x, const double* y, 
                           size_t size, double* result);

/* ============================================================================
 * DISTANCE AND SIMILARITY METRICS
 * ============================================================================ */

/**
 * Cosine similarity: (A · B) / (||A|| * ||B||)
 * @param a First vector
 * @param b Second vector
 * @param size Dimension of vectors (must be > 0)
 * @param result Pointer to store result (range: [-1, 1])
 * @return AIM_SUCCESS or error code
 */
AIM_NODISCARD
aim_error_t aim_cosine_similarity(const double* a, const double* b,
                                 size_t size, double* result);

/**
 * Euclidean distance: sqrt(sum((a_i - b_i)^2))
 * @param a First vector
 * @param b Second vector
 * @param size Dimension of vectors (must be > 0)
 * @param result Pointer to store result
 * @return AIM_SUCCESS or error code
 */
AIM_NODISCARD
aim_error_t aim_euclidean_distance(const double* a, const double* b,
                                  size_t size, double* result);

/* ============================================================================
 * LOSS AND METRIC FUNCTIONS
 * ============================================================================ */

/**
 * Mean Squared Error: MSE = (1/n) * sum((y_i - y_hat_i)^2)
 * @param y_true True values
 * @param y_pred Predicted values
 * @param size Number of samples (must be > 0)
 * @param result Pointer to store result
 * @return AIM_SUCCESS or error code
 */
AIM_NODISCARD
aim_error_t aim_mse(const double* y_true, const double* y_pred,
                   size_t size, double* result);

/**
 * MSE with L2 Regularization: MSE + lambda * sum(beta_j^2)
 * @param y_true True values
 * @param y_pred Predicted values
 * @param size Number of samples (must be > 0)
 * @param weights Model weights
 * @param num_weights Number of weights
 * @param lambda Regularization parameter (must be >= 0)
 * @param result Pointer to store result
 * @return AIM_SUCCESS or error code
 */
AIM_NODISCARD
aim_error_t aim_mse_l2(const double* y_true, const double* y_pred, size_t size,
                      const double* weights, size_t num_weights,
                      double lambda, double* result);

/**
 * R-squared score: R^2 = 1 - (SS_res / SS_tot)
 * @param y_true True values
 * @param y_pred Predicted values
 * @param size Number of samples (must be > 1)
 * @param result Pointer to store result
 * @return AIM_SUCCESS or error code
 */
AIM_NODISCARD
aim_error_t aim_r2_score(const double* y_true, const double* y_pred,
                        size_t size, double* result);

/**
 * Binary cross-entropy (Log Loss): -(1/N) * sum(y*log(p) + (1-y)*log(1-p))
 * @param y_true True binary labels (0 or 1)
 * @param y_pred Predicted probabilities (0 to 1)
 * @param size Number of samples (must be > 0)
 * @param result Pointer to store result
 * @return AIM_SUCCESS or error code
 */
AIM_NODISCARD
aim_error_t aim_log_loss(const double* y_true, const double* y_pred,
                        size_t size, double* result);

/**
 * F1 Score: 2PR / (P + R) where P=precision, R=recall
 * @param precision Precision value (0 to 1)
 * @param recall Recall value (0 to 1)
 * @param result Pointer to store result
 * @return AIM_SUCCESS or error code
 */
AIM_NODISCARD
aim_error_t aim_f1_score(double precision, double recall, double* result);

/* ============================================================================
 * INFORMATION THEORY
 * ============================================================================ */

/**
 * Shannon Entropy: H = -sum(p_i * log2(p_i))
 * @param probabilities Probability distribution (must sum to 1.0)
 * @param size Number of elements (must be > 0)
 * @param result Pointer to store result
 * @return AIM_SUCCESS or error code
 */
AIM_NODISCARD
aim_error_t aim_entropy(const double* probabilities, size_t size, double* result);

/**
 * Kullback-Leibler Divergence: D_KL(P||Q) = sum(P(x) * log(P(x)/Q(x)))
 * @param p First probability distribution
 * @param q Second probability distribution
 * @param size Number of elements (must be > 0)
 * @param result Pointer to store result
 * @return AIM_SUCCESS or error code
 */
AIM_NODISCARD
aim_error_t aim_kl_divergence(const double* p, const double* q,
                             size_t size, double* result);

/* ============================================================================
 * OPTIMIZATION
 * ============================================================================ */

/**
 * Gradient descent update step: theta_new = theta_old - alpha * gradient
 * @param theta Parameters to update (modified in place)
 * @param gradient Gradient values
 * @param size Number of parameters (must be > 0)
 * @param alpha Learning rate (must be > 0)
 * @return AIM_SUCCESS or error code
 */
AIM_NODISCARD
aim_error_t aim_gradient_descent_step(double* theta, const double* gradient,
                                     size_t size, double alpha);

/* ============================================================================
 * LINEAR ALGEBRA OPERATIONS
 * These functions provide interfaces to standard operations
 * ============================================================================ */

/**
 * Ordinary Least Squares: beta = (X^T * X)^(-1) * X^T * y
 * Solves linear regression using normal equations
 * @param X Design matrix (n x p, row-major)
 * @param y Response vector (n x 1)
 * @param n Number of samples (must be > p)
 * @param p Number of features (must be > 0)
 * @param beta Output coefficients (p x 1)
 * @return AIM_SUCCESS or error code
 * NOTE: Requires LAPACK for matrix inversion
 */
AIM_NODISCARD
aim_error_t aim_ols(const double* X, const double* y,
                   size_t n, size_t p, double* beta);

/**
 * Singular Value Decomposition: A = U * Sigma * V^T
 * @param A Input matrix (m x n, row-major)
 * @param m Number of rows
 * @param n Number of columns
 * @param U Left singular vectors (m x m)
 * @param sigma Singular values (min(m,n))
 * @param VT Right singular vectors transposed (n x n)
 * @return AIM_SUCCESS or error code
 * NOTE: Requires LAPACK (dgesvd)
 */
AIM_NODISCARD
aim_error_t aim_svd(const double* A, size_t m, size_t n,
                   double* U, double* sigma, double* VT);

/**
 * Eigenvalue decomposition: A*v = lambda*v
 * @param A Symmetric matrix (n x n, row-major)
 * @param n Matrix dimension
 * @param eigenvalues Output eigenvalues (n)
 * @param eigenvectors Output eigenvectors (n x n, column-major)
 * @return AIM_SUCCESS or error code
 * NOTE: Requires LAPACK (dsyev)
 */
AIM_NODISCARD
aim_error_t aim_eigen_decomposition(const double* A, size_t n,
                                   double* eigenvalues,
                                   double* eigenvectors);

/* ============================================================================
 * UTILITY FUNCTIONS
 * ============================================================================ */

/**
 * Get error message string
 * @param error Error code
 * @return Human-readable error message
 */
const char* aim_error_string(aim_error_t error);

/**
 * Check if value is valid (not NaN or Inf)
 * @param x Value to check
 * @return true if valid, false otherwise
 */
AIM_INLINE
bool aim_is_valid(double x) {
    return !__builtin_isnan(x) && !__builtin_isinf(x);
}

#endif /* AI_MATH_H */
