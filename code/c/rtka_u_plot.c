/**
 * RTKA-U Plotting and Visualization System Implementation
 * 
 * Provides comprehensive graphical analysis capabilities for visualizing
 * algorithm behavior across multiple operational dimensions.
 *
 * Author: H.Overman <opsec.ee@pm.ee>
 */

#include "rtka_u_plot.h"
#include <stdlib.h>
#include <string.h>
#include <time.h>

// Plot canvas management
rtka_plot_canvas_t* rtka_plot_create(const rtka_plot_config_t *config) {
    if (!config) return NULL;
    
    rtka_plot_canvas_t *canvas = malloc(sizeof(rtka_plot_canvas_t));
    if (!canvas) return NULL;
    
    canvas->width = config->width;
    canvas->height = config->height;
    canvas->config = *config;
    canvas->series_count = 0;
    canvas->series_capacity = 10;
    
    // Allocate canvas memory
    canvas->canvas = malloc(canvas->height * sizeof(char*));
    if (!canvas->canvas) {
        free(canvas);
        return NULL;
    }
    
    for (size_t i = 0; i < canvas->height; i++) {
        canvas->canvas[i] = calloc(canvas->width + 1, sizeof(char));
        if (!canvas->canvas[i]) {
            for (size_t j = 0; j < i; j++) {
                free(canvas->canvas[j]);
            }
            free(canvas->canvas);
            free(canvas);
            return NULL;
        }
        memset(canvas->canvas[i], ' ', canvas->width);
        canvas->canvas[i][canvas->width] = '\0';
    }
    
    // Allocate series array
    canvas->series = calloc(canvas->series_capacity, sizeof(rtka_plot_series_t));
    if (!canvas->series) {
        rtka_plot_destroy(canvas);
        return NULL;
    }
    
    return canvas;
}

void rtka_plot_destroy(rtka_plot_canvas_t *canvas) {
    if (!canvas) return;
    
    if (canvas->canvas) {
        for (size_t i = 0; i < canvas->height; i++) {
            free(canvas->canvas[i]);
        }
        free(canvas->canvas);
    }
    
    if (canvas->series) {
        for (size_t i = 0; i < canvas->series_count; i++) {
            rtka_plot_series_destroy(&canvas->series[i]);
        }
        free(canvas->series);
    }
    
    free(canvas);
}

// Series management
rtka_plot_series_t* rtka_plot_series_create(size_t capacity) {
    rtka_plot_series_t *series = malloc(sizeof(rtka_plot_series_t));
    if (!series) return NULL;
    
    series->x_values = malloc(capacity * sizeof(double));
    series->y_values = malloc(capacity * sizeof(double));
    if (!series->x_values || !series->y_values) {
        free(series->x_values);
        free(series->y_values);
        free(series);
        return NULL;
    }
    
    series->length = 0;
    series->symbol = '*';
    series->is_line = true;
    memset(series->label, 0, sizeof(series->label));
    
    return series;
}

void rtka_plot_series_destroy(rtka_plot_series_t *series) {
    if (series) {
        free(series->x_values);
        free(series->y_values);
        // Note: Don't free series itself if it's part of an array
    }
}

bool rtka_plot_series_add_point(rtka_plot_series_t *series, double x, double y) {
    if (!series) return false;
    
    series->x_values[series->length] = x;
    series->y_values[series->length] = y;
    series->length++;
    return true;
}

// Plotting utilities
char rtka_plot_value_to_char(double value, double min_val, double max_val) {
    if (max_val <= min_val) return ' ';
    
    double normalized = (value - min_val) / (max_val - min_val);
    
    if (normalized < 0.1) return '.';
    else if (normalized < 0.3) return ':';
    else if (normalized < 0.5) return '+';
    else if (normalized < 0.7) return '*';
    else if (normalized < 0.9) return '#';
    else return '@';
}

