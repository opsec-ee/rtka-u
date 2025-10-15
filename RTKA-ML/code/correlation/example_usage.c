/**
 * File: example_usage.c
 * Copyright (c) 2025 - H.Overman opsec.ee@pm.me
 * Email: opsec.ee@pm.me
 * 
 * RTKA Correlation Analysis - Example Usage
 * 
 * Demonstrates practical usage of the correlation library with
 * realistic examples.
 */

#include "rtka_correlation.h"
#include <stdio.h>
#include <stdlib.h>

/* ============================================================================
 * EXAMPLE 1: Basic Correlation Calculation
 * ============================================================================ */

static void example_basic_correlation(void) {
    printf("\n");
    printf("========================================\n");
    printf("Example 1: Basic Correlation\n");
    printf("========================================\n");
    
    /* Sample data: height (cm) vs weight (kg) */
    double heights[] = {165.0, 170.0, 175.0, 180.0, 185.0};
    double weights[] = {55.0, 62.0, 68.0, 75.0, 82.0};
    size_t count = sizeof(heights) / sizeof(heights[0]);
    
    printf("Dataset: Height vs Weight\n");
    printf("Samples: %zu pairs\n\n", count);
    
    /* Calculate correlation */
    rtka_corr_result_t result = {0};
    rtka_corr_error_t err = rtka_corr_pearson(
        heights, weights, count, &result
    );
    
    if (err == RTKA_CORR_SUCCESS) {
        printf("Results:\n");
        printf("  Correlation coefficient: %.6f\n", result.coefficient);
        printf("  Interpretation: %s correlation\n",
               rtka_corr_interpret(result.coefficient));
        printf("  Covariance: %.3f\n", result.covariance);
        printf("\n");
        printf("  Height statistics:\n");
        printf("    Mean: %.2f cm\n", result.x_stats.mean);
        printf("    Std Dev: %.2f cm\n", result.x_stats.std_dev);
        printf("\n");
        printf("  Weight statistics:\n");
        printf("    Mean: %.2f kg\n", result.y_stats.mean);
        printf("    Std Dev: %.2f kg\n", result.y_stats.std_dev);
        
        /* Check if significant */
        bool sig = rtka_corr_is_significant(
            result.coefficient, count, 0.95
        );
        printf("\n");
        printf("  Statistically significant (95%% confidence): %s\n",
               sig ? "Yes" : "No");
    } else {
        printf("Error: %d\n", err);
    }
}

/* ============================================================================
 * EXAMPLE 2: Simplified Interface
 * ============================================================================ */

static void example_simplified_interface(void) {
    printf("\n");
    printf("========================================\n");
    printf("Example 2: Simplified Interface\n");
    printf("========================================\n");
    
    /* Temperature (C) vs ice cream sales (units) */
    double temperature[] = {15.0, 18.0, 22.0, 25.0, 28.0, 30.0, 32.0};
    double sales[] = {120.0, 145.0, 180.0, 210.0, 245.0, 270.0, 290.0};
    size_t count = sizeof(temperature) / sizeof(temperature[0]);
    
    printf("Dataset: Temperature vs Ice Cream Sales\n");
    printf("Samples: %zu pairs\n\n", count);
    
    /* Quick calculation - just get the coefficient */
    double coefficient = 0.0;
    rtka_corr_error_t err = rtka_corr_pearson_simple(
        temperature, sales, count, &coefficient
    );
    
    if (err == RTKA_CORR_SUCCESS) {
        printf("Correlation coefficient: %.6f\n", coefficient);
        printf("Interpretation: %s\n", rtka_corr_interpret(coefficient));
        
        /* This shows strong positive correlation between temperature
         * and ice cream sales - expected relationship! */
    } else {
        printf("Error: %d\n", err);
    }
}

/* ============================================================================
 * EXAMPLE 3: Correlation Matrix
 * ============================================================================ */

