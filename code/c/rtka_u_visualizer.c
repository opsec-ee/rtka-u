/**
 * RTKA-U Algorithm Visualization Demonstration
 * 
 * Comprehensive graphical analysis program that generates visual representations
 * of algorithm behavior across multiple operational dimensions.
 *
 * Author: H.Overman <opsec.ee@pm.ee>
 */

#include "rtka_u_plot.h"
#include <stdio.h>
#include <stdlib.h>

// Interactive menu system for plot selection
void print_menu(void) {
    printf("\nRTKA-U Algorithm Visualization System\n");
    printf("=====================================\n\n");
    printf("Available Visualizations:\n");
    printf("  1. Confidence Decay Analysis\n");
    printf("  2. UNKNOWN Level Protection\n");
    printf("  3. Mathematical Properties\n");
    printf("  4. Performance Scaling\n");
    printf("  5. Early Termination Patterns\n");
    printf("  6. Generate Complete Dashboard\n");
    printf("  7. Interactive Algorithm Demo\n");
    printf("  0. Exit\n\n");
    printf("Select visualization (0-7): ");
}

// Interactive algorithm demonstration
void interactive_algorithm_demo(void) {
    printf("\nINTERACTIVE ALGORITHM DEMONSTRATION\n");
    printf("===================================\n\n");
    
    printf("This demonstration shows real-time algorithm behavior\n");
    printf("as you modify input parameters.\n\n");
    
    size_t vector_size;
    double base_confidence;
    size_t unknown_count;
    
    printf("Enter vector size (1-50): ");
    if (scanf("%zu", &vector_size) != 1 || vector_size == 0 || vector_size > 50) {
        printf("Invalid input. Using default: 10\n");
        vector_size = 10;
    }
    
    printf("Enter base confidence (0.1-1.0): ");
    if (scanf("%lf", &base_confidence) != 1 || base_confidence <= 0.0 || base_confidence > 1.0) {
        printf("Invalid input. Using default: 0.8\n");
        base_confidence = 0.8;
    }
    
    printf("Enter number of UNKNOWN values (0-%zu): ", vector_size);
    if (scanf("%zu", &unknown_count) != 1 || unknown_count > vector_size) {
        printf("Invalid input. Using default: %zu\n", vector_size / 3);
        unknown_count = vector_size / 3;
    }
    
    // Create test vector based on user input
    rtka_vector_t *demo_vector = rtka_vector_create(vector_size);
    
    for (size_t i = 0; i < vector_size; i++) {
        if (i < unknown_count) {
            rtka_vector_push(demo_vector, RTKA_UNKNOWN, base_confidence);
        } else if (i % 3 == 0) {
            rtka_vector_push(demo_vector, RTKA_FALSE, base_confidence);
        } else {
            rtka_vector_push(demo_vector, RTKA_TRUE, base_confidence);
        }
    }
    
    printf("\nGenerated Vector Contents:\n");
    printf("Position | Value   | Confidence\n");
    printf("---------|---------|------------\n");
    for (size_t i = 0; i < demo_vector->length; i++) {
        printf("%8zu | %7s | %10.6f\n", i, 
               rtka_ternary_to_string(demo_vector->values[i]),
               demo_vector->confidences[i]);
    }
    
    // Test all operations
    rtka_operation_t operations[] = {RTKA_OP_AND, RTKA_OP_OR, RTKA_OP_NOT};
    const char* op_names[] = {"AND", "OR", "NOT"};
    
    printf("\nAlgorithm Results:\n");
    printf("Operation | Result  | Confidence | Operations | Early Term | UNKNOWN Count | Limit Reached\n");
    printf("----------|---------|------------|------------|------------|---------------|---------------\n");
    
    for (size_t op = 0; op < 3; op++) {
        rtka_result_t result = rtka_recursive_eval(operations[op], demo_vector);
        
        printf("%9s | %7s | %10.6f | %10zu | %10s | %13zu | %13s\n",
               op_names[op],
               rtka_ternary_to_string(result.value),
               result.confidence,
               result.operations_performed,
               result.early_terminated ? "Yes" : "No",
               result.unknown_levels,
               result.unknown_limit_reached ? "Yes" : "No");
    }
    
    // Generate a small visualization of confidence decay
    printf("\nConfidence Decay Visualization:\n");
    printf("Vector Size | Confidence | Visual Representation\n");
    printf("------------|------------|");
    for (int i = 0; i < 30; i++) printf("-");
    printf("\n");
    
    for (size_t test_size = 1; test_size <= 15; test_size++) {
        rtka_vector_t *test_vector = rtka_vector_create(test_size);
        for (size_t i = 0; i < test_size; i++) {
            rtka_vector_push(test_vector, RTKA_TRUE, base_confidence);
        }
        
        rtka_result_t result = rtka_recursive_eval(RTKA_OP_AND, test_vector);
        
        printf("%11zu | %10.6f | ", test_size, result.confidence);
        
        int bar_length = (int)(result.confidence * 30);
        for (int i = 0; i < bar_length; i++) {
            printf("*");
        }
        printf("\n");
        
        rtka_vector_destroy(test_vector);
    }
    
    rtka_vector_destroy(demo_vector);
    
    printf("\nPress Enter to continue...");
    getchar(); // Clear previous input
    getchar(); // Wait for user input
}

// Comprehensive system analysis
void generate_system_analysis(void) {
    printf("\nGENERATING COMPREHENSIVE SYSTEM ANALYSIS\n");
    printf("=========================================\n\n");
    
    printf("Analyzing algorithm behavior across multiple dimensions...\n");
    
    // Generate detailed analysis
    rtka_plot_generate_dashboard(".");
    
    printf("Analysis complete. Detailed visualizations saved to rtka_u_analysis_dashboard.txt\n");
    printf("You can view this file to see comprehensive graphical analysis.\n");
}

int main(void) {
    int choice;
    
    printf("RTKA-U Algorithm Visualization System\n");
    printf("Comprehensive graphical analysis and demonstration tool\n\n");
    
    do {
        print_menu();
        
        if (scanf("%d", &choice) != 1) {
            printf("Invalid input. Please enter a number.\n");
            while (getchar() != '\n'); // Clear input buffer
            continue;
        }
        
        switch (choice) {
            case 1:
                rtka_plot_confidence_decay_analysis(stdout);
                break;
                
            case 2:
                rtka_plot_unknown_level_protection(stdout);
                break;
                
            case 3:
                rtka_plot_mathematical_properties(stdout);
                break;
                
            case 4:
                rtka_plot_performance_scaling(stdout);
                break;
                
            case 5:
                rtka_plot_early_termination_patterns(stdout);
                break;
                
            case 6:
                generate_system_analysis();
                break;
                
            case 7:
                interactive_algorithm_demo();
                break;
                
            case 0:
                printf("Exiting visualization system.\n");
                break;
                
            default:
                printf("Invalid choice. Please select 0-7.\n");
                break;
        }
        
        if (choice != 0 && choice != 6 && choice != 7) {
            printf("\nPress Enter to continue...");
            getchar(); // Clear previous input
            getchar(); // Wait for user input
        }
        
    } while (choice != 0);
    
    return 0;
}
