/**
 * File: rtka_robotics_control.h
 * Copyright (c) 2025 - H.Overman opsec.ee@pm.me
 * Email: opsec.ee@pm.me
 * 
 * RTKA Robotics Control System
 * Confidence-driven continuous control for robotic systems
 * 
 * CHANGELOG:
 * v1.0.0 - 2025-10-13: Initial implementation
 *   - Confidence-driven proportional control
 *   - Hierarchical mode switching
 *   - Double pendulum stabilization
 *   - Status: NEW - Requires validation
 * 
 * VALIDATION STATUS: UNTESTED
 * This implementation requires:
 *   1. Physics simulation validation
 *   2. Hardware testing
 *   3. Stability analysis
 *   4. Performance comparison with baselines
 */

#ifndef RTKA_ROBOTICS_CONTROL_H
#define RTKA_ROBOTICS_CONTROL_H

#include <stdint.h>
#include <stdbool.h>
#include <math.h>

/* ============================================================================
 * RTKA CORE TYPES (from rtka_core.h)
 * ============================================================================ */

typedef int8_t rtka_value_t;
typedef float rtka_confidence_t;

#define RTKA_FALSE  ((rtka_value_t)-1)
#define RTKA_UNKNOWN ((rtka_value_t)0)
#define RTKA_TRUE   ((rtka_value_t)1)

typedef struct {
    rtka_value_t value;
    rtka_confidence_t confidence;
} rtka_state_t;

/* ============================================================================
 * OPT-230: CONFIDENCE-DRIVEN PROPORTIONAL CONTROL
 * ============================================================================ */

/**
 * Control parameter mapping configuration
 * Maps confidence levels to control outputs
 */
typedef struct {
    float confidence_low;   /* Below this: increase control effort */
    float confidence_high;  /* Above this: reduce control effort */
    float u_min;           /* Minimum control output */
    float u_nominal;       /* Steady-state output */
    float u_max;           /* Maximum control output */
    float gain_increase;   /* Gain coefficient for low confidence */
    float gain_decrease;   /* Gain coefficient for high confidence */
    float rate_limit;      /* Maximum output change per timestep */
} rtka_control_map_t;

/**
 * Sensor reading with variance
 * Extends rtka_state_t with measurement uncertainty
 */
typedef struct {
    rtka_value_t value;
    float confidence;
    float variance;
} rtka_sensor_reading_t;

/**
 * Control state
 * Tracks control loop state across iterations
 */
typedef struct {
    float output;          /* Current control output */
    float prev_output;     /* Previous output (for rate limiting) */
    float confidence;      /* Aggregated confidence from sensors */
    uint32_t iteration;    /* Control loop counter */
    
    /* Statistics */
    uint32_t saturation_count;    /* Times output hit limits */
    uint32_t rate_limit_count;    /* Times rate limiter engaged */
    float avg_confidence;         /* Running average confidence */
} rtka_control_state_t;

/**
 * Compute aggregated confidence from sensor array
 * Uses RTKA recursive AND: C = ∏cᵢ
 * 
 * @param sensors Array of sensor readings
 * @param n_sensors Number of sensors
 * @return Aggregated confidence [0, 1]
 */
float rtka_compute_control_confidence(
    const rtka_sensor_reading_t* sensors,
    uint32_t n_sensors
);

/**
 * Map confidence to control output
 * Implements piecewise linear mapping
 * 
 * @param confidence Input confidence [0, 1]
 * @param map Control mapping parameters
 * @return Control output [u_min, u_max]
 */
float rtka_confidence_to_control(
    float confidence,
    const rtka_control_map_t* map
);

/**
 * Main control step
 * Combines sensor fusion, confidence mapping, and rate limiting
 * 
 * @param state Control state (updated in-place)
 * @param sensors Array of sensor readings
 * @param n_sensors Number of sensors
 * @param map Control mapping parameters
 * @return Control output for this timestep
 */
float rtka_control_step(
    rtka_control_state_t* state,
    const rtka_sensor_reading_t* sensors,
    uint32_t n_sensors,
    const rtka_control_map_t* map
);