static void example_correlation_matrix(void) {
    printf("\n");
    printf("========================================\n");
    printf("Example 3: Correlation Matrix\n");
    printf("========================================\n");
    
    /* Three variables: study hours, sleep hours, test scores */
    const size_t num_vars = 3U;
    const size_t num_samples = 8U;
    
    double study_hours[] = {2.0, 3.0, 4.0, 5.0, 6.0, 7.0, 8.0, 9.0};
    double sleep_hours[] = {8.0, 7.5, 7.0, 7.0, 6.5, 6.0, 5.5, 5.0};
    double test_scores[] = {65.0, 70.0, 75.0, 80.0, 85.0, 88.0, 90.0, 92.0};
    
    /* Organize data as data[variable][sample] */
    const double* data[3] = {study_hours, sleep_hours, test_scores};
    const char* var_names[3] = {"Study Hours", "Sleep Hours", "Test Scores"};
    
    printf("Variables: Study Hours, Sleep Hours, Test Scores\n");
    printf("Samples: %zu\n\n", num_samples);
    
    /* Create and compute correlation matrix */
    rtka_corr_matrix_t* matrix = rtka_corr_matrix_create(
        num_vars, num_samples
    );
    
    if (matrix == NULL) {
        printf("Error: Failed to create matrix\n");
        return;
    }
    
    rtka_corr_error_t err = rtka_corr_matrix_compute(
        data, num_vars, num_samples, matrix
    );
    
    if (err == RTKA_CORR_SUCCESS) {
        printf("Correlation Matrix:\n");
        printf("%-15s", "");
        for (size_t col = 0U; col < num_vars; col++) {
            printf("%-15s", var_names[col]);
        }
        printf("\n");
        
        for (size_t row = 0U; row < num_vars; row++) {
            printf("%-15s", var_names[row]);
            for (size_t col = 0U; col < num_vars; col++) {
                double value = 0.0;
                rtka_corr_matrix_get(matrix, row, col, &value);
                printf("%-15.6f", value);
            }
            printf("\n");
        }
        
        printf("\n");
        printf("Observations:\n");
        
        double study_test = 0.0;
        rtka_corr_matrix_get(matrix, 0U, 2U, &study_test);
        printf("  Study Hours <-> Test Scores: %.3f (%s)\n",
               study_test, rtka_corr_interpret(study_test));
        
        double sleep_test = 0.0;
        rtka_corr_matrix_get(matrix, 1U, 2U, &sleep_test);
        printf("  Sleep Hours <-> Test Scores: %.3f (%s)\n",
               sleep_test, rtka_corr_interpret(sleep_test));
        
        double study_sleep = 0.0;
        rtka_corr_matrix_get(matrix, 0U, 1U, &study_sleep);
        printf("  Study Hours <-> Sleep Hours: %.3f (%s)\n",
               study_sleep, rtka_corr_interpret(study_sleep));
    } else {
        printf("Error computing matrix: %d\n", err);
    }
    
    rtka_corr_matrix_free(matrix);
}

/* ============================================================================
 * EXAMPLE 4: Statistics Calculation
 * ============================================================================ */

static void example_statistics(void) {
    printf("\n");
    printf("========================================\n");
    printf("Example 4: Statistical Analysis\n");
    printf("========================================\n");
    
    /* Monthly revenue data (in thousands) */
    double revenue[] = {
        45.2, 48.7, 52.3, 49.8, 55.1,
        58.4, 61.2, 59.7, 63.5, 67.8,
        70.2, 73.6
    };
    size_t count = sizeof(revenue) / sizeof(revenue[0]);
    
    printf("Dataset: Monthly Revenue (in thousands)\n");
    printf("Samples: %zu months\n\n", count);
    
    /* Calculate comprehensive statistics */
    rtka_stats_t stats = {0};
    rtka_corr_error_t err = rtka_corr_statistics(revenue, count, &stats);
    
    if (err == RTKA_CORR_SUCCESS) {
        printf("Statistical Summary:\n");
        printf("  Mean: $%.2fk\n", stats.mean);
        printf("  Standard Deviation: $%.2fk\n", stats.std_dev);
        printf("  Variance: %.2f\n", stats.variance);
        printf("  Sample Count: %zu\n", stats.count);
        
        /* Calculate coefficient of variation (relative variability) */
        double cv = (stats.std_dev / stats.mean) * 100.0;
        printf("  Coefficient of Variation: %.2f%%\n", cv);
        
        /* Interpret CV */
        printf("\n");
        if (cv < 15.0) {
            printf("Interpretation: Low variability (stable revenue)\n");
        } else if (cv < 30.0) {
            printf("Interpretation: Moderate variability\n");
        } else {
            printf("Interpretation: High variability (volatile revenue)\n");
        }
    } else {
        printf("Error: %d\n", err);
    }
}

/* ============================================================================
 * MAIN PROGRAM
 * ============================================================================ */

int main(void) {
    printf("========================================\n");
    printf("RTKA Correlation Analysis\n");
    printf("Example Usage Demonstrations\n");
    printf("Copyright (c) 2025 - H.Overman\n");
    printf("Email: opsec.ee@pm.me\n");
    printf("========================================\n");
    
    /* Run all examples */
    example_basic_correlation();
    example_simplified_interface();
    example_correlation_matrix();
    example_statistics();
    
    printf("\n");
    printf("========================================\n");
    printf("All examples completed successfully!\n");
    printf("========================================\n");
    
    return 0;
}
