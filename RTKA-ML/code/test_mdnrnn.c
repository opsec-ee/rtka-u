/**
 * File: test_mdnrnn.c
 * Copyright (c) 2025 - H.Overman opsec.ee@pm.me
 * Email: opsec.ee@pm.me
 * 
 * RTKA MDNRNN Test Suite
 * Tests LSTM, MDN, and combined MDNRNN functionality
 * 
 * CHANGELOG:
 * v1.0.0 - 2025-10-12: Initial test implementation
 *   - LSTM forward pass tests
 *   - MDN parameter splitting tests
 *   - MDNRNN integration tests
 *   - Output verification tests
 * 
 * NOTE: This file contains ONLY test logic and output.
 * Algorithm implementations are in separate modules.
 */

#include "rtka_mdnrnn.h"
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <math.h>

/* ============================================================================
 * TEST OUTPUT FUNCTIONS
 * All printf and output generation is isolated here
 * ============================================================================ */

static void print_test_header(const char* test_name) {
    printf("\n");
    printf("========================================\n");
    printf("TEST: %s\n", test_name);
    printf("========================================\n");
}

static void print_test_result(const char* test_name, bool passed) {
    printf("[%s] %s\n", passed ? "PASS" : "FAIL", test_name);
}

static void print_tensor_shape(const char* name, rtka_tensor_t* tensor) {
    if (!tensor) {
        printf("%s: NULL\n", name);
        return;
    }
    
    printf("%s shape: (", name);
    for (uint32_t i = 0; i < tensor->ndim; i++) {
        printf("%u", tensor->shape[i]);
        if (i < tensor->ndim - 1) printf(", ");
    }
    printf(") size=%u\n", tensor->size);
}

static void print_param_count(const char* component, uint32_t count) {
    printf("%s parameters: %u\n", component, count);
}

static void print_sample_values(const char* name, rtka_tensor_t* tensor, uint32_t max_vals) {
    if (!tensor || tensor->size == 0) return;
    
    printf("%s sample values: [", name);
    uint32_t count = (tensor->size < max_vals) ? tensor->size : max_vals;
    for (uint32_t i = 0; i < count; i++) {
        printf("%.4f", tensor->data[i].confidence * (float)tensor->data[i].value);
        if (i < count - 1) printf(", ");
    }
    if (tensor->size > max_vals) printf(", ...");
    printf("]\n");
}

static void print_mdn_stats(const char* prefix, rtka_mdn_output_t* mdn_out) {
    if (!mdn_out->pi || !mdn_out->mu || !mdn_out->sigma) {
        printf("%s MDN output: INVALID\n", prefix);
        return;
    }
    
    printf("%s Pi sum check: ", prefix);
    /* Check that pi sums to ~1.0 for each batch */
    uint32_t batch = mdn_out->pi->shape[0];
    uint32_t K = mdn_out->pi->shape[1];
    
    float sum = 0.0f;
    for (uint32_t k = 0; k < K; k++) {
        uint32_t idx[] = {0, k};
        sum += mdn_out->pi->data[idx[0] * K + idx[1]].confidence;
    }
    printf("%.4f (batch 0)\n", sum);
    
    print_sample_values("  Pi", mdn_out->pi, 5);
    print_sample_values("  Mu", mdn_out->mu, 5);
    print_sample_values("  Sigma", mdn_out->sigma, 5);
}

/* ============================================================================
 * TEST HELPER FUNCTIONS
 * Pure test logic without output
 * ============================================================================ */

static bool check_tensor_not_null(rtka_tensor_t* tensor) {
    return tensor != NULL && tensor->data != NULL;
}

static bool check_shape_matches(rtka_tensor_t* tensor, uint32_t* expected, uint32_t ndim) {
    if (!tensor || tensor->ndim != ndim) return false;
    
    for (uint32_t i = 0; i < ndim; i++) {
        if (tensor->shape[i] != expected[i]) return false;
    }
    return true;
}

static bool check_value_range(rtka_tensor_t* tensor, float min_val, float max_val) {
    if (!tensor) return false;
    
    for (uint32_t i = 0; i < tensor->size; i++) {
        float val = tensor->data[i].confidence * (float)tensor->data[i].value;
        if (val < min_val || val > max_val) return false;
    }
    return true;
}

static bool check_pi_normalized(rtka_tensor_t* pi, float tolerance) {
    if (!pi || pi->ndim != 2) return false;
    
    uint32_t batch = pi->shape[0];
    uint32_t K = pi->shape[1];
    
    for (uint32_t b = 0; b < batch; b++) {
        float sum = 0.0f;
        for (uint32_t k = 0; k < K; k++) {
            sum += pi->data[b * K + k].confidence;
        }
        if (fabsf(sum - 1.0f) > tolerance) return false;
    }
    return true;
}