void rtka_plot_draw_axes(rtka_plot_canvas_t *canvas) {
    if (!canvas) return;
    
    // Draw Y-axis
    for (size_t y = 0; y < canvas->height - 1; y++) {
        canvas->canvas[y][0] = '|';
    }
    
    // Draw X-axis
    for (size_t x = 0; x < canvas->width; x++) {
        canvas->canvas[canvas->height - 1][x] = '-';
    }
    
    // Origin
    canvas->canvas[canvas->height - 1][0] = '+';
}

void rtka_plot_render(const rtka_plot_canvas_t *canvas, FILE *output) {
    if (!canvas || !output) return;
    
    // Print title
    if (strlen(canvas->config.title) > 0) {
        fprintf(output, "\n%s\n", canvas->config.title);
        for (size_t i = 0; i < strlen(canvas->config.title); i++) {
            fprintf(output, "=");
        }
        fprintf(output, "\n");
    }
    
    // Print Y-axis label
    if (strlen(canvas->config.y_label) > 0) {
        fprintf(output, "%s ^\n", canvas->config.y_label);
        fprintf(output, "%*s|\n", (int)strlen(canvas->config.y_label), "");
    }
    
    // Print canvas
    for (size_t y = 0; y < canvas->height; y++) {
        fprintf(output, "%s\n", canvas->canvas[y]);
    }
    
    // Print X-axis label
    if (strlen(canvas->config.x_label) > 0) {
        fprintf(output, "%s\n", canvas->config.x_label);
    }
    
    // Print legend
    if (canvas->series_count > 0) {
        fprintf(output, "\nLegend:\n");
        for (size_t i = 0; i < canvas->series_count; i++) {
            if (strlen(canvas->series[i].label) > 0) {
                fprintf(output, "  %c - %s\n", canvas->series[i].symbol, canvas->series[i].label);
            }
        }
    }
    
    fprintf(output, "\n");
}

// Confidence decay analysis
void rtka_plot_confidence_decay_analysis(FILE *output) {
    fprintf(output, "CONFIDENCE DECAY ANALYSIS\n");
    fprintf(output, "=========================\n\n");
    
    // Generate test data for confidence decay
    rtka_plot_config_t config = {
        .type = PLOT_CONFIDENCE_DECAY,
        .width = 80,
        .height = 25,
        .min_x = 0,
        .max_x = 20,
        .min_y = 0,
        .max_y = 1.0,
        .show_grid = true,
        .show_labels = true
    };
    strcpy(config.title, "Confidence Decay vs Vector Size");
    strcpy(config.x_label, "Vector Size");
    strcpy(config.y_label, "Confidence");
    
    rtka_plot_canvas_t *canvas = rtka_plot_create(&config);
    if (!canvas) return;
    
    rtka_plot_draw_axes(canvas);
    
    // Plot confidence decay for different base confidence levels
    double base_confidences[] = {0.95, 0.90, 0.85, 0.80};
    char symbols[] = {'*', '#', '+', '.'};
    
    for (size_t b = 0; b < 4; b++) {
        double base_conf = base_confidences[b];
        
        for (size_t vector_size = 1; vector_size <= 20; vector_size++) {
            rtka_vector_t *test_vector = rtka_vector_create(vector_size);
            
            for (size_t i = 0; i < vector_size; i++) {
                rtka_vector_push(test_vector, RTKA_TRUE, base_conf);
            }
            
            rtka_result_t result = rtka_recursive_eval(RTKA_OP_AND, test_vector);
            
            // Map to canvas coordinates
            double x_norm = (double)vector_size / 20.0;
            double y_norm = result.confidence;
            
            size_t canvas_x = (size_t)(x_norm * (canvas->width - 2)) + 1;
            size_t canvas_y = (size_t)((1.0 - y_norm) * (canvas->height - 2));
            
            if (canvas_x < canvas->width && canvas_y < canvas->height) {
                canvas->canvas[canvas_y][canvas_x] = symbols[b];
            }
            
            rtka_vector_destroy(test_vector);
        }
    }
    
    rtka_plot_render(canvas, output);
    
    // Print legend for base confidence levels
    fprintf(output, "Base Confidence Levels:\n");
    for (size_t b = 0; b < 4; b++) {
        fprintf(output, "  %c - %.2f\n", symbols[b], base_confidences[b]);
    }
    
    rtka_plot_destroy(canvas);
}

