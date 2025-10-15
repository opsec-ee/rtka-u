/**
 * File: rtka_correlation_test.c
 * Copyright (c) 2025 - H.Overman opsec.ee@pm.me
 * Email: opsec.ee@pm.me
 * 
 * RTKA Correlation Analysis - Test Suite
 * 
 * Comprehensive test suite for correlation functions with test data
 * generation, validation, and output reporting.
 * 
 * This file is SEPARATE from the algorithm implementation and contains
 * all test-specific code including:
 * - Test data generation
 * - Output formatting (printf)
 * - Test validation
 * - Performance benchmarking
 * 
 * CHANGELOG:
 * 2025-01-15: Initial test implementation
 *   - Basic functionality tests
 *   - Edge case tests
 *   - Known correlation tests (positive, negative, none)
 *   - Correlation matrix tests
 *   - Error handling tests
 *   - Performance benchmarks
 */

#include "rtka_correlation.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <time.h>
#include <assert.h>

/* ============================================================================
 * TEST CONFIGURATION
 * ============================================================================ */

#define TEST_EPSILON 1e-6
#define TEST_DATASET_SIZE 100U
#define TEST_LARGE_DATASET_SIZE 10000U

/* Test result tracking */
typedef struct {
    size_t passed;
    size_t failed;
    size_t total;
} test_results_t;

static test_results_t g_test_results = {0};

/* ============================================================================
 * TEST UTILITIES
 * ============================================================================ */

/**
 * Check if two doubles are approximately equal
 */
static bool approx_equal(double val1, double val2, double epsilon) {
    return fabs(val1 - val2) < epsilon;
}

/**
 * Report test result
 */
static void report_test(const char* test_name, bool passed) {
    g_test_results.total++;
    if (passed) {
        g_test_results.passed++;
        printf("[PASS] %s\n", test_name);
    } else {
        g_test_results.failed++;
        printf("[FAIL] %s\n", test_name);
    }
}

/**
 * Print test summary
 */
static void print_test_summary(void) {
    printf("\n");
    printf("========================================\n");
    printf("Test Summary\n");
    printf("========================================\n");
    printf("Total:  %zu\n", g_test_results.total);
    printf("Passed: %zu\n", g_test_results.passed);
    printf("Failed: %zu\n", g_test_results.failed);
    printf("Success Rate: %.1f%%\n",
           100.0 * (double)g_test_results.passed / (double)g_test_results.total);
    printf("========================================\n");
}

/* ============================================================================
 * TEST DATA GENERATION
 * ============================================================================ */

/**
 * Generate linearly correlated data: y = a*x + b + noise
 */
static void generate_linear_data(
    double* x_data,
    double* y_data,
    size_t count,
    double slope,
    double intercept,
    double noise_level
) {
    for (size_t idx = 0U; idx < count; idx++) {
        x_data[idx] = (double)idx;
        
        /* Generate y with linear relationship plus noise */
        double noise = ((double)rand() / (double)RAND_MAX - 0.5) * 2.0 * noise_level;
        y_data[idx] = slope * x_data[idx] + intercept + noise;
    }
}

/**
 * Generate uncorrelated random data
 */
static void generate_random_data(
    double* x_data,
    double* y_data,
    size_t count
) {
    for (size_t idx = 0U; idx < count; idx++) {
        x_data[idx] = (double)rand() / (double)RAND_MAX;
        y_data[idx] = (double)rand() / (double)RAND_MAX;
    }
}

/**
 * Generate constant data (zero variance)
 */
static void generate_constant_data(
    double* data,
    size_t count,
    double value
) {
    for (size_t idx = 0U; idx < count; idx++) {
        data[idx] = value;
    }
}

/* ============================================================================
 * BASIC FUNCTIONALITY TESTS
 * ============================================================================ */

