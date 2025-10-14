/**
 * File: rtka_robotics_control.c
 * Copyright (c) 2025 - H.Overman opsec.ee@pm.me
 * Email: opsec.ee@pm.me
 * 
 * RTKA Robotics Control System - Implementation
 * 
 * CHANGELOG:
 * v1.0.0 - 2025-10-13: Initial implementation
 *   - Core control functions
 *   - Mode switching
 *   - Double pendulum control
 * 
 * VALIDATION STATUS: UNTESTED - Requires simulation validation
 */

#include "rtka_robotics_control.h"
#include <stdio.h>
#include <string.h>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

/* ============================================================================
 * VARIANCE WEIGHT LOOKUP TABLE (OPT-225)
 * ============================================================================ */

#define VAR_LUT_SIZE 101
static float g_var_weight_lut[VAR_LUT_SIZE] __attribute__((aligned(64)));

void rtka_init_variance_lut(void) {
    for (uint32_t i = 0; i < VAR_LUT_SIZE; i++) {
        float variance = (float)i / (float)(VAR_LUT_SIZE - 1);
        g_var_weight_lut[i] = 1.0f / (1.0f + variance);
    }
}

/* ============================================================================
 * OPT-230: CONFIDENCE-DRIVEN PROPORTIONAL CONTROL
 * ============================================================================ */

float rtka_compute_control_confidence(
    const rtka_sensor_reading_t* sensors,
    uint32_t n_sensors
) {
    if (n_sensors == 0) return 0.0f;
    
    float confidence = 1.0f;
    
    for (uint32_t i = 0; i < n_sensors; i++) {
        /* Early termination on critical failure */
        if (sensors[i].value == RTKA_FALSE) {
            return 0.0f;
        }
        
        /* Multiplicative confidence decay (RTKA recursive AND) */
        confidence *= sensors[i].confidence;
        
        /* Early exit if confidence becomes negligible */
        if (confidence < 0.01f) {
            return confidence;
        }
    }
    
    return confidence;
}

float rtka_confidence_to_control(
    float confidence,
    const rtka_control_map_t* map
) {
    float output;
    
    if (confidence < map->confidence_low) {
        /* Low confidence: increase control effort proportionally */
        float deficit = map->confidence_low - confidence;
        float range = map->u_max - map->u_nominal;
        output = map->u_nominal + map->gain_increase * deficit * range;
    }
    else if (confidence > map->confidence_high) {
        /* High confidence: reduce control effort proportionally */
        float excess = confidence - map->confidence_high;
        float range = map->u_nominal - map->u_min;
        output = map->u_nominal - map->gain_decrease * excess * range;
    }
    else {
        /* Moderate confidence: steady state */
        output = map->u_nominal;
    }
    
    /* Clamp to valid range */
    return rtka_clampf(output, map->u_min, map->u_max);
}

float rtka_control_step(
    rtka_control_state_t* state,
    const rtka_sensor_reading_t* sensors,
    uint32_t n_sensors,
    const rtka_control_map_t* map
) {
    /* 1. Aggregate sensor confidences */
    state->confidence = rtka_compute_control_confidence(sensors, n_sensors);
    
    /* 2. Map confidence to control output */
    float raw_output = rtka_confidence_to_control(state->confidence, map);
    
    /* 3. Apply rate limiting */
    float limited_output = rtka_apply_rate_limit(
        raw_output,
        state->prev_output,
        map->rate_limit
    );
    
    /* 4. Check for saturation and rate limiting */
    if (fabsf(raw_output - map->u_min) < 0.01f || 
        fabsf(raw_output - map->u_max) < 0.01f) {
        state->saturation_count++;
    }
    
    if (fabsf(limited_output - raw_output) > 0.01f) {
        state->rate_limit_count++;
    }
    
    /* 5. Update running average confidence */
    float alpha = 0.95f;
    state->avg_confidence = alpha * state->avg_confidence + 
                           (1.0f - alpha) * state->confidence;
    
    /* 6. Update state */
    state->prev_output = state->output;
    state->output = limited_output;
    state->iteration++;
    
    return limited_output;
}

