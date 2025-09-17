/**
 * rtka-u.c
 * Recursive Ternary with Kleene Algorithm + UNKNOWN
 * Implementation in C
 *
 * Author: H.Overman <opsec.ee@pm.me>
 */

#include "rtka-u.h"
#include <math.h>
#include <stdlib.h>
#include <assert.h>
#include <string.h>  // For memcpy

#define UNLIKELY(x) __builtin_expect(!!(x), 0)

// Branch-free min/max for Kleene ops
static inline rtka_ternary_t min_branchfree(rtka_ternary_t a, rtka_ternary_t b) {
  return (a < b) ? a : b;
}

static inline rtka_ternary_t max_branchfree(rtka_ternary_t a, rtka_ternary_t b) {
  return (a > b) ? a : b;
}

rtka_ternary_t rtka_and(rtka_ternary_t a, rtka_ternary_t b) {
  return min_branchfree(a, b);
}

rtka_ternary_t rtka_or(rtka_ternary_t a, rtka_ternary_t b) {
  return max_branchfree(a, b);
}

rtka_ternary_t rtka_not(rtka_ternary_t a) {
  return -a;
}

rtka_ternary_t rtka_eqv(rtka_ternary_t a, rtka_ternary_t b) {
  return a * b;
}

// Closed-form confidence calculations (integrated inline for performance)

static rtka_confidence_t confidence_eqv(const rtka_confidence_t* confidences, size_t count, size_t known_pairs) {
  rtka_confidence_t prod = 1.0;
  for (size_t i = 0; i < count; i++) {
    prod *= confidences[i];
  }
  // Explicit cast to avoid conversion warnings
  rtka_confidence_t geom_mean = pow(prod, 2.0 / (double)count);
  rtka_confidence_t weight = (double)known_pairs / ((double)count * ((double)count - 1.0) / 2.0);
  return geom_mean * weight;
}

// Early termination in recursive evaluation
// Progressive validation (inputs, count)
rtka_result_t rtka_eval_and(const rtka_ternary_t* inputs,
                            const rtka_confidence_t* confidences,
                            size_t count,
                            const rtka_threshold_t* threshold) {
  rtka_result_t result = {RTKA_UNKNOWN, 1.0, 0, false, 0};
  if (UNLIKELY(count == 0 || !inputs)) {
    result.error_code = RTKA_ERROR_INVALID_INPUT;
    result.confidence = 0.0;
    return result;
  }

  size_t unknown_count = 0;
  for (size_t i = 0; i < count; i++) {
    if (UNLIKELY(!rtka_is_valid(inputs[i]))) {
      result.error_code = RTKA_ERROR_INVALID_VALUE;
      result.confidence = 0.0;
      return result;
    }
    if (inputs[i] == RTKA_UNKNOWN) unknown_count++;
  }

  if (unknown_count >= RTKA_MAX_UNKNOWN_LEVELS) {
    result.value = RTKA_FALSE;
    result.confidence = 0.0;
    result.error_code = RTKA_ERROR_TOO_MANY_UNKNOWN;
    return result;
  }

  rtka_ternary_t accumulator = inputs[0];
  rtka_confidence_t conf_acc = confidences ? confidences[0] : 1.0;

  for (size_t i = 1; i < count; i++) {
    result.operations_performed++;
    accumulator = rtka_and(accumulator, inputs[i]);
    if (confidences) conf_acc *= confidences[i];

    if (accumulator == RTKA_FALSE) {
      result.early_terminated = true;
      break;
    }
  }

  result.value = accumulator;
  result.confidence = conf_acc;

  if (threshold && result.confidence < fmax(threshold->epsilon, threshold->c0 * pow(threshold->alpha_and, (double)count))) {
    result.confidence = 0.0;
    result.value = RTKA_UNKNOWN;
  }

  return result;
}

rtka_result_t rtka_eval_or(const rtka_ternary_t* inputs,
                           const rtka_confidence_t* confidences,
                           size_t count,
                           const rtka_threshold_t* threshold) {
  rtka_result_t result = {RTKA_UNKNOWN, 1.0, 0, false, 0};
  if (UNLIKELY(count == 0 || !inputs)) {
    result.error_code = RTKA_ERROR_INVALID_INPUT;
    result.confidence = 0.0;
    return result;
  }

  size_t unknown_count = 0;
  for (size_t i = 0; i < count; i++) {
    if (UNLIKELY(!rtka_is_valid(inputs[i]))) {
      result.error_code = RTKA_ERROR_INVALID_VALUE;
      result.confidence = 0.0;
      return result;
    }
    if (inputs[i] == RTKA_UNKNOWN) unknown_count++;
  }

  if (unknown_count >= RTKA_MAX_UNKNOWN_LEVELS) {
    result.value = RTKA_FALSE;
    result.confidence = 0.0;
    result.error_code = RTKA_ERROR_TOO_MANY_UNKNOWN;
    return result;
  }

  rtka_ternary_t accumulator = inputs[0];
  rtka_confidence_t conf_acc = confidences ? confidences[0] : 1.0;

  for (size_t i = 1; i < count; i++) {
    result.operations_performed++;
    accumulator = rtka_or(accumulator, inputs[i]);
    if (confidences) {
      // OR confidence formula: 1 - product(1 - confidence_i)
      conf_acc = 1.0 - ((1.0 - conf_acc) * (1.0 - confidences[i]));
    }

    if (accumulator == RTKA_TRUE) {
      result.early_terminated = true;
      break;
    }
  }

  result.value = accumulator;
  result.confidence = conf_acc;

  if (threshold && result.confidence < fmax(threshold->epsilon, threshold->c0 * pow(threshold->alpha_or, (double)count))) {
    result.confidence = 0.0;
    result.value = RTKA_UNKNOWN;
  }

  return result;
}