/* ============================================================================
 * INDIVIDUAL TESTS
 * ============================================================================ */

static bool test_lstm_creation(void) {
    print_test_header("LSTM Layer Creation");
    
    uint32_t input_size = 35;  /* z_size + action_size = 32 + 3 */
    uint32_t hidden_size = 256;
    
    rtka_lstm_layer_t* lstm = rtka_lstm_create(input_size, hidden_size, true);
    
    bool passed = (lstm != NULL) && 
                  (lstm->input_size == input_size) &&
                  (lstm->hidden_size == hidden_size);
    
    if (passed) {
        uint32_t param_count = rtka_lstm_param_count(lstm);
        print_param_count("LSTM", param_count);
        
        /* Expected: 4 * ((35 + 256) * 256 + 256) = 298,240 */
        uint32_t expected = 4 * ((input_size + hidden_size) * hidden_size + hidden_size);
        passed = (param_count == expected);
    }
    
    if (lstm) rtka_lstm_free(lstm);
    
    print_test_result("LSTM Creation", passed);
    return passed;
}

static bool test_lstm_forward(void) {
    print_test_header("LSTM Forward Pass");
    
    uint32_t batch = 2;
    uint32_t seq_len = 10;
    uint32_t input_size = 35;
    uint32_t hidden_size = 256;
    
    rtka_lstm_layer_t* lstm = rtka_lstm_create(input_size, hidden_size, true);
    if (!lstm) return false;
    
    /* Initialize hidden states */
    rtka_lstm_init_hidden(lstm, batch);
    
    /* Create input tensor */
    uint32_t input_shape[] = {batch, seq_len, input_size};
    rtka_tensor_t* input = rtka_tensor_create(input_shape, 3);
    
    /* Fill with random values */
    for (uint32_t i = 0; i < input->size; i++) {
        float val = ((float)rand() / RAND_MAX) * 2.0f - 1.0f;
        input->data[i] = rtka_make_state(
            val > 0.0f ? RTKA_TRUE : RTKA_FALSE,
            fabsf(val)
        );
    }
    
    rtka_grad_node_t* input_node = rtka_grad_node_create(input, false);
    
    /* Forward pass */
    rtka_lstm_output_t output = rtka_lstm_forward(lstm, input_node, NULL, NULL);
    
    bool passed = (output.output != NULL) &&
                  (output.hidden_state != NULL) &&
                  (output.cell_state != NULL);
    
    if (passed) {
        print_tensor_shape("LSTM output", output.output->data);
        print_tensor_shape("Hidden state", output.hidden_state);
        print_tensor_shape("Cell state", output.cell_state);
        
        uint32_t expected_out[] = {batch, seq_len, hidden_size};
        uint32_t expected_state[] = {batch, hidden_size};
        
        passed = check_shape_matches(output.output->data, expected_out, 3) &&
                 check_shape_matches(output.hidden_state, expected_state, 2) &&
                 check_shape_matches(output.cell_state, expected_state, 2);
    }
    
    /* Cleanup */
    if (output.output) rtka_grad_node_free(output.output);
    if (output.hidden_state) rtka_tensor_free(output.hidden_state);
    if (output.cell_state) rtka_tensor_free(output.cell_state);
    rtka_grad_node_free(input_node);
    rtka_lstm_free(lstm);
    
    print_test_result("LSTM Forward", passed);
    return passed;
}

static bool test_mdn_creation(void) {
    print_test_header("MDN Layer Creation");
    
    uint32_t input_size = 256;
    uint32_t output_size = 32;
    uint32_t num_gaussians = 5;
    
    rtka_mdn_layer_t* mdn = rtka_mdn_create(input_size, output_size, num_gaussians);
    
    bool passed = (mdn != NULL) &&
                  (mdn->input_size == input_size) &&
                  (mdn->output_size == output_size) &&
                  (mdn->num_gaussians == num_gaussians);
    
    if (passed) {
        uint32_t param_count = rtka_mdn_param_count(mdn);
        print_param_count("MDN", param_count);
        
        /* Expected: 256 * (5 * (1 + 2*32)) + (5 * (1 + 2*32)) = 83,200 + 325 */
        uint32_t num_params = num_gaussians * (1 + 2 * output_size);
        uint32_t expected = input_size * num_params + num_params;
        passed = (param_count == expected);
    }
    
    if (mdn) rtka_mdn_free(mdn);
    
    print_test_result("MDN Creation", passed);
    return passed;
}

