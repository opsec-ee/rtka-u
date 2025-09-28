/**
 * File: rtka_reinforcement.c
 * Copyright (c) 2025 - H.Overman opsec.ee@pm.me
 * 
 * PROPRIETARY AND CONFIDENTIAL
 * This file is proprietary to H. Overman and may not be reproduced,
 * distributed, or used without explicit written permission.
 *
 * For licensing inquiries: opsec.ee@pm.me
 *
 * RTKA Reinforcement Learning Implementation
 */

#include "rtka_reinforcement.h"
#include "rtka_ml.h"
#include "rtka_memory.h"
#include <string.h>
#include <stdlib.h>
#include <math.h>

/* Create DQN agent */
rtka_dqn_agent_t* rtka_rl_create_dqn(uint32_t state_dim, uint32_t action_dim,
                                     rtka_confidence_t epsilon, rtka_confidence_t gamma) {
    rtka_dqn_agent_t* agent = (rtka_dqn_agent_t*)rtka_alloc_state();
    if (!agent) return NULL;
    
    /* Q-network: state -> Q(s,a) for each action */
    uint32_t hidden[] = {128, 64};
    agent->q_network = rtka_ml_create_mlp(state_dim, hidden, 2, action_dim);
    agent->target_network = rtka_ml_create_mlp(state_dim, hidden, 2, action_dim);
    
    /* Initialize with Adam optimizer */
    agent->q_network->optimizer = rtka_optimizer_adam(0.001f, 0.9f, 0.999f);
    rtka_ml_compile(agent->q_network, agent->q_network->optimizer);
    
    agent->buffer = rtka_rl_create_buffer(10000);
    agent->epsilon = epsilon;
    agent->gamma = gamma;
    agent->update_frequency = 100;
    
    return agent;
}

/* Create Policy Gradient agent */
rtka_pg_agent_t* rtka_rl_create_pg(uint32_t state_dim, uint32_t action_dim) {
    rtka_pg_agent_t* agent = (rtka_pg_agent_t*)rtka_alloc_state();
    if (!agent) return NULL;
    
    /* Policy network: state -> action probabilities */
    agent->policy_network = rtka_ml_create_ternary_classifier(state_dim, action_dim, 0.5f);
    rtka_ml_compile(agent->policy_network, rtka_optimizer_adam(0.001f, 0.9f, 0.999f));
    
    /* Value network for baseline */
    uint32_t hidden[] = {64, 32};
    agent->value_network = rtka_ml_create_mlp(state_dim, hidden, 2, 1);
    rtka_ml_compile(agent->value_network, rtka_optimizer_adam(0.001f, 0.9f, 0.999f));
    
    agent->trajectory_length = 0;
    
    return agent;
}

/* Create replay buffer */
rtka_replay_buffer_t* rtka_rl_create_buffer(uint32_t capacity) {
    rtka_replay_buffer_t* buffer = (rtka_replay_buffer_t*)rtka_alloc_state();
    if (!buffer) return NULL;
    
    buffer->capacity = capacity;
    buffer->size = 0;
    buffer->position = 0;
    
    buffer->states = (rtka_tensor_t**)rtka_alloc_states(capacity * sizeof(void*) / sizeof(rtka_state_t));
    buffer->actions = (rtka_value_t*)rtka_alloc_states(capacity * sizeof(rtka_value_t) / sizeof(rtka_state_t));
    buffer->rewards = (rtka_confidence_t*)rtka_alloc_states(capacity * sizeof(rtka_confidence_t) / sizeof(rtka_state_t));
    buffer->next_states = (rtka_tensor_t**)rtka_alloc_states(capacity * sizeof(void*) / sizeof(rtka_state_t));
    buffer->dones = (bool*)rtka_alloc_states(capacity * sizeof(bool) / sizeof(rtka_state_t));
    
    return buffer;
}

