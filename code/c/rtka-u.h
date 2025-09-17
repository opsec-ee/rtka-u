/**
 * rtka-u.h
 * Recursive Ternary with Kleene Algorithm + UNKNOWN
 * Author: H.Overman <opsec.ee@pm.me>
 */

#ifndef RTKA_U_H
#define RTKA_U_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

// C23 compatibility
#if __STDC_VERSION__ >= 201112L
    _Static_assert(sizeof(int8_t) == 1, "int8_t must be 1 byte");
    _Static_assert(sizeof(double) == 8, "double must be 8 bytes");
#endif

// C23 nullptr compatibility
#if __STDC_VERSION__ >= 202311L
    // C23 has nullptr built-in
#elif __STDC_VERSION__ >= 201112L
    #define nullptr ((void*)0)
#endif

#define RTKA_FALSE    ((rtka_ternary_t)-1)
#define RTKA_UNKNOWN  ((rtka_ternary_t)0)
#define RTKA_TRUE     ((rtka_ternary_t)1)

#define RTKA_MAX_UNKNOWN_LEVELS 8

// Error codes
#define RTKA_ERROR_INVALID_INPUT 1
#define RTKA_ERROR_INVALID_VALUE 2
#define RTKA_ERROR_TOO_MANY_UNKNOWN 3

typedef int8_t rtka_ternary_t;
typedef double rtka_confidence_t;

typedef struct {
  double epsilon;     // Minimum confidence threshold
  double c0;          // Base confidence factor
  double alpha_and;   // Decay factor for AND
  double alpha_or;    // Decay factor for OR
  double alpha_eqv;   // Decay factor for EQV
} rtka_threshold_t;

typedef enum {
  RTKA_AND,
  RTKA_OR,
  RTKA_EQV,
  RTKA_NOT
} rtka_op_t;

typedef struct rtka_node {
  rtka_ternary_t value;
  rtka_confidence_t confidence;
  rtka_op_t op;
  struct rtka_node** children;
  size_t child_count;
  bool is_leaf;  // Added for error fix
} rtka_node_t;

typedef struct {
  rtka_ternary_t value;
  rtka_confidence_t confidence;
  size_t operations_performed;
  bool early_terminated;
  int error_code;  // Integrated error info
} rtka_result_t;

// Core operations
rtka_ternary_t rtka_and(rtka_ternary_t a, rtka_ternary_t b);
rtka_ternary_t rtka_or(rtka_ternary_t a, rtka_ternary_t b);
rtka_ternary_t rtka_not(rtka_ternary_t a);
rtka_ternary_t rtka_eqv(rtka_ternary_t a, rtka_ternary_t b);

// Recursive evaluation
rtka_result_t rtka_eval_and(const rtka_ternary_t* inputs,
                            const rtka_confidence_t* confidences,
                            size_t count,
                            const rtka_threshold_t* threshold);

rtka_result_t rtka_eval_or(const rtka_ternary_t* inputs,
                           const rtka_confidence_t* confidences,
                           size_t count,
                           const rtka_threshold_t* threshold);

rtka_result_t rtka_eval_eqv(const rtka_ternary_t* inputs,
                            const rtka_confidence_t* confidences,
                            size_t count,
                            const rtka_threshold_t* threshold);

// Tree evaluation
rtka_result_t rtka_eval_node(const rtka_node_t* node,
                             const rtka_threshold_t* threshold);

// Utility functions
const char* rtka_to_string(rtka_ternary_t value);
bool rtka_is_valid(rtka_ternary_t value);

// Tree construction
rtka_node_t* rtka_create_node(rtka_ternary_t value, rtka_confidence_t
confidence,
rtka_op_t op, rtka_node_t** children, size_t
child_count);
void rtka_free_tree(rtka_node_t* node);

#endif /* RTKA_U_H */