static bool test_mdn_forward(void) {
    print_test_header("MDN Forward Pass");
    
    uint32_t batch = 2;
    uint32_t input_size = 256;
    uint32_t output_size = 32;
    uint32_t num_gaussians = 5;
    
    rtka_mdn_layer_t* mdn = rtka_mdn_create(input_size, output_size, num_gaussians);
    if (!mdn) return false;
    
    /* Create input */
    uint32_t input_shape[] = {batch, input_size};
    rtka_tensor_t* input = rtka_tensor_create(input_shape, 2);
    
    for (uint32_t i = 0; i < input->size; i++) {
        float val = ((float)rand() / RAND_MAX) * 2.0f - 1.0f;
        input->data[i] = rtka_make_state(
            val > 0.0f ? RTKA_TRUE : RTKA_FALSE,
            fabsf(val)
        );
    }
    
    rtka_grad_node_t* input_node = rtka_grad_node_create(input, false);
    
    /* Forward pass */
    rtka_mdn_output_t output = rtka_mdn_forward(mdn, input_node);
    
    bool passed = (output.pi != NULL) &&
                  (output.mu != NULL) &&
                  (output.sigma != NULL);
    
    if (passed) {
        print_tensor_shape("Pi", output.pi);
        print_tensor_shape("Mu", output.mu);
        print_tensor_shape("Sigma", output.sigma);
        
        uint32_t expected_pi[] = {batch, num_gaussians};
        uint32_t expected_mu[] = {batch, num_gaussians, output_size};
        uint32_t expected_sigma[] = {batch, num_gaussians, output_size};
        
        passed = check_shape_matches(output.pi, expected_pi, 2) &&
                 check_shape_matches(output.mu, expected_mu, 3) &&
                 check_shape_matches(output.sigma, expected_sigma, 3);
        
        if (passed) {
            /* Check pi is normalized */
            passed = check_pi_normalized(output.pi, 0.01f);
            print_mdn_stats("MDN", &output);
        }
    }
    
    /* Cleanup */
    rtka_mdn_output_free(&output);
    rtka_grad_node_free(input_node);
    rtka_mdn_free(mdn);
    
    print_test_result("MDN Forward", passed);
    return passed;
}

static bool test_mdnrnn_creation(void) {
    print_test_header("MDNRNN Model Creation");
    
    uint32_t z_size = 32;
    uint32_t action_size = 3;
    uint32_t hidden_size = 256;
    uint32_t num_gaussians = 5;
    
    rtka_mdnrnn_t* model = rtka_mdnrnn_create(z_size, action_size, 
                                              hidden_size, num_gaussians);
    
    bool passed = (model != NULL) &&
                  (model->z_size == z_size) &&
                  (model->action_size == action_size) &&
                  (model->hidden_size == hidden_size) &&
                  (model->num_gaussians == num_gaussians);
    
    if (passed) {
        uint32_t total_params = rtka_mdnrnn_param_count(model);
        print_param_count("MDNRNN total", total_params);
        
        /* Expected: LSTM (298,240) + MDN (83,525) = 381,765 */
        passed = (total_params > 380000 && total_params < 385000);
    }
    
    if (model) rtka_mdnrnn_free(model);
    
    print_test_result("MDNRNN Creation", passed);
    return passed;
}