void rtka_reset_control_state(rtka_control_state_t* state) {
    state->output = 0.0f;
    state->prev_output = 0.0f;
    state->confidence = 1.0f;
    state->iteration = 0;
    state->saturation_count = 0;
    state->rate_limit_count = 0;
    state->avg_confidence = 1.0f;
}

void rtka_print_control_stats(const rtka_control_state_t* state) {
    printf("Control Statistics:\n");
    printf("  Iterations: %u\n", state->iteration);
    printf("  Current output: %.4f\n", state->output);
    printf("  Current confidence: %.4f\n", state->confidence);
    printf("  Average confidence: %.4f\n", state->avg_confidence);
    printf("  Saturation events: %u (%.2f%%)\n", 
           state->saturation_count,
           100.0f * state->saturation_count / (float)state->iteration);
    printf("  Rate limit events: %u (%.2f%%)\n",
           state->rate_limit_count,
           100.0f * state->rate_limit_count / (float)state->iteration);
}

/* ============================================================================
 * OPT-231: HIERARCHICAL MODE SWITCHING
 * ============================================================================ */

void rtka_init_mode_controller(rtka_mode_controller_t* ctrl) {
    memset(ctrl, 0, sizeof(*ctrl));
    
    /* NOMINAL mode configuration */
    ctrl->configs[RTKA_MODE_NOMINAL] = (rtka_mode_config_t){
        .mode = RTKA_MODE_NOMINAL,
        .confidence_threshold = 0.70f,
        .hysteresis_band = 0.05f,
        .dwell_time_sec = 0.5f,
        .control_params = {
            .confidence_low = 0.6f,
            .confidence_high = 0.9f,
            .u_min = -10.0f,
            .u_nominal = 0.0f,
            .u_max = 10.0f,
            .gain_increase = 2.0f,
            .gain_decrease = 1.0f,
            .rate_limit = 5.0f
        }
    };
    
    /* DEGRADED mode configuration */
    ctrl->configs[RTKA_MODE_DEGRADED] = (rtka_mode_config_t){
        .mode = RTKA_MODE_DEGRADED,
        .confidence_threshold = 0.40f,
        .hysteresis_band = 0.05f,
        .dwell_time_sec = 0.3f,
        .control_params = {
            .confidence_low = 0.3f,
            .confidence_high = 0.8f,
            .u_min = -5.0f,
            .u_nominal = 0.0f,
            .u_max = 5.0f,
            .gain_increase = 1.5f,
            .gain_decrease = 1.2f,
            .rate_limit = 2.0f
        }
    };
    
    /* SAFE mode configuration */
    ctrl->configs[RTKA_MODE_SAFE] = (rtka_mode_config_t){
        .mode = RTKA_MODE_SAFE,
        .confidence_threshold = 0.10f,
        .hysteresis_band = 0.05f,
        .dwell_time_sec = 2.0f,
        .control_params = {
            .confidence_low = 0.05f,
            .confidence_high = 0.5f,
            .u_min = -1.0f,
            .u_nominal = 0.0f,
            .u_max = 1.0f,
            .gain_increase = 1.0f,
            .gain_decrease = 1.0f,
            .rate_limit = 0.5f
        }
    };
    
    /* EMERGENCY mode configuration */
    ctrl->configs[RTKA_MODE_EMERGENCY] = (rtka_mode_config_t){
        .mode = RTKA_MODE_EMERGENCY,
        .confidence_threshold = 0.0f,
        .hysteresis_band = 0.0f,
        .dwell_time_sec = 0.0f,
        .control_params = {
            .confidence_low = 0.0f,
            .confidence_high = 0.0f,
            .u_min = 0.0f,
            .u_nominal = 0.0f,
            .u_max = 0.0f,
            .gain_increase = 0.0f,
            .gain_decrease = 0.0f,
            .rate_limit = 0.0f
        }
    };
    
    ctrl->current_mode = RTKA_MODE_NOMINAL;
    ctrl->previous_mode = RTKA_MODE_NOMINAL;
}