// UNKNOWN level protection visualization
void rtka_plot_unknown_level_protection(FILE *output) {
    fprintf(output, "UNKNOWN LEVEL PROTECTION ANALYSIS\n");
    fprintf(output, "==================================\n\n");
    
    rtka_plot_config_t config = {
        .type = PLOT_LEVEL_PROTECTION,
        .width = 100,
        .height = 30,
        .min_x = 0,
        .max_x = 120,
        .min_y = 0,
        .max_y = 1.0
    };
    strcpy(config.title, "UNKNOWN Level Protection Mechanism");
    strcpy(config.x_label, "UNKNOWN Count");
    strcpy(config.y_label, "Processing Status");
    
    rtka_plot_canvas_t *canvas = rtka_plot_create(&config);
    if (!canvas) return;
    
    rtka_plot_draw_axes(canvas);
    
    // Test UNKNOWN level protection
    for (size_t unknown_count = 5; unknown_count <= 120; unknown_count += 3) {
        rtka_vector_t *test_vector = rtka_vector_create(unknown_count + 2);
        
        rtka_vector_push(test_vector, RTKA_TRUE, 0.9);
        for (size_t i = 0; i < unknown_count; i++) {
            rtka_vector_push(test_vector, RTKA_UNKNOWN, 0.8);
        }
        rtka_vector_push(test_vector, RTKA_TRUE, 0.9);
        
        rtka_result_t result = rtka_recursive_eval(RTKA_OP_AND, test_vector);
        
        double x_norm = (double)unknown_count / 120.0;
        double y_value = result.unknown_limit_reached ? 0.0 : 1.0;
        
        size_t canvas_x = (size_t)(x_norm * (canvas->width - 2)) + 1;
        size_t canvas_y = (size_t)((1.0 - y_value) * (canvas->height - 2));
        
        if (canvas_x < canvas->width && canvas_y < canvas->height) {
            char symbol = result.unknown_limit_reached ? 'X' : 'O';
            canvas->canvas[canvas_y][canvas_x] = symbol;
        }
        
        rtka_vector_destroy(test_vector);
    }
    
    // Mark the protection threshold
    size_t threshold_x = (size_t)((double)RTKA_MAX_UNKNOWN_LEVELS / 120.0 * (canvas->width - 2)) + 1;
    for (size_t y = 0; y < canvas->height - 1; y++) {
        canvas->canvas[y][threshold_x] = '|';
    }
    
    rtka_plot_render(canvas, output);
    
    fprintf(output, "Protection Status:\n");
    fprintf(output, "  O - Processing Allowed\n");
    fprintf(output, "  X - Protection Active (FALSE forced)\n");
    fprintf(output, "  | - Protection Threshold (%d UNKNOWN levels)\n", RTKA_MAX_UNKNOWN_LEVELS);
}