static bool test_mdnrnn_forward(void) {
    print_test_header("MDNRNN Forward Pass");
    
    uint32_t batch = 2;
    uint32_t seq_len = 10;
    uint32_t z_size = 32;
    uint32_t action_size = 3;
    uint32_t hidden_size = 256;
    uint32_t num_gaussians = 5;
    
    rtka_mdnrnn_t* model = rtka_mdnrnn_create(z_size, action_size,
                                              hidden_size, num_gaussians);
    if (!model) return false;
    
    rtka_mdnrnn_init_hidden(model, batch);
    
    /* Create inputs */
    uint32_t z_shape[] = {batch, seq_len, z_size};
    uint32_t a_shape[] = {batch, seq_len, action_size};
    
    rtka_tensor_t* z = rtka_tensor_create(z_shape, 3);
    rtka_tensor_t* actions = rtka_tensor_create(a_shape, 3);
    
    /* Fill with random data */
    for (uint32_t i = 0; i < z->size; i++) {
        float val = ((float)rand() / RAND_MAX) * 2.0f - 1.0f;
        z->data[i] = rtka_make_state(
            val > 0.0f ? RTKA_TRUE : RTKA_FALSE,
            fabsf(val)
        );
    }
    
    for (uint32_t i = 0; i < actions->size; i++) {
        float val = ((float)rand() / RAND_MAX) * 2.0f - 1.0f;
        actions->data[i] = rtka_make_state(
            val > 0.0f ? RTKA_TRUE : RTKA_FALSE,
            fabsf(val)
        );
    }
    
    /* Forward pass */
    rtka_mdnrnn_output_t output = rtka_mdnrnn_forward(model, z, actions, NULL, NULL);
    
    bool passed = (output.mdn_params.pi != NULL) &&
                  (output.mdn_params.mu != NULL) &&
                  (output.mdn_params.sigma != NULL) &&
                  (output.hidden_state != NULL) &&
                  (output.cell_state != NULL);
    
    if (passed) {
        printf("MDNRNN output shapes:\n");
        print_tensor_shape("  Pi", output.mdn_params.pi);
        print_tensor_shape("  Mu", output.mdn_params.mu);
        print_tensor_shape("  Sigma", output.mdn_params.sigma);
        print_tensor_shape("  Hidden", output.hidden_state);
        print_tensor_shape("  Cell", output.cell_state);
        
        print_mdn_stats("MDNRNN", &output.mdn_params);
        
        /* Sample from output */
        rtka_tensor_t* sample = rtka_mdnrnn_sample_next(&output, 0);
        if (sample) {
            print_sample_values("Sampled z_next", sample, 10);
            rtka_tensor_free(sample);
        }
    }
    
    /* Cleanup */
    rtka_mdnrnn_output_free(&output);
    rtka_tensor_free(z);
    rtka_tensor_free(actions);
    rtka_mdnrnn_free(model);
    
    print_test_result("MDNRNN Forward", passed);
    return passed;
}

static bool test_mdnrnn_step(void) {
    print_test_header("MDNRNN Single Step");
    
    uint32_t batch = 1;
    uint32_t z_size = 32;
    uint32_t action_size = 3;
    
    rtka_mdnrnn_t* model = rtka_mdnrnn_create(z_size, action_size, 256, 5);
    if (!model) return false;
    
    rtka_mdnrnn_init_hidden(model, batch);
    
    /* Create single timestep inputs */
    uint32_t input_shape[] = {batch, z_size};
    uint32_t action_shape[] = {batch, action_size};
    
    rtka_tensor_t* z_t = rtka_tensor_create(input_shape, 2);
    rtka_tensor_t* a_t = rtka_tensor_create(action_shape, 2);
    
    for (uint32_t i = 0; i < z_t->size; i++) {
        z_t->data[i] = rtka_make_state(RTKA_TRUE, 0.5f);
    }
    for (uint32_t i = 0; i < a_t->size; i++) {
        a_t->data[i] = rtka_make_state(RTKA_TRUE, 0.3f);
    }
    
    /* Single step */
    rtka_mdn_output_t output = rtka_mdnrnn_step(model, z_t, a_t);
    
    bool passed = (output.pi != NULL) &&
                  (output.mu != NULL) &&
                  (output.sigma != NULL);
    
    if (passed) {
        printf("Step output:\n");
        print_tensor_shape("  Pi", output.pi);
        print_mdn_stats("Step", &output);
    }
    
    /* Cleanup */
    rtka_mdn_output_free(&output);
    rtka_tensor_free(z_t);
    rtka_tensor_free(a_t);
    rtka_mdnrnn_free(model);
    
    print_test_result("MDNRNN Single Step", passed);
    return passed;
}

/* ============================================================================
 * MAIN TEST RUNNER
 * ============================================================================ */

int main(void) {
    srand((unsigned int)time(NULL));
    
    printf("\n");
    printf("================================================================================\n");
    printf("RTKA MDNRNN Test Suite\n");
    printf("Copyright (c) 2025 - H.Overman opsec.ee@pm.me\n");
    printf("Email: opsec.ee@pm.me\n");
    printf("================================================================================\n");
    
    int passed = 0;
    int total = 0;
    
    /* Run all tests */
    total++; if (test_lstm_creation()) passed++;
    total++; if (test_lstm_forward()) passed++;
    total++; if (test_mdn_creation()) passed++;
    total++; if (test_mdn_forward()) passed++;
    total++; if (test_mdnrnn_creation()) passed++;
    total++; if (test_mdnrnn_forward()) passed++;
    total++; if (test_mdnrnn_step()) passed++;
    
    /* Final results */
    printf("\n");
    printf("================================================================================\n");
    printf("TEST RESULTS: %d/%d passed (%.1f%%)\n", 
           passed, total, (float)passed / total * 100.0f);
    printf("================================================================================\n");
    
    return (passed == total) ? 0 : 1;
}
