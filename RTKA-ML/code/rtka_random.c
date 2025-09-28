/**
 * File: rtka_random.c
 * Copyright (c) 2025 - H.Overman opsec.ee@pm.me
 * 
 * PROPRIETARY AND CONFIDENTIAL
 * This file is proprietary to H. Overman and may not be reproduced,
 * distributed, or used without explicit written permission.
 *
 * For licensing inquiries: opsec.ee@pm.me
 *
 * RTKA Random Number Generation Implementation
 */

#include "rtka_random.h"

/* Global RNG */
rtka_rng_t g_rtka_rng = {{0x12345678, 0x87654321, 0xFEDCBA98, 0x98FEDCBA}};

/* Rotate left */
static inline uint64_t rotl(const uint64_t x, int k) {
    return (x << k) | (x >> (64 - k));
}

/* xoshiro256** algorithm */
uint64_t rtka_random_next(rtka_rng_t* rng) {
    const uint64_t result = rotl(rng->s[1] * 5, 7) * 9;
    const uint64_t t = rng->s[1] << 17;
    
    rng->s[2] ^= rng->s[0];
    rng->s[3] ^= rng->s[1];
    rng->s[1] ^= rng->s[2];
    rng->s[0] ^= rng->s[3];
    
    rng->s[2] ^= t;
    rng->s[3] = rotl(rng->s[3], 45);
    
    return result;
}

/* Initialize with splitmix64 */
void rtka_random_init_splitmix(rtka_rng_t* rng, uint64_t seed) {
    uint64_t z = seed + 0x9e3779b97f4a7c15;
    
    for (int i = 0; i < 4; i++) {
        z = (z ^ (z >> 30)) * 0xbf58476d1ce4e5b9;
        z = (z ^ (z >> 27)) * 0x94d049bb133111eb;
        rng->s[i] = z ^ (z >> 31);
    }
}

/* Get 32-bit value */
uint32_t rtka_random_uint32(rtka_rng_t* rng) {
    return (uint32_t)(rtka_random_next(rng) >> 32);
}

/* Get float [0, 1) */
float rtka_random_float(void) {
    return (rtka_random_uint32(&g_rtka_rng) >> 8) * 0x1.0p-24f;
}

/* Get double [0, 1) */
double rtka_random_double(rtka_rng_t* rng) {
    return (rtka_random_next(rng) >> 11) * 0x1.0p-53;
}

/* Random ternary value */
rtka_value_t rtka_random_ternary(rtka_rng_t* rng) {
    uint32_t r = rtka_random_uint32(rng) % 3;
    return (rtka_value_t)(r - 1);
}

/* Random state with confidence */
rtka_state_t rtka_random_state(rtka_rng_t* rng) {
    return (rtka_state_t){
        .value = rtka_random_ternary(rng),
        .confidence = (float)rtka_random_double(rng)
    };
}