static void test_mean_calculation(void) {
    double data[] = {1.0, 2.0, 3.0, 4.0, 5.0};
    size_t count = sizeof(data) / sizeof(data[0]);
    double mean = 0.0;
    
    rtka_corr_error_t err = rtka_corr_mean(data, count, &mean);
    
    bool passed = (err == RTKA_CORR_SUCCESS) &&
                  approx_equal(mean, 3.0, TEST_EPSILON);
    
    report_test("Mean Calculation", passed);
}

static void test_variance_calculation(void) {
    double data[] = {2.0, 4.0, 4.0, 4.0, 5.0, 5.0, 7.0, 9.0};
    size_t count = sizeof(data) / sizeof(data[0]);
    double mean = 0.0;
    double variance = 0.0;
    double std_dev = 0.0;
    
    /* First calculate mean */
    rtka_corr_error_t err = rtka_corr_mean(data, count, &mean);
    if (err != RTKA_CORR_SUCCESS) {
        report_test("Variance Calculation", false);
        return;
    }
    
    /* Then calculate variance with known mean */
    err = rtka_corr_variance(data, count, mean, &variance, &std_dev);
    
    /* Debug: Print actual values */
    if (err == RTKA_CORR_SUCCESS) {
        printf("  Mean: %.6f, Variance: %.6f, Std Dev: %.6f\n", 
               mean, variance, std_dev);
    }
    
    /* Mean = 5.0, Sum of squared deviations = 32, Sample variance = 32/7 ≈ 4.571 */
    bool passed = (err == RTKA_CORR_SUCCESS) &&
                  approx_equal(mean, 5.0, TEST_EPSILON) &&
                  approx_equal(variance, 4.571429, 0.001) &&
                  approx_equal(std_dev, 2.138090, 0.001);
    
    report_test("Variance Calculation", passed);
}

static void test_perfect_positive_correlation(void) {
    const size_t count = 10U;
    double* x_data = malloc(count * sizeof(double));
    double* y_data = malloc(count * sizeof(double));
    
    /* Generate perfect positive correlation: y = 2*x + 1 */
    generate_linear_data(x_data, y_data, count, 2.0, 1.0, 0.0);
    
    rtka_corr_result_t result = {0};
    rtka_corr_error_t err = rtka_corr_pearson(x_data, y_data, count, &result);
    
    bool passed = (err == RTKA_CORR_SUCCESS) &&
                  approx_equal(result.coefficient, 1.0, TEST_EPSILON);
    
    if (passed) {
        printf("  Coefficient: %.6f (expected: 1.000000)\n", result.coefficient);
    }
    
    report_test("Perfect Positive Correlation", passed);
    
    free(x_data);
    free(y_data);
}

static void test_perfect_negative_correlation(void) {
    const size_t count = 10U;
    double* x_data = malloc(count * sizeof(double));
    double* y_data = malloc(count * sizeof(double));
    
    /* Generate perfect negative correlation: y = -2*x + 10 */
    generate_linear_data(x_data, y_data, count, -2.0, 10.0, 0.0);
    
    rtka_corr_result_t result = {0};
    rtka_corr_error_t err = rtka_corr_pearson(x_data, y_data, count, &result);
    
    bool passed = (err == RTKA_CORR_SUCCESS) &&
                  approx_equal(result.coefficient, -1.0, TEST_EPSILON);
    
    if (passed) {
        printf("  Coefficient: %.6f (expected: -1.000000)\n", result.coefficient);
    }
    
    report_test("Perfect Negative Correlation", passed);
    
    free(x_data);
    free(y_data);
}

static void test_no_correlation(void) {
    const size_t count = 100U;
    double* x_data = malloc(count * sizeof(double));
    double* y_data = malloc(count * sizeof(double));
    
    /* Generate random uncorrelated data */
    srand(12345U);  /* Fixed seed for reproducibility */
    generate_random_data(x_data, y_data, count);
    
    rtka_corr_result_t result = {0};
    rtka_corr_error_t err = rtka_corr_pearson(x_data, y_data, count, &result);
    
    /* With random data, correlation should be close to 0 */
    bool passed = (err == RTKA_CORR_SUCCESS) &&
                  fabs(result.coefficient) < 0.3;
    
    if (passed) {
        printf("  Coefficient: %.6f (expected: ~0.0)\n", result.coefficient);
    }
    
    report_test("No Correlation (Random Data)", passed);
    
    free(x_data);
    free(y_data);
}