/* Store experience */
void rtka_rl_store_experience(rtka_replay_buffer_t* buffer,
                             rtka_tensor_t* state,
                             rtka_value_t action,
                             rtka_confidence_t reward,
                             rtka_tensor_t* next_state,
                             bool done) {
    buffer->states[buffer->position] = state;
    buffer->actions[buffer->position] = action;
    buffer->rewards[buffer->position] = reward;
    buffer->next_states[buffer->position] = next_state;
    buffer->dones[buffer->position] = done;
    
    buffer->position = (buffer->position + 1) % buffer->capacity;
    if (buffer->size < buffer->capacity) buffer->size++;
}

/* Sample batch from buffer */
void rtka_rl_sample_batch(rtka_replay_buffer_t* buffer,
                         rtka_tensor_t** states,
                         rtka_value_t* actions,
                         rtka_confidence_t* rewards,
                         rtka_tensor_t** next_states,
                         bool* dones,
                         uint32_t batch_size) {
    for (uint32_t i = 0; i < batch_size; i++) {
        uint32_t idx = rand() % buffer->size;
        states[i] = buffer->states[idx];
        actions[i] = buffer->actions[idx];
        rewards[i] = buffer->rewards[idx];
        next_states[i] = buffer->next_states[idx];
        dones[i] = buffer->dones[idx];
    }
}

/* Epsilon-greedy action selection */
rtka_value_t rtka_rl_select_action_epsilon_greedy(rtka_dqn_agent_t* agent, rtka_tensor_t* state) {
    if ((float)rand() / RAND_MAX < agent->epsilon) {
        /* Random action with ternary values */
        float r = (float)rand() / RAND_MAX;
        if (r < 0.333f) return RTKA_FALSE;
        else if (r < 0.667f) return RTKA_UNKNOWN;
        else return RTKA_TRUE;
    }
    
    /* Greedy action from Q-network */
    rtka_tensor_t* q_values = rtka_ml_predict(agent->q_network, state);
    
    rtka_value_t best_action = RTKA_UNKNOWN;
    rtka_confidence_t best_value = -1000.0f;
    
    for (uint32_t a = 0; a < q_values->shape[0]; a++) {
        if (q_values->data[a].confidence > best_value) {
            best_value = q_values->data[a].confidence;
            best_action = q_values->data[a].value;
        }
    }
    
    rtka_tensor_free(q_values);
    return best_action;
}

/* Policy-based action selection */
rtka_value_t rtka_rl_select_action_policy(rtka_pg_agent_t* agent, rtka_tensor_t* state) {
    rtka_tensor_t* action_probs = rtka_ml_predict(agent->policy_network, state);
    
    /* Sample from ternary distribution */
    rtka_confidence_t total = 0.0f;
    for (uint32_t i = 0; i < action_probs->size; i++) {
        total += action_probs->data[i].confidence;
    }
    
    float r = (float)rand() / RAND_MAX * total;
    rtka_confidence_t cumsum = 0.0f;
    
    for (uint32_t i = 0; i < action_probs->size; i++) {
        cumsum += action_probs->data[i].confidence;
        if (r <= cumsum) {
            rtka_value_t action = action_probs->data[i].value;
            rtka_tensor_free(action_probs);
            return action;
        }
    }
    
    rtka_tensor_free(action_probs);
    return RTKA_UNKNOWN;
}

