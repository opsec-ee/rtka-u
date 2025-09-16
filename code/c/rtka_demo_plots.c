/*
* Author: H.Overman <opsec.ee@pm.ee>
*/

#include "rtka_u_plot.h"
#include <stdio.h>

int main(void) {
    printf("RTKA-U Algorithm Visualization Demonstration\n");
    printf("============================================\n\n");
    
    printf("Generating sample visualizations to demonstrate system capabilities...\n\n");
    
    // Generate mathematical properties visualization
    rtka_plot_mathematical_properties(stdout);
    
    // Generate confidence decay analysis
    rtka_plot_confidence_decay_analysis(stdout);
    
    // Generate UNKNOWN level protection visualization
    rtka_plot_unknown_level_protection(stdout);
    
    return 0;
}
