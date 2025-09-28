/**
 * File: rtka_random.h
 * Copyright (c) 2025 - H.Overman opsec.ee@pm.me
 * 
 * PROPRIETARY AND CONFIDENTIAL
 * This file is proprietary to H. Overman and may not be reproduced,
 * distributed, or used without explicit written permission.
 *
 * For licensing inquiries: opsec.ee@pm.me
 *
 * RTKA Random Number Generation - xoshiro256**
 */

#ifndef RTKA_RANDOM_H
#define RTKA_RANDOM_H

#include "rtka_types.h"

/* xoshiro256** state */
typedef struct {
    uint64_t s[4];
} rtka_rng_t;

/* Initialize with seed */
void rtka_random_init(rtka_rng_t* rng, uint64_t seed);
void rtka_random_init_splitmix(rtka_rng_t* rng, uint64_t seed);

/* Core xoshiro256** */
uint64_t rtka_random_next(rtka_rng_t* rng);

/* Derived generators */
uint32_t rtka_random_uint32(rtka_rng_t* rng);
float rtka_random_float(void);
double rtka_random_double(rtka_rng_t* rng);

/* Range generators */
uint32_t rtka_random_range(rtka_rng_t* rng, uint32_t min, uint32_t max);
rtka_value_t rtka_random_ternary(rtka_rng_t* rng);
rtka_state_t rtka_random_state(rtka_rng_t* rng);

/* Global RNG */
extern rtka_rng_t g_rtka_rng;

#endif /* RTKA_RANDOM_H */