static rtka_control_mode_t determine_target_mode(
    float confidence,
    rtka_control_mode_t current_mode
) {
    /* EMERGENCY: immediate transition, no hysteresis */
    if (confidence < 0.05f) {
        return RTKA_MODE_EMERGENCY;
    }
    
    /* Apply hysteresis based on current mode */
    switch (current_mode) {
        case RTKA_MODE_NOMINAL:
            if (confidence < 0.65f) return RTKA_MODE_DEGRADED;
            return RTKA_MODE_NOMINAL;
            
        case RTKA_MODE_DEGRADED:
            if (confidence > 0.75f) return RTKA_MODE_NOMINAL;
            if (confidence < 0.35f) return RTKA_MODE_SAFE;
            return RTKA_MODE_DEGRADED;
            
        case RTKA_MODE_SAFE:
            if (confidence > 0.45f) return RTKA_MODE_DEGRADED;
            if (confidence < 0.05f) return RTKA_MODE_EMERGENCY;
            return RTKA_MODE_SAFE;
            
        case RTKA_MODE_EMERGENCY:
            /* Require manual recovery */
            return RTKA_MODE_EMERGENCY;
    }
    
    return current_mode;
}

const rtka_control_map_t* rtka_update_mode_controller(
    rtka_mode_controller_t* ctrl,
    float confidence,
    float dt
) {
    /* Update time in current mode */
    ctrl->time_in_mode += dt;
    
    /* Store confidence in history */
    ctrl->confidence_history[ctrl->history_idx] = confidence;
    ctrl->history_idx = (ctrl->history_idx + 1) & 15;
    
    /* Determine target mode */
    rtka_control_mode_t target_mode = determine_target_mode(
        confidence,
        ctrl->current_mode
    );
    
    /* Check dwell time requirement before transition */
    if (target_mode != ctrl->current_mode) {
        float required_dwell = ctrl->configs[ctrl->current_mode].dwell_time_sec;
        
        /* Transition if dwell time satisfied or emergency */
        if (ctrl->time_in_mode >= required_dwell || 
            target_mode == RTKA_MODE_EMERGENCY) {
            ctrl->previous_mode = ctrl->current_mode;
            ctrl->current_mode = target_mode;
            ctrl->mode_entry_time += ctrl->time_in_mode;
            ctrl->time_in_mode = 0.0f;
            ctrl->mode_transitions++;
        }
    }
    
    /* Return control parameters for current mode */
    return &ctrl->configs[ctrl->current_mode].control_params;
}

const char* rtka_get_mode_name(rtka_control_mode_t mode) {
    switch (mode) {
        case RTKA_MODE_NOMINAL: return "NOMINAL";
        case RTKA_MODE_DEGRADED: return "DEGRADED";
        case RTKA_MODE_SAFE: return "SAFE";
        case RTKA_MODE_EMERGENCY: return "EMERGENCY";
        default: return "UNKNOWN";
    }
}

void rtka_print_mode_stats(const rtka_mode_controller_t* ctrl) {
    printf("Mode Controller Statistics:\n");
    printf("  Current mode: %s\n", rtka_get_mode_name(ctrl->current_mode));
    printf("  Time in mode: %.3f s\n", ctrl->time_in_mode);
    printf("  Total transitions: %u\n", ctrl->mode_transitions);
    printf("  Total runtime: %.3f s\n", 
           ctrl->mode_entry_time + ctrl->time_in_mode);
}

void rtka_force_mode(rtka_mode_controller_t* ctrl, rtka_control_mode_t mode) {
    ctrl->previous_mode = ctrl->current_mode;
    ctrl->current_mode = mode;
    ctrl->time_in_mode = 0.0f;
    ctrl->mode_transitions++;
}