/* ============================================================================
 * EDGE CASE TESTS
 * ============================================================================ */

static void test_zero_variance_error(void) {
    const size_t count = 10U;
    double* x_data = malloc(count * sizeof(double));
    double* y_data = malloc(count * sizeof(double));
    
    /* Generate constant data (zero variance) */
    generate_constant_data(x_data, count, 5.0);
    generate_linear_data(y_data, y_data, count, 1.0, 0.0, 0.0);
    
    rtka_corr_result_t result = {0};
    rtka_corr_error_t err = rtka_corr_pearson(x_data, y_data, count, &result);
    
    bool passed = (err == RTKA_CORR_ERROR_ZERO_VARIANCE);
    
    report_test("Zero Variance Error Handling", passed);
    
    free(x_data);
    free(y_data);
}

static void test_insufficient_samples_error(void) {
    double x_data[] = {1.0};
    double y_data[] = {2.0};
    
    double coefficient = 0.0;
    rtka_corr_error_t err = rtka_corr_pearson_simple(
        x_data, y_data, 1U, &coefficient
    );
    
    bool passed = (err == RTKA_CORR_ERROR_INVALID_SIZE);
    
    report_test("Insufficient Samples Error", passed);
}

static void test_null_pointer_error(void) {
    double data[] = {1.0, 2.0, 3.0};
    double coefficient = 0.0;
    
    rtka_corr_error_t err = rtka_corr_pearson_simple(
        NULL, data, 3U, &coefficient
    );
    
    bool passed = (err == RTKA_CORR_ERROR_NULL_PTR);
    
    report_test("NULL Pointer Error Handling", passed);
}

/* ============================================================================
 * CORRELATION MATRIX TESTS
 * ============================================================================ */

static void test_correlation_matrix(void) {
    const size_t num_vars = 3U;
    const size_t num_samples = 50U;
    
    /* Allocate data for 3 variables */
    double** data = malloc(num_vars * sizeof(double*));
    for (size_t idx = 0U; idx < num_vars; idx++) {
        data[idx] = malloc(num_samples * sizeof(double));
    }
    
    /* Generate test data:
     * Var 0: Independent
     * Var 1: Correlated with Var 0
     * Var 2: Independent
     */
    srand(54321U);
    for (size_t samp = 0U; samp < num_samples; samp++) {
        data[0][samp] = (double)samp;
        data[1][samp] = 2.0 * (double)samp + 5.0;  /* Correlated with var 0 */
        data[2][samp] = (double)rand() / (double)RAND_MAX * 100.0;
    }
    
    /* Create and compute correlation matrix */
    rtka_corr_matrix_t* matrix = rtka_corr_matrix_create(num_vars, num_samples);
    rtka_corr_error_t err = rtka_corr_matrix_compute(
        (const double* const*)data, num_vars, num_samples, matrix
    );
    
    /* Check diagonal elements (should be 1.0) */
    double diag_val = 0.0;
    bool diagonal_ok = true;
    for (size_t idx = 0U; idx < num_vars && diagonal_ok; idx++) {
        rtka_corr_matrix_get(matrix, idx, idx, &diag_val);
        diagonal_ok = approx_equal(diag_val, 1.0, TEST_EPSILON);
    }
    
    /* Check correlation between var 0 and var 1 (should be ~1.0) */
    double corr_01 = 0.0;
    rtka_corr_matrix_get(matrix, 0U, 1U, &corr_01);
    bool vars_correlated = approx_equal(corr_01, 1.0, 0.01);
    
    bool passed = (err == RTKA_CORR_SUCCESS) &&
                  diagonal_ok &&
                  vars_correlated;
    
    if (passed) {
        printf("  Matrix computed successfully\n");
        printf("  Diagonal elements: 1.0 (verified)\n");
        printf("  Corr(Var0, Var1): %.6f (expected: ~1.0)\n", corr_01);
    }
    
    report_test("Correlation Matrix Computation", passed);
    
    /* Cleanup */
    rtka_corr_matrix_free(matrix);
    for (size_t idx = 0U; idx < num_vars; idx++) {
        free(data[idx]);
    }
    free(data);
}