// Mathematical properties visualization
void rtka_plot_mathematical_properties(FILE *output) {
    fprintf(output, "MATHEMATICAL PROPERTIES VISUALIZATION\n");
    fprintf(output, "=====================================\n\n");
    
    rtka_plot_config_t config = {
        .width = 80,
        .height = 25
    };
    strcpy(config.title, "UNKNOWN Probability Distribution: P(UNKNOWN|n) = (2/3)^n");
    strcpy(config.x_label, "n (sequence length)");
    strcpy(config.y_label, "Probability");
    
    rtka_plot_canvas_t *canvas = rtka_plot_create(&config);
    if (!canvas) return;
    
    rtka_plot_draw_axes(canvas);
    
    // Plot (2/3)^n curve
    for (size_t n = 1; n <= 30; n++) {
        double prob = rtka_unknown_probability(n);
        
        double x_norm = (double)n / 30.0;
        double y_norm = prob;
        
        size_t canvas_x = (size_t)(x_norm * (canvas->width - 2)) + 1;
        size_t canvas_y = (size_t)((1.0 - y_norm) * (canvas->height - 2));
        
        if (canvas_x < canvas->width && canvas_y < canvas->height) {
            canvas->canvas[canvas_y][canvas_x] = '*';
        }
    }
    
    rtka_plot_render(canvas, output);
    
    // Tabular data for precise values
    fprintf(output, "Precise Values:\n");
    fprintf(output, "n  | P(UNKNOWN|n)\n");
    fprintf(output, "---|-------------\n");
    for (size_t n = 1; n <= 15; n++) {
        fprintf(output, "%2zu | %.6f\n", n, rtka_unknown_probability(n));
    }
    
    rtka_plot_destroy(canvas);
}

// Performance scaling analysis
void rtka_plot_performance_scaling(FILE *output) {
    fprintf(output, "PERFORMANCE SCALING ANALYSIS\n");
    fprintf(output, "============================\n\n");
    
    rtka_plot_config_t config = {
        .width = 80,
        .height = 25
    };
    strcpy(config.title, "Execution Time vs Vector Size");
    strcpy(config.x_label, "Vector Size");
    strcpy(config.y_label, "Execution Time (relative)");
    
    rtka_plot_canvas_t *canvas = rtka_plot_create(&config);
    if (!canvas) return;
    
    rtka_plot_draw_axes(canvas);
    
    double max_time = 0.0;
    double times[20];
    
    // Measure performance across vector sizes
    for (size_t vector_size = 10; vector_size <= 200; vector_size += 10) {
        rtka_vector_t *test_vector = rtka_plot_generate_random_vector(vector_size);
        
        clock_t start = clock();
        for (int i = 0; i < 100; i++) {  // Multiple iterations for measurement stability
            rtka_result_t result = rtka_recursive_eval(RTKA_OP_AND, test_vector);
            (void)result;  // Suppress unused variable warning
        }
        clock_t end = clock();
        
        double time_taken = (double)(end - start) / CLOCKS_PER_SEC;
        times[(vector_size / 10) - 1] = time_taken;
        if (time_taken > max_time) max_time = time_taken;
        
        rtka_vector_destroy(test_vector);
    }
    
    // Plot the results
    for (size_t i = 0; i < 20; i++) {
        double x_norm = (double)i / 19.0;
        double y_norm = times[i] / max_time;
        
        size_t canvas_x = (size_t)(x_norm * (canvas->width - 2)) + 1;
        size_t canvas_y = (size_t)((1.0 - y_norm) * (canvas->height - 2));
        
        if (canvas_x < canvas->width && canvas_y < canvas->height) {
            canvas->canvas[canvas_y][canvas_x] = '#';
        }
    }
    
    rtka_plot_render(canvas, output);
    rtka_plot_destroy(canvas);
}

