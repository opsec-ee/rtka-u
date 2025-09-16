/**
 * RTKA-U Plotting and Visualization System
 * 
 * Comprehensive graphical analysis tools for visualizing algorithm behavior,
 * confidence propagation patterns, UNKNOWN level protection, and performance
 * characteristics across various operational parameters.
 *
 * Author: H.Overman <opsec.ee@pm.ee>
 */

#ifndef RTKA_U_PLOT_H
#define RTKA_U_PLOT_H

#include "rtka_u.h"
#include <stdio.h>
#include <math.h>

// Plot configuration constants
#define PLOT_MAX_WIDTH 120
#define PLOT_MAX_HEIGHT 40
#define PLOT_DATA_POINTS 100
#define PLOT_PRECISION 6

// Plot types for different visualization requirements
typedef enum {
    PLOT_CONFIDENCE_DECAY = 0,
    PLOT_UNKNOWN_PROPAGATION = 1,
    PLOT_LEVEL_PROTECTION = 2,
    PLOT_PERFORMANCE_SCALING = 3,
    PLOT_STATE_TRANSITIONS = 4,
    PLOT_MATHEMATICAL_PROPERTIES = 5,
    PLOT_OPERATION_COMPARISON = 6,
    PLOT_EARLY_TERMINATION = 7
} rtka_plot_type_t;

// Plot configuration structure
typedef struct {
    rtka_plot_type_t type;
    size_t width;
    size_t height;
    double min_x;
    double max_x;
    double min_y;
    double max_y;
    bool show_grid;
    bool show_labels;
    bool use_colors;
    char title[256];
    char x_label[128];
    char y_label[128];
} rtka_plot_config_t;

// Data series for plotting
typedef struct {
    double *x_values;
    double *y_values;
    size_t length;
    char label[64];
    char symbol;
    bool is_line;
} rtka_plot_series_t;

// Plot canvas structure
typedef struct {
    char **canvas;
    size_t width;
    size_t height;
    rtka_plot_config_t config;
    rtka_plot_series_t *series;
    size_t series_count;
    size_t series_capacity;
} rtka_plot_canvas_t;

// Plot generation functions
rtka_plot_canvas_t* rtka_plot_create(const rtka_plot_config_t *config);
void rtka_plot_destroy(rtka_plot_canvas_t *canvas);
bool rtka_plot_add_series(rtka_plot_canvas_t *canvas, const rtka_plot_series_t *series);
void rtka_plot_render(const rtka_plot_canvas_t *canvas, FILE *output);
void rtka_plot_save_to_file(const rtka_plot_canvas_t *canvas, const char *filename);

// Specialized plotting functions for RTKA-U algorithm analysis
void rtka_plot_confidence_decay_analysis(FILE *output);
void rtka_plot_unknown_level_protection(FILE *output);
void rtka_plot_operation_comparison(FILE *output);
void rtka_plot_performance_scaling(FILE *output);
void rtka_plot_state_transition_behavior(FILE *output);
void rtka_plot_mathematical_properties(FILE *output);
void rtka_plot_early_termination_patterns(FILE *output);

// Comprehensive analysis dashboard
void rtka_plot_generate_dashboard(const char *output_directory);

// Data generation utilities for plotting
rtka_plot_series_t* rtka_generate_confidence_decay_series(size_t max_vector_size);
rtka_plot_series_t* rtka_generate_unknown_protection_series(size_t max_unknown_count);
rtka_plot_series_t* rtka_generate_performance_series(size_t max_vector_size);
rtka_plot_series_t* rtka_generate_mathematical_property_series(size_t max_n);

// Random data generation for plotting demonstrations
rtka_vector_t* rtka_plot_generate_random_vector(size_t length);

// Utility functions
rtka_plot_config_t rtka_plot_create_config(rtka_plot_type_t type);
void rtka_plot_auto_scale(rtka_plot_canvas_t *canvas);
char rtka_plot_value_to_char(double value, double min_val, double max_val);
void rtka_plot_draw_axes(rtka_plot_canvas_t *canvas);
void rtka_plot_draw_grid(rtka_plot_canvas_t *canvas);
void rtka_plot_draw_labels(rtka_plot_canvas_t *canvas);

// Series management
rtka_plot_series_t* rtka_plot_series_create(size_t capacity);
void rtka_plot_series_destroy(rtka_plot_series_t *series);
bool rtka_plot_series_add_point(rtka_plot_series_t *series, double x, double y);

#endif // RTKA_U_PLOT_H