/* ============================================================================
 * VARIANCE-WEIGHTED FUSION (OPT-225)
 * ============================================================================ */

rtka_fusion_result_t rtka_fuse_sensors_weighted(
    const rtka_sensor_reading_t* sensors,
    uint32_t n
) {
    if (n == 0) {
        return (rtka_fusion_result_t){RTKA_UNKNOWN, 0.0f, 0.0f};
    }
    
    float weighted_sum = 0.0f;
    float weight_sum = 0.0f;
    float total_var = 0.0f;
    float conf_prod_or = 1.0f;
    
    for (uint32_t i = 0; i < n; i++) {
        /* Variance weight from LUT */
        uint32_t var_idx = (uint32_t)(sensors[i].variance * (VAR_LUT_SIZE - 1));
        if (var_idx >= VAR_LUT_SIZE) var_idx = VAR_LUT_SIZE - 1;
        float var_weight = g_var_weight_lut[var_idx];
        
        /* Combined variance and confidence weighting */
        float combined_weight = var_weight * sensors[i].confidence;
        float weighted_value = (float)sensors[i].value * combined_weight;
        
        weighted_sum += weighted_value;
        weight_sum += sensors[i].confidence;
        total_var += sensors[i].variance;
        
        /* Probabilistic OR for confidence: 1 - ∏(1-cᵢ) */
        conf_prod_or *= (1.0f - sensors[i].confidence);
    }
    
    /* Consensus from weighted average */
    float consensus = (weight_sum > 0.0f) ? weighted_sum / weight_sum : 0.0f;
    
    /* Quantize to ternary */
    rtka_value_t result;
    if (consensus > 0.5f) {
        result = RTKA_TRUE;
    } else if (consensus < -0.5f) {
        result = RTKA_FALSE;
    } else {
        result = RTKA_UNKNOWN;
    }
    
    float aggregate_conf = 1.0f - conf_prod_or;
    
    return (rtka_fusion_result_t){
        .fused = result,
        .confidence = aggregate_conf,
        .total_variance = total_var / (float)n
    };
}

/* ============================================================================
 * OPT-232: DOUBLE PENDULUM CONTROL
 * ============================================================================ */

void rtka_init_pendulum_controller(
    rtka_pendulum_controller_t* ctrl,
    float dt
) {
    memset(ctrl, 0, sizeof(*ctrl));
    
    ctrl->dt = dt;
    
    /* Initialize with default physical parameters */
    rtka_set_pendulum_params(ctrl, 1.0f, 1.0f, 1.0f, 1.0f, 9.81f, 0.1f, 0.1f, 10.0f);
    
    /* Initialize control state */
    rtka_reset_control_state(&ctrl->control);
    
    /* Initialize mode controller */
    rtka_init_mode_controller(&ctrl->mode_ctrl);
    
    /* Initialize sensor states */
    for (uint32_t i = 0; i < RTKA_NUM_PENDULUM_SENSORS; i++) {
        ctrl->sensors[i].value = RTKA_TRUE;
        ctrl->sensors[i].confidence = 0.9f;
        ctrl->sensors[i].variance = 0.01f;
    }
}

void rtka_set_pendulum_params(
    rtka_pendulum_controller_t* ctrl,
    float m1, float m2,
    float l1, float l2,
    float g,
    float b1, float b2,
    float tau_max
) {
    ctrl->params.m1 = m1;
    ctrl->params.m2 = m2;
    ctrl->params.l1 = l1;
    ctrl->params.l2 = l2;
    ctrl->params.g = g;
    ctrl->params.b1 = b1;
    ctrl->params.b2 = b2;
    ctrl->params.tau_max = tau_max;
}