/* Train DQN */
void rtka_rl_train_dqn(rtka_dqn_agent_t* agent, rtka_environment_t* env, uint32_t episodes) {
    uint32_t batch_size = 32;
    uint32_t total_steps = 0;
    
    for (uint32_t episode = 0; episode < episodes; episode++) {
        rtka_tensor_t* state = env->reset(env->env_data);
        rtka_confidence_t episode_reward = 0.0f;
        bool done = false;
        
        while (!done) {
            /* Select action */
            rtka_value_t action = rtka_rl_select_action_epsilon_greedy(agent, state);
            
            /* Take action */
            rtka_confidence_t reward;
            rtka_tensor_t* next_state = env->step(env->env_data, action, &reward, &done);
            
            /* Store experience */
            rtka_rl_store_experience(agent->buffer, state, action, reward, next_state, done);
            
            /* Train if enough experiences */
            if (agent->buffer->size >= batch_size) {
                /* Sample batch */
                rtka_tensor_t** batch_states = (rtka_tensor_t**)rtka_alloc_states(batch_size * sizeof(void*) / sizeof(rtka_state_t));
                rtka_value_t* batch_actions = (rtka_value_t*)rtka_alloc_states(batch_size * sizeof(rtka_value_t) / sizeof(rtka_state_t));
                rtka_confidence_t* batch_rewards = (rtka_confidence_t*)rtka_alloc_states(batch_size * sizeof(rtka_confidence_t) / sizeof(rtka_state_t));
                rtka_tensor_t** batch_next_states = (rtka_tensor_t**)rtka_alloc_states(batch_size * sizeof(void*) / sizeof(rtka_state_t));
                bool* batch_dones = (bool*)rtka_alloc_states(batch_size * sizeof(bool) / sizeof(rtka_state_t));
                
                rtka_rl_sample_batch(agent->buffer, batch_states, batch_actions, 
                                   batch_rewards, batch_next_states, batch_dones, batch_size);
                
                /* Compute Q-targets */
                for (uint32_t i = 0; i < batch_size; i++) {
                    rtka_tensor_t* next_q = rtka_ml_predict(agent->target_network, batch_next_states[i]);
                    
                    rtka_confidence_t max_next_q = 0.0f;
                    for (uint32_t a = 0; a < next_q->shape[0]; a++) {
                        if (next_q->data[a].confidence > max_next_q) {
                            max_next_q = next_q->data[a].confidence;
                        }
                    }
                    
                    rtka_confidence_t target = batch_rewards[i];
                    if (!batch_dones[i]) {
                        target += agent->gamma * max_next_q;
                    }
                    
                    /* Update Q-network (simplified) */
                    rtka_tensor_free(next_q);
                }
            }
            
            state = next_state;
            episode_reward += reward;
            total_steps++;
            
            /* Update target network */
            if (total_steps % agent->update_frequency == 0) {
                /* Copy weights from Q to target (simplified) */
                for (uint32_t i = 0; i < agent->q_network->network->num_layers; i++) {
                    rtka_layer_t* q_layer = agent->q_network->network->layers[i];
                    rtka_layer_t* target_layer = agent->target_network->network->layers[i];
                    
                    if (q_layer->weight && target_layer->weight) {
                        memcpy(target_layer->weight->data->data, 
                              q_layer->weight->data->data,
                              q_layer->weight->data->size * sizeof(rtka_state_t));
                    }
                }
            }
        }
        
        /* Decay epsilon */
        agent->epsilon *= 0.995f;
        if (agent->epsilon < 0.01f) agent->epsilon = 0.01f;
    }
}

/* Compute advantages */
rtka_confidence_t* rtka_rl_compute_advantages(rtka_confidence_t* rewards,
                                             rtka_tensor_t** states,
                                             rtka_model_t* value_network,
                                             uint32_t length,
                                             rtka_confidence_t gamma,
                                             rtka_confidence_t lambda) {
    rtka_confidence_t* advantages = (rtka_confidence_t*)rtka_alloc_states(
        length * sizeof(rtka_confidence_t) / sizeof(rtka_state_t));
    rtka_confidence_t* returns = (rtka_confidence_t*)rtka_alloc_states(
        length * sizeof(rtka_confidence_t) / sizeof(rtka_state_t));
    
    /* Compute returns with GAE */
    rtka_confidence_t gae = 0.0f;
    
    for (int32_t t = length - 1; t >= 0; t--) {
        rtka_tensor_t* value = rtka_ml_predict(value_network, states[t]);
        rtka_confidence_t v_t = value->data[0].confidence;
        rtka_tensor_free(value);
        
        rtka_confidence_t v_next = 0.0f;
        if (t < (int32_t)length - 1) {
            rtka_tensor_t* next_value = rtka_ml_predict(value_network, states[t + 1]);
            v_next = next_value->data[0].confidence;
            rtka_tensor_free(next_value);
        }
        
        rtka_confidence_t delta = rewards[t] + gamma * v_next - v_t;
        gae = delta + gamma * lambda * gae;
        advantages[t] = gae;
        returns[t] = advantages[t] + v_t;
    }
    
    return advantages;
}