/* ============================================================================
 * UTILITY FUNCTION TESTS
 * ============================================================================ */

static void test_interpretation(void) {
    const char* result = NULL;
    bool passed = true;
    
    result = rtka_corr_interpret(0.95);
    passed = passed && (strcmp(result, "Very strong") == 0);
    
    result = rtka_corr_interpret(0.75);
    passed = passed && (strcmp(result, "Strong") == 0);
    
    result = rtka_corr_interpret(0.55);
    passed = passed && (strcmp(result, "Moderate") == 0);
    
    result = rtka_corr_interpret(0.35);
    passed = passed && (strcmp(result, "Weak") == 0);
    
    result = rtka_corr_interpret(0.15);
    passed = passed && (strcmp(result, "Very weak or none") == 0);
    
    report_test("Correlation Interpretation", passed);
}

/* ============================================================================
 * PERFORMANCE TESTS
 * ============================================================================ */

static void test_performance_large_dataset(void) {
    const size_t count = TEST_LARGE_DATASET_SIZE;
    double* x_data = malloc(count * sizeof(double));
    double* y_data = malloc(count * sizeof(double));
    
    /* Generate data */
    srand(99999U);
    generate_linear_data(x_data, y_data, count, 1.5, 3.0, 0.1);
    
    /* Time the correlation calculation */
    clock_t start = clock();
    
    rtka_corr_result_t result = {0};
    rtka_corr_error_t err = rtka_corr_pearson(x_data, y_data, count, &result);
    
    clock_t end = clock();
    double elapsed = (double)(end - start) / CLOCKS_PER_SEC * 1000.0;
    
    bool passed = (err == RTKA_CORR_SUCCESS);
    
    printf("  Dataset size: %zu samples\n", count);
    printf("  Time: %.3f ms\n", elapsed);
    printf("  Throughput: %.0f samples/ms\n", (double)count / elapsed);
    
    report_test("Large Dataset Performance", passed);
    
    free(x_data);
    free(y_data);
}

/* ============================================================================
 * MAIN TEST RUNNER
 * ============================================================================ */

int main(void) {
    printf("========================================\n");
    printf("RTKA Correlation Analysis Test Suite\n");
    printf("Copyright (c) 2025 - H.Overman\n");
    printf("Email: opsec.ee@pm.me\n");
    printf("========================================\n\n");
    
    /* Basic functionality tests */
    printf("=== Basic Functionality Tests ===\n");
    test_mean_calculation();
    test_variance_calculation();
    test_perfect_positive_correlation();
    test_perfect_negative_correlation();
    test_no_correlation();
    printf("\n");
    
    /* Edge case tests */
    printf("=== Edge Case Tests ===\n");
    test_zero_variance_error();
    test_insufficient_samples_error();
    test_null_pointer_error();
    printf("\n");
    
    /* Correlation matrix tests */
    printf("=== Correlation Matrix Tests ===\n");
    test_correlation_matrix();
    printf("\n");
    
    /* Utility function tests */
    printf("=== Utility Function Tests ===\n");
    test_interpretation();
    printf("\n");
    
    /* Performance tests */
    printf("=== Performance Tests ===\n");
    test_performance_large_dataset();
    printf("\n");
    
    /* Print summary */
    print_test_summary();
    
    return (g_test_results.failed == 0U) ? 0 : 1;
}