float rtka_compute_pendulum_energy(
    const rtka_pendulum_state_t* state,
    const rtka_pendulum_params_t* params
) {
    /* Kinetic energy */
    float KE = 0.5f * params->m1 * params->l1 * params->l1 * 
               state->omega1 * state->omega1;
    
    float cos_delta = cosf(state->theta1 - state->theta2);
    KE += 0.5f * params->m2 * (
        params->l1 * params->l1 * state->omega1 * state->omega1 +
        params->l2 * params->l2 * state->omega2 * state->omega2 +
        2.0f * params->l1 * params->l2 * state->omega1 * state->omega2 * cos_delta
    );
    
    /* Potential energy (relative to hanging position) */
    float PE = -(params->m1 + params->m2) * params->g * params->l1 * 
               cosf(state->theta1);
    PE -= params->m2 * params->g * params->l2 * cosf(state->theta2);
    
    return KE + PE;
}

bool rtka_is_pendulum_controllable(
    const rtka_pendulum_state_t* state,
    const rtka_pendulum_params_t* params
) {
    /* Define controllable region (small angle approximation valid) */
    const float max_angle = 0.5f;  /* ~28 degrees */
    const float max_velocity = 2.0f;  /* rad/s */
    
    bool angle_ok = (fabsf(state->theta1) < max_angle) && 
                    (fabsf(state->theta2) < max_angle);
    bool velocity_ok = (fabsf(state->omega1) < max_velocity) && 
                       (fabsf(state->omega2) < max_velocity);
    
    return angle_ok && velocity_ok;
}

void rtka_update_pendulum_sensors(
    rtka_pendulum_controller_t* ctrl,
    float encoder_noise,
    float gyro_noise,
    float accel_noise
) {
    bool controllable = rtka_is_pendulum_controllable(&ctrl->state, &ctrl->params);
    float energy = rtka_compute_pendulum_energy(&ctrl->state, &ctrl->params);
    
    /* Encoder 1: theta1 measurement */
    ctrl->sensors[0].value = RTKA_TRUE;
    ctrl->sensors[0].variance = encoder_noise * encoder_noise;
    ctrl->sensors[0].confidence = controllable ? 0.90f : 0.30f;
    
    /* Encoder 2: theta2 measurement */
    ctrl->sensors[1].value = RTKA_TRUE;
    ctrl->sensors[1].variance = encoder_noise * encoder_noise;
    ctrl->sensors[1].confidence = controllable ? 0.90f : 0.30f;
    
    /* Gyroscope 1: omega1 measurement */
    ctrl->sensors[2].value = RTKA_TRUE;
    ctrl->sensors[2].variance = gyro_noise * gyro_noise;
    ctrl->sensors[2].confidence = fabsf(ctrl->state.omega1) < 5.0f ? 0.85f : 0.40f;
    
    /* Gyroscope 2: omega2 measurement */
    ctrl->sensors[3].value = RTKA_TRUE;
    ctrl->sensors[3].variance = gyro_noise * gyro_noise;
    ctrl->sensors[3].confidence = fabsf(ctrl->state.omega2) < 5.0f ? 0.85f : 0.40f;
    
    /* IMU accelerometer */
    ctrl->sensors[4].value = RTKA_TRUE;
    ctrl->sensors[4].variance = accel_noise * accel_noise;
    ctrl->sensors[4].confidence = energy < 10.0f ? 0.80f : 0.20f;
    
    /* Further degrade confidence in uncontrollable regions */
    if (!controllable) {
        for (uint32_t i = 0; i < RTKA_NUM_PENDULUM_SENSORS; i++) {
            ctrl->sensors[i].confidence *= 0.3f;
        }
    }
    
    /* Degrade confidence at high energy */
    if (energy > 20.0f) {
        for (uint32_t i = 0; i < RTKA_NUM_PENDULUM_SENSORS; i++) {
            ctrl->sensors[i].confidence *= 0.5f;
        }
    }
}

