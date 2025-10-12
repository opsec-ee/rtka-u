/**
 * File: rtka_constants.h
 * Copyright (c) 2025 H. Overman <opsec.ee@pm.me>
 *
 * PROPRIETARY AND CONFIDENTIAL
 * This file is proprietary to H. Overman and may not be reproduced,
 * distributed, or used without explicit written permission.
 *
 * For licensing inquiries: opsec.ee@pm.me
 *
 * RTKA Global Constants and Configuration
 * Central location for compile-time constants
 */

#ifndef RTKA_CONSTANTS_H
#define RTKA_CONSTANTS_H

/* Core limits */
#define RTKA_MAX_FACTORS 64U
#define RTKA_MAX_VECTOR_SIZE 16U
#define RTKA_CONFIDENCE_EPSILON 1e-6f

/* Default allocator sizes */
#define RTKA_DEFAULT_POOL_BLOCKS 1024U
#define RTKA_DEFAULT_STACK_SIZE (64U * 1024U)
#define RTKA_DEFAULT_RING_SIZE 256U

/* Threading model - opt-in */
#define RTKA_SINGLE_THREADED_DEFAULT 1

/* Memory alignment */
#define RTKA_CACHE_LINE_SIZE 64U
#define RTKA_SIMD_ALIGNMENT 64U

#endif /* RTKA_CONSTANTS_H */
