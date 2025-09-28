/**
 * File: rtka_reinforcement.h
 * Copyright (c) 2025 - H.Overman opsec.ee@pm.me
 * 
 * PROPRIETARY AND CONFIDENTIAL
 * This file is proprietary to H. Overman and may not be reproduced,
 * distributed, or used without explicit written permission.
 *
 * For licensing inquiries: opsec.ee@pm.me
 *
 * RTKA Reinforcement Learning
 */

#ifndef RTKA_REINFORCEMENT_H
#define RTKA_REINFORCEMENT_H

#include "rtka_types.h"
#include "rtka_tensor.h"
#include "rtka_nn.h"
#include "rtka_ml.h"

/* Environment interface */
typedef struct {
    rtka_tensor_t* (*reset)(void* env_data);
    rtka_tensor_t* (*step)(void* env_data, rtka_value_t action, rtka_confidence_t* reward, bool* done);
    uint32_t observation_dim;
    uint32_t action_dim;
    void* env_data;
} rtka_environment_t;

/* Experience replay buffer */
typedef struct {
    rtka_tensor_t** states;
    rtka_value_t* actions;
    rtka_confidence_t* rewards;
    rtka_tensor_t** next_states;
    bool* dones;
    uint32_t capacity;
    uint32_t size;
    uint32_t position;
} rtka_replay_buffer_t;

/* Q-Network for ternary actions */
typedef struct {
    rtka_model_t* q_network;
    rtka_model_t* target_network;
    rtka_replay_buffer_t* buffer;
    rtka_confidence_t epsilon;
    rtka_confidence_t gamma;
    uint32_t update_frequency;
} rtka_dqn_agent_t;

/* Policy gradient agent */
typedef struct {
    rtka_model_t* policy_network;
    rtka_model_t* value_network;
    rtka_tensor_t** trajectory_states;
    rtka_value_t* trajectory_actions;
    rtka_confidence_t* trajectory_rewards;
    uint32_t trajectory_length;
} rtka_pg_agent_t;

/* Agent creation */
rtka_dqn_agent_t* rtka_rl_create_dqn(uint32_t state_dim, uint32_t action_dim,
                                     rtka_confidence_t epsilon, rtka_confidence_t gamma);
rtka_pg_agent_t* rtka_rl_create_pg(uint32_t state_dim, uint32_t action_dim);

/* Training */
void rtka_rl_train_dqn(rtka_dqn_agent_t* agent, rtka_environment_t* env, uint32_t episodes);
void rtka_rl_train_pg(rtka_pg_agent_t* agent, rtka_environment_t* env, uint32_t episodes);

/* Action selection */
rtka_value_t rtka_rl_select_action_epsilon_greedy(rtka_dqn_agent_t* agent, rtka_tensor_t* state);
rtka_value_t rtka_rl_select_action_policy(rtka_pg_agent_t* agent, rtka_tensor_t* state);

/* Ternary Q-learning update */
RTKA_INLINE void rtka_rl_update_q_ternary(rtka_state_t* q_value, 
                                         rtka_confidence_t reward,
                                         rtka_state_t max_next_q,
                                         rtka_confidence_t alpha,
                                         rtka_confidence_t gamma) {
    /* TD error with ternary logic */
    rtka_confidence_t td_target = reward + gamma * max_next_q.confidence;
    rtka_confidence_t td_error = td_target - q_value->confidence;
    
    /* Update confidence */
    q_value->confidence += alpha * td_error;
    
    /* Update ternary value based on new confidence */
    if (q_value->confidence > 0.66f) {
        q_value->value = RTKA_TRUE;
    } else if (q_value->confidence < 0.33f) {
        q_value->value = RTKA_FALSE;
    } else {
        q_value->value = RTKA_UNKNOWN;
    }
}

/* Experience replay operations */
rtka_replay_buffer_t* rtka_rl_create_buffer(uint32_t capacity);
void rtka_rl_store_experience(rtka_replay_buffer_t* buffer,
                             rtka_tensor_t* state,
                             rtka_value_t action,
                             rtka_confidence_t reward,
                             rtka_tensor_t* next_state,
                             bool done);
void rtka_rl_sample_batch(rtka_replay_buffer_t* buffer,
                         rtka_tensor_t** states,
                         rtka_value_t* actions,
                         rtka_confidence_t* rewards,
                         rtka_tensor_t** next_states,
                         bool* dones,
                         uint32_t batch_size);

/* Advantage computation */
rtka_confidence_t* rtka_rl_compute_advantages(rtka_confidence_t* rewards,
                                             rtka_tensor_t** states,
                                             rtka_model_t* value_network,
                                             uint32_t length,
                                             rtka_confidence_t gamma,
                                             rtka_confidence_t lambda);

#endif /* RTKA_REINFORCEMENT_H */