/**
 * Reset control state
 * Clears history and statistics
 */
void rtka_reset_control_state(rtka_control_state_t* state);

/**
 * Print control state statistics
 */
void rtka_print_control_stats(const rtka_control_state_t* state);

/* ============================================================================
 * OPT-231: HIERARCHICAL MODE SWITCHING
 * ============================================================================ */

/**
 * Control modes
 * Ordered by decreasing performance/increasing safety
 */
typedef enum {
    RTKA_MODE_NOMINAL,      /* Full performance, high confidence */
    RTKA_MODE_DEGRADED,     /* Reduced performance, moderate confidence */
    RTKA_MODE_SAFE,         /* Minimum functionality, low confidence */
    RTKA_MODE_EMERGENCY     /* System shutdown, critical failure */
} rtka_control_mode_t;

/**
 * Mode configuration
 * Defines thresholds and control parameters for each mode
 */
typedef struct {
    rtka_control_mode_t mode;
    float confidence_threshold;    /* Entry threshold */
    float hysteresis_band;         /* Hysteresis width */
    float dwell_time_sec;          /* Minimum time before transition */
    rtka_control_map_t control_params;
} rtka_mode_config_t;

/**
 * Mode controller state
 * Manages hierarchical mode switching
 */
typedef struct {
    rtka_control_mode_t current_mode;
    rtka_control_mode_t previous_mode;
    float mode_entry_time;
    float time_in_mode;
    uint32_t mode_transitions;
    
    /* Hysteresis state */
    float confidence_history[16];
    uint32_t history_idx;
    
    /* Mode configurations */
    rtka_mode_config_t configs[4];
} rtka_mode_controller_t;

/**
 * Initialize mode controller with default configurations
 * 
 * @param ctrl Mode controller (initialized in-place)
 */
void rtka_init_mode_controller(rtka_mode_controller_t* ctrl);

/**
 * Update mode controller
 * Checks for mode transitions based on confidence and dwell time
 * 
 * @param ctrl Mode controller (updated in-place)
 * @param confidence Current aggregated confidence
 * @param dt Timestep duration (seconds)
 * @return Control parameters for active mode
 */
const rtka_control_map_t* rtka_update_mode_controller(
    rtka_mode_controller_t* ctrl,
    float confidence,
    float dt
);

/**
 * Get mode name string
 */
const char* rtka_get_mode_name(rtka_control_mode_t mode);

/**
 * Print mode controller statistics
 */
void rtka_print_mode_stats(const rtka_mode_controller_t* ctrl);

/**
 * Force mode transition (for testing/emergency)
 */
void rtka_force_mode(rtka_mode_controller_t* ctrl, rtka_control_mode_t mode);

/* ============================================================================
 * OPT-232: DOUBLE PENDULUM CONTROL
 * ============================================================================ */

#define RTKA_NUM_PENDULUM_SENSORS 5

/**
 * Double pendulum state
 * Complete kinematic state
 */
typedef struct {
    float theta1;       /* Joint 1 angle (rad) */
    float theta2;       /* Joint 2 angle (rad) */
    float omega1;       /* Joint 1 angular velocity (rad/s) */
    float omega2;       /* Joint 2 angular velocity (rad/s) */
    float alpha1;       /* Joint 1 angular acceleration (rad/s²) */
    float alpha2;       /* Joint 2 angular acceleration (rad/s²) */
} rtka_pendulum_state_t;

/**
 * Double pendulum physical parameters
 */
typedef struct {
    float m1, m2;       /* Masses (kg) */
    float l1, l2;       /* Lengths (m) */
    float g;            /* Gravity (m/s²) */
    float b1, b2;       /* Damping coefficients (N·m·s/rad) */
    float tau_max;      /* Maximum torque (N·m) */
} rtka_pendulum_params_t;

/**
 * Complete pendulum controller
 * Integrates state, sensors, control, and mode management
 */