rtka_result_t rtka_eval_eqv(const rtka_ternary_t* inputs,
                            const rtka_confidence_t* confidences,
                            size_t count,
                            const rtka_threshold_t* threshold) {
  rtka_result_t result = {RTKA_UNKNOWN, 1.0, 0, false, 0};
  if (UNLIKELY(count == 0 || !inputs)) {
    result.error_code = RTKA_ERROR_INVALID_INPUT;
    result.confidence = 0.0;
    return result;
  }

  size_t unknown_count = 0;
  size_t known_pairs = 0;
  int64_t sum = 0;
  for (size_t i = 0; i < count; i++) {
    if (UNLIKELY(!rtka_is_valid(inputs[i]))) {
      result.error_code = RTKA_ERROR_INVALID_VALUE;
      result.confidence = 0.0;
      return result;
    }
    if (inputs[i] == RTKA_UNKNOWN) unknown_count++;

    for (size_t j = i + 1; j < count; j++) {
      result.operations_performed++;
      sum += rtka_eqv(inputs[i], inputs[j]);
      if (inputs[i] != RTKA_UNKNOWN && inputs[j] != RTKA_UNKNOWN) known_pairs++;
    }
  }

  if (unknown_count >= RTKA_MAX_UNKNOWN_LEVELS) {
    result.value = RTKA_FALSE;
    result.confidence = 0.0;
    result.error_code = RTKA_ERROR_TOO_MANY_UNKNOWN;
    return result;
  }

  result.value = (sum > 0) ? RTKA_TRUE : (sum < 0) ? RTKA_FALSE : RTKA_UNKNOWN;

  if (confidences) {
    result.confidence = confidence_eqv(confidences, count, known_pairs);
  }

  if (threshold && result.confidence < fmax(threshold->epsilon, threshold->c0 * pow(threshold->alpha_eqv, (double)count))) {
    result.confidence = 0.0;
    result.value = RTKA_UNKNOWN;
  }

  return result;
}

rtka_result_t rtka_eval_node(const rtka_node_t* node, const rtka_threshold_t* threshold) {
  rtka_result_t result = {RTKA_UNKNOWN, 1.0, 0, false, 0};
  if (!node) return result;

  if (node->child_count == 0) {
    result.value = node->value;
    result.confidence = node->confidence;
    return result;
  }

  rtka_ternary_t* child_values = malloc(node->child_count * sizeof(rtka_ternary_t));
  rtka_confidence_t* child_confidences = malloc(node->child_count * sizeof(rtka_confidence_t));

  for (size_t i = 0; i < node->child_count; i++) {
    rtka_result_t child_result = rtka_eval_node(node->children[i], threshold);
    child_values[i] = child_result.value;
    child_confidences[i] = child_result.confidence;
    result.operations_performed += child_result.operations_performed;
    result.early_terminated |= child_result.early_terminated;
  }

  switch (node->op) {
    case RTKA_AND:
      result = rtka_eval_and(child_values, child_confidences, node->child_count, threshold);
      break;
    case RTKA_OR:
      result = rtka_eval_or(child_values, child_confidences, node->child_count, threshold);
      break;
    case RTKA_EQV:
      result = rtka_eval_eqv(child_values, child_confidences, node->child_count, threshold);
      break;
    case RTKA_NOT:
      if (node->child_count == 1) {
        result.value = rtka_not(child_values[0]);
        result.confidence = child_confidences[0];
      }
      break;
    default:
      // Default case for unknown operations
      result.error_code = RTKA_ERROR_INVALID_VALUE;
      result.confidence = 0.0;
      break;
  }

  free(child_values);
  free(child_confidences);

  return result;
}

const char* rtka_to_string(rtka_ternary_t value) {
  switch (value) {
    case RTKA_FALSE: return "FALSE";
    case RTKA_UNKNOWN: return "UNKNOWN";
    case RTKA_TRUE: return "TRUE";
    default: return "INVALID";
  }
}

bool rtka_is_valid(rtka_ternary_t value) {
  return value == RTKA_FALSE || value == RTKA_UNKNOWN || value == RTKA_TRUE;
}

rtka_node_t* rtka_create_node(rtka_ternary_t value, rtka_confidence_t confidence,
                              rtka_op_t op, rtka_node_t** children, size_t child_count) {
  rtka_node_t* node = malloc(sizeof(rtka_node_t));
  node->value = value;
  node->confidence = confidence;
  node->op = op;
  node->child_count = child_count;
  node->is_leaf = (child_count == 0);
  if (child_count > 0) {
    node->children = malloc(child_count * sizeof(rtka_node_t*));
    memcpy(node->children, children, child_count * sizeof(rtka_node_t*));
  } else {
    node->children = NULL;
  }
  return node;
}

void rtka_free_tree(rtka_node_t* node) {
  if (!node) return;
  for (size_t i = 0; i < node->child_count; i++) {
    rtka_free_tree(node->children[i]);
  }
  free(node->children);
  free(node);
}