float rtka_pendulum_control_step(rtka_pendulum_controller_t* ctrl) {
    /* 1. Update mode based on confidence and energy */
    float energy = rtka_compute_pendulum_energy(&ctrl->state, &ctrl->params);
    bool controllable = rtka_is_pendulum_controllable(&ctrl->state, &ctrl->params);
    
    /* 2. Get control parameters for current mode */
    const rtka_control_map_t* params = rtka_update_mode_controller(
        &ctrl->mode_ctrl,
        ctrl->control.confidence,
        ctrl->dt
    );
    
    /* 3. Compute control torque */
    float torque = rtka_control_step(
        &ctrl->control,
        ctrl->sensors,
        RTKA_NUM_PENDULUM_SENSORS,
        params
    );
    
    /* 4. Apply physical limits */
    torque = rtka_clampf(torque, -ctrl->params.tau_max, ctrl->params.tau_max);
    
    /* 5. Emergency shutdown */
    if (ctrl->mode_ctrl.current_mode == RTKA_MODE_EMERGENCY) {
        torque = 0.0f;
    }
    
    /* 6. Update statistics */
    ctrl->steps++;
    ctrl->total_energy += energy;
    if (fabsf(ctrl->state.theta1) > ctrl->max_angle1) {
        ctrl->max_angle1 = fabsf(ctrl->state.theta1);
    }
    if (fabsf(ctrl->state.theta2) > ctrl->max_angle2) {
        ctrl->max_angle2 = fabsf(ctrl->state.theta2);
    }
    if (!controllable) {
        ctrl->uncontrollable_count++;
    }
    
    return torque;
}

void rtka_print_pendulum_stats(const rtka_pendulum_controller_t* ctrl) {
    printf("\nDouble Pendulum Controller Statistics:\n");
    printf("========================================\n");
    
    printf("\nPhysical State:\n");
    printf("  theta1: %.4f rad (%.2f deg)\n", 
           ctrl->state.theta1, ctrl->state.theta1 * 180.0f / M_PI);
    printf("  theta2: %.4f rad (%.2f deg)\n",
           ctrl->state.theta2, ctrl->state.theta2 * 180.0f / M_PI);
    printf("  omega1: %.4f rad/s\n", ctrl->state.omega1);
    printf("  omega2: %.4f rad/s\n", ctrl->state.omega2);
    
    float energy = rtka_compute_pendulum_energy(&ctrl->state, &ctrl->params);
    bool controllable = rtka_is_pendulum_controllable(&ctrl->state, &ctrl->params);
    
    printf("\nSystem Status:\n");
    printf("  Energy: %.4f J\n", energy);
    printf("  Controllable: %s\n", controllable ? "YES" : "NO");
    printf("  Current mode: %s\n", 
           rtka_get_mode_name(ctrl->mode_ctrl.current_mode));
    
    printf("\nPerformance Metrics:\n");
    printf("  Steps: %u\n", ctrl->steps);
    printf("  Average energy: %.4f J\n", ctrl->total_energy / ctrl->steps);
    printf("  Max |theta1|: %.4f rad (%.2f deg)\n",
           ctrl->max_angle1, ctrl->max_angle1 * 180.0f / M_PI);
    printf("  Max |theta2|: %.4f rad (%.2f deg)\n",
           ctrl->max_angle2, ctrl->max_angle2 * 180.0f / M_PI);
    printf("  Uncontrollable: %u steps (%.2f%%)\n",
           ctrl->uncontrollable_count,
           100.0f * ctrl->uncontrollable_count / ctrl->steps);
    printf("  Mode transitions: %u\n", ctrl->mode_ctrl.mode_transitions);
    
    printf("\n");
    rtka_print_control_stats(&ctrl->control);
}

void rtka_reset_pendulum_stats(rtka_pendulum_controller_t* ctrl) {
    ctrl->steps = 0;
    ctrl->total_energy = 0.0f;
    ctrl->max_angle1 = 0.0f;
    ctrl->max_angle2 = 0.0f;
    ctrl->uncontrollable_count = 0;
    rtka_reset_control_state(&ctrl->control);
}