typedef struct {
    rtka_sensor_reading_t sensors[RTKA_NUM_PENDULUM_SENSORS];
    rtka_pendulum_state_t state;
    rtka_pendulum_params_t params;
    rtka_control_state_t control;
    rtka_mode_controller_t mode_ctrl;
    float dt;           /* Timestep (s) */
    
    /* Statistics */
    uint32_t steps;
    float total_energy;
    float max_angle1;
    float max_angle2;
    uint32_t uncontrollable_count;
} rtka_pendulum_controller_t;

/**
 * Initialize pendulum controller with default parameters
 * 
 * @param ctrl Pendulum controller (initialized in-place)
 * @param dt Timestep duration (seconds)
 */
void rtka_init_pendulum_controller(
    rtka_pendulum_controller_t* ctrl,
    float dt
);

/**
 * Set pendulum physical parameters
 */
void rtka_set_pendulum_params(
    rtka_pendulum_controller_t* ctrl,
    float m1, float m2,
    float l1, float l2,
    float g,
    float b1, float b2,
    float tau_max
);

/**
 * Compute total mechanical energy
 * E = KE + PE
 * 
 * @param state Pendulum state
 * @param params Physical parameters
 * @return Total energy (Joules)
 */
float rtka_compute_pendulum_energy(
    const rtka_pendulum_state_t* state,
    const rtka_pendulum_params_t* params
);

/**
 * Check if pendulum is in controllable region
 * Based on small angle approximation validity
 * 
 * @param state Pendulum state
 * @param params Physical parameters
 * @return true if controllable
 */
bool rtka_is_pendulum_controllable(
    const rtka_pendulum_state_t* state,
    const rtka_pendulum_params_t* params
);

/**
 * Update sensor readings from pendulum state
 * Simulates sensor fusion with realistic noise
 * 
 * @param ctrl Pendulum controller (sensors updated in-place)
 * @param encoder_noise Encoder noise std dev (rad)
 * @param gyro_noise Gyroscope noise std dev (rad/s)
 * @param accel_noise Accelerometer noise std dev (m/s²)
 */
void rtka_update_pendulum_sensors(
    rtka_pendulum_controller_t* ctrl,
    float encoder_noise,
    float gyro_noise,
    float accel_noise
);

/**
 * Main pendulum control step
 * Computes control torque from sensors via RTKA confidence fusion
 * 
 * @param ctrl Pendulum controller (updated in-place)
 * @return Control torque (N·m)
 */
float rtka_pendulum_control_step(rtka_pendulum_controller_t* ctrl);

/**
 * Print pendulum controller statistics
 */
void rtka_print_pendulum_stats(const rtka_pendulum_controller_t* ctrl);

/**
 * Reset pendulum controller statistics
 */
void rtka_reset_pendulum_stats(rtka_pendulum_controller_t* ctrl);

/* ============================================================================
 * VARIANCE-WEIGHTED FUSION (from OPT-225)
 * ============================================================================ */

/**
 * Fusion result with aggregated confidence and variance
 */
typedef struct {
    rtka_value_t fused;
    float confidence;
    float total_variance;
} rtka_fusion_result_t;

/**
 * Variance-weighted sensor fusion
 * Implements OPT-225 with LUT acceleration
 * 
 * @param sensors Array of sensor readings
 * @param n Number of sensors
 * @return Fused result with confidence
 */
rtka_fusion_result_t rtka_fuse_sensors_weighted(
    const rtka_sensor_reading_t* sensors,
    uint32_t n
);

/* ============================================================================
 * UTILITY FUNCTIONS
 * ============================================================================ */

/**
 * Initialize variance weight LUT
 * Called automatically via constructor attribute
 */
void rtka_init_variance_lut(void) __attribute__((constructor));

/**
 * Clamp value to range
 */
static inline float rtka_clampf(float x, float min, float max) {
    if (x < min) return min;
    if (x > max) return max;
    return x;
}

/**
 * Apply rate limiting
 */
static inline float rtka_apply_rate_limit(
    float current,
    float previous,
    float rate_limit
) {
    float delta = current - previous;
    if (fabsf(delta) > rate_limit) {
        return previous + copysignf(rate_limit, delta);
    }
    return current;
}

#endif /* RTKA_ROBOTICS_CONTROL_H */
