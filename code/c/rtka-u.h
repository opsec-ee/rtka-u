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

/* Domain: {FALSE=-1, UNKNOWN=0, TRUE=1} */
typedef int8_t rtka_ternary_t;
typedef double rtka_confidence_t;

#define RTKA_FALSE    ((rtka_ternary_t)-1)
#define RTKA_UNKNOWN  ((rtka_ternary_t)0)
#define RTKA_TRUE     ((rtka_ternary_t)1)

#define RTKA_MAX_UNKNOWN_LEVELS 8

/* Result structure */
typedef struct {
    rtka_ternary_t value;
    rtka_confidence_t confidence;
    size_t operations_performed;
    bool early_terminated;
} rtka_result_t;

/* Core operations */
rtka_ternary_t rtka_and(rtka_ternary_t a, rtka_ternary_t b);
rtka_ternary_t rtka_or(rtka_ternary_t a, rtka_ternary_t b);
rtka_ternary_t rtka_not(rtka_ternary_t a);
rtka_ternary_t rtka_eqv(rtka_ternary_t a, rtka_ternary_t b);

/* Recursive evaluation */
rtka_result_t rtka_eval_and(const rtka_ternary_t* inputs, const rtka_confidence_t* confidences, size_t count);
rtka_result_t rtka_eval_or(const rtka_ternary_t* inputs, const rtka_confidence_t* confidences, size_t count);

/* Utility functions */
const char* rtka_to_string(rtka_ternary_t value);
bool rtka_is_valid(rtka_ternary_t value);

#endif /* RTKA_U_H */
