/**
 * File: rtka_constants.h
 * Copyright (c) 2025 - H.Overman opsec.ee@pm.me
 * 
 * PROPRIETARY AND CONFIDENTIAL
 * This file is proprietary to H. Overman and may not be reproduced,
 * distributed, or used without explicit written permission.
 *
 * RTKA System Constants
 * Performance-tuned configuration values
 */

#ifndef RTKA_CONSTANTS_H
#define RTKA_CONSTANTS_H

/* Cache optimization */
#define RTKA_CACHE_LINE_SIZE 64U
#define RTKA_L1_SIZE (32U * 1024U)
#define RTKA_L2_SIZE (256U * 1024U)

/* SIMD alignment */
#define RTKA_SIMD_ALIGNMENT 32U  /* AVX2 */
#define RTKA_VECTOR_WIDTH 8U      /* 8 floats in AVX */

/* Core limits */
#define RTKA_MAX_FACTORS 64U
#define RTKA_MAX_RECURSION_DEPTH 16U
#define RTKA_MAX_VECTOR_SIZE 1024U

/* Memory pools - sized for L1/L2 cache efficiency */
#define RTKA_STATE_POOL_SIZE 4096U
#define RTKA_VECTOR_POOL_SIZE 256U
#define RTKA_TEMP_STACK_SIZE (64U * 1024U)

/* Precision */
#define RTKA_CONFIDENCE_EPSILON 1e-6f
#define RTKA_ZERO_THRESHOLD 1e-9f

/* Optimization thresholds */
#define RTKA_SIMD_THRESHOLD 32U        /* Min elements for SIMD */
#define RTKA_PARALLEL_THRESHOLD 1024U  /* Min for threading */
#define RTKA_LOOKUP_TABLE_SIZE 256U    /* Precomputed operations */

/* Algorithm tuning */
#define RTKA_EARLY_TERMINATION 1
#define RTKA_UNKNOWN_PRESERVATION 1
#define RTKA_USE_LOOKUP_TABLES 1

#endif /* RTKA_CONSTANTS_H */