// Early termination patterns
void rtka_plot_early_termination_patterns(FILE *output) {
    fprintf(output, "EARLY TERMINATION PATTERN ANALYSIS\n");
    fprintf(output, "===================================\n\n");
    
    rtka_plot_config_t config = {
        .width = 60,
        .height = 20
    };
    strcpy(config.title, "Early Termination vs Absorbing Element Position");
    strcpy(config.x_label, "Absorbing Element Position");
    strcpy(config.y_label, "Operations Required");
    
    rtka_plot_canvas_t *canvas = rtka_plot_create(&config);
    if (!canvas) return;
    
    rtka_plot_draw_axes(canvas);
    
    // Test early termination for AND operations
    for (size_t absorbing_pos = 1; absorbing_pos <= 20; absorbing_pos++) {
        rtka_vector_t *test_vector = rtka_vector_create(25);
        
        // Fill with TRUE values
        for (size_t i = 0; i < 25; i++) {
            if (i == absorbing_pos) {
                rtka_vector_push(test_vector, RTKA_FALSE, 0.9);  // Absorbing element
            } else {
                rtka_vector_push(test_vector, RTKA_TRUE, 0.9);
            }
        }
        
        rtka_result_t result = rtka_recursive_eval(RTKA_OP_AND, test_vector);
        
        double x_norm = (double)absorbing_pos / 20.0;
        double y_norm = (double)result.operations_performed / 25.0;
        
        size_t canvas_x = (size_t)(x_norm * (canvas->width - 2)) + 1;
        size_t canvas_y = (size_t)((1.0 - y_norm) * (canvas->height - 2));
        
        if (canvas_x < canvas->width && canvas_y < canvas->height) {
            canvas->canvas[canvas_y][canvas_x] = '*';
        }
        
        rtka_vector_destroy(test_vector);
    }
    
    rtka_plot_render(canvas, output);
    rtka_plot_destroy(canvas);
}

// Comprehensive dashboard generator
void rtka_plot_generate_dashboard(const char *output_directory) {
    char filename[512];
    FILE *dashboard_file;
    
    // Create comprehensive analysis file
    snprintf(filename, sizeof(filename), "%s/rtka_u_analysis_dashboard.txt", output_directory);
    dashboard_file = fopen(filename, "w");
    if (!dashboard_file) return;
    
    fprintf(dashboard_file, "RTKA-U ALGORITHM COMPREHENSIVE ANALYSIS DASHBOARD\n");
    fprintf(dashboard_file, "==================================================\n");
    fprintf(dashboard_file, "Generated: %s\n", ctime(&(time_t){time(NULL)}));
    
    rtka_plot_confidence_decay_analysis(dashboard_file);
    rtka_plot_unknown_level_protection(dashboard_file);
    rtka_plot_mathematical_properties(dashboard_file);
    rtka_plot_performance_scaling(dashboard_file);
    rtka_plot_early_termination_patterns(dashboard_file);
    
    fclose(dashboard_file);
    
    printf("Analysis dashboard generated: %s\n", filename);
}

// Random data generation for plotting demonstrations
rtka_vector_t* rtka_plot_generate_random_vector(size_t length) {
    rtka_vector_t* vector = rtka_vector_create(length);
    if (!vector) return NULL;
    
    // Initialize random seed if not already done
    static bool seed_initialized = false;
    if (!seed_initialized) {
        srand((unsigned int)time(NULL));
        seed_initialized = true;
    }
    
    for (size_t i = 0; i < length; i++) {
        // Generate random ternary value
        int rand_val = rand() % 3;
        rtka_ternary_t value;
        switch (rand_val) {
            case 0: value = RTKA_FALSE; break;
            case 1: value = RTKA_UNKNOWN; break;
            case 2: value = RTKA_TRUE; break;
            default: value = RTKA_UNKNOWN; break;
        }
        
        // Generate random confidence value between 0.3 and 1.0
        double confidence = 0.3 + ((double)rand() / RAND_MAX) * 0.7;
        
        rtka_vector_push(vector, value, confidence);
    }
    
    return vector;
}

// Configuration helper
rtka_plot_config_t rtka_plot_create_config(rtka_plot_type_t type) {
    rtka_plot_config_t config = {
        .type = type,
        .width = 80,
        .height = 25,
        .min_x = 0,
        .max_x = 100,
        .min_y = 0,
        .max_y = 1.0,
        .show_grid = true,
        .show_labels = true,
        .use_colors = false
    };
    
    memset(config.title, 0, sizeof(config.title));
    memset(config.x_label, 0, sizeof(config.x_label));
    memset(config.y_label, 0, sizeof(config.y_label));
    
    return config;
}
