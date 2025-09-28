/**
 * File: rtka_tensor.h
 * Copyright (c) 2025 - H.Overman opsec.ee@pm.me
 * 
 * PROPRIETARY AND CONFIDENTIAL
 * This file is proprietary to H. Overman and may not be reproduced,
 * distributed, or used without explicit written permission.
 *
 * For licensing inquiries: opsec.ee@pm.me
 *
 * RTKA Tensor Operations - N-dimensional arrays for ML
 */

#ifndef RTKA_TENSOR_H
#define RTKA_TENSOR_H

#include "rtka_types.h"
#include "rtka_memory.h"
#include "rtka_vector.h"

#define RTKA_MAX_DIMENSIONS 8
#define RTKA_TENSOR_CONTIGUOUS (1U << 0)
#define RTKA_TENSOR_TRANSPOSED (1U << 1)
#define RTKA_TENSOR_BROADCAST  (1U << 2)

/* Tensor structure - optimized for ML operations */
typedef struct RTKA_ALIGNED(64) {
    rtka_state_t* data;
    uint32_t shape[RTKA_MAX_DIMENSIONS];
    uint32_t strides[RTKA_MAX_DIMENSIONS];
    uint32_t ndim;
    uint32_t size;
    uint32_t flags;
    rtka_allocator_t* allocator;
} rtka_tensor_t;

/* Tensor view for zero-copy operations */
typedef struct {
    rtka_state_t* data;
    uint32_t offset;
    uint32_t shape[RTKA_MAX_DIMENSIONS];
    uint32_t strides[RTKA_MAX_DIMENSIONS];
    uint32_t ndim;
} rtka_tensor_view_t;

/* Creation and destruction */
rtka_tensor_t* rtka_tensor_create(const uint32_t* shape, uint32_t ndim);
rtka_tensor_t* rtka_tensor_zeros(const uint32_t* shape, uint32_t ndim);
rtka_tensor_t* rtka_tensor_ones(const uint32_t* shape, uint32_t ndim);
rtka_tensor_t* rtka_tensor_unknown(const uint32_t* shape, uint32_t ndim);
void rtka_tensor_free(rtka_tensor_t* tensor);

/* Shape operations */
rtka_tensor_t* rtka_tensor_reshape(rtka_tensor_t* tensor, const uint32_t* new_shape, uint32_t new_ndim);
rtka_tensor_t* rtka_tensor_transpose(rtka_tensor_t* tensor, const uint32_t* axes);
rtka_tensor_t* rtka_tensor_squeeze(rtka_tensor_t* tensor);
rtka_tensor_t* rtka_tensor_unsqueeze(rtka_tensor_t* tensor, uint32_t axis);

/* Element access - optimized for cache */
RTKA_INLINE rtka_state_t* rtka_tensor_get(rtka_tensor_t* tensor, const uint32_t* indices) {
    uint32_t offset = 0;
    for (uint32_t i = 0; i < tensor->ndim; i++) {
        offset += indices[i] * tensor->strides[i];
    }
    return &tensor->data[offset];
}

RTKA_INLINE void rtka_tensor_set(rtka_tensor_t* tensor, const uint32_t* indices, rtka_state_t value) {
    uint32_t offset = 0;
    for (uint32_t i = 0; i < tensor->ndim; i++) {
        offset += indices[i] * tensor->strides[i];
    }
    tensor->data[offset] = value;
}

/* Tensor operations */
rtka_tensor_t* rtka_tensor_add(const rtka_tensor_t* a, const rtka_tensor_t* b);
rtka_tensor_t* rtka_tensor_multiply(const rtka_tensor_t* a, const rtka_tensor_t* b);
rtka_tensor_t* rtka_tensor_matmul(const rtka_tensor_t* a, const rtka_tensor_t* b);

/* Ternary logic operations on tensors */
rtka_tensor_t* rtka_tensor_and(const rtka_tensor_t* a, const rtka_tensor_t* b);
rtka_tensor_t* rtka_tensor_or(const rtka_tensor_t* a, const rtka_tensor_t* b);
rtka_tensor_t* rtka_tensor_not(const rtka_tensor_t* a);

/* Reductions */
rtka_state_t rtka_tensor_reduce_and(const rtka_tensor_t* tensor);
rtka_state_t rtka_tensor_reduce_or(const rtka_tensor_t* tensor);
rtka_tensor_t* rtka_tensor_reduce_along_axis(const rtka_tensor_t* tensor, uint32_t axis, 
                                            rtka_state_t (*reduce_fn)(const rtka_state_t*, uint32_t));

/* Broadcasting */
bool rtka_tensor_broadcast_compatible(const rtka_tensor_t* a, const rtka_tensor_t* b);
void rtka_tensor_broadcast_shapes(const uint32_t* shape_a, uint32_t ndim_a,
                                  const uint32_t* shape_b, uint32_t ndim_b,
                                  uint32_t* out_shape, uint32_t* out_ndim);

/* Slicing and indexing */
rtka_tensor_view_t rtka_tensor_slice(const rtka_tensor_t* tensor, 
                                     const uint32_t* start, 
                                     const uint32_t* stop);
rtka_tensor_t* rtka_tensor_from_view(const rtka_tensor_view_t* view);

/* Confidence operations */
void rtka_tensor_apply_confidence(rtka_tensor_t* tensor, rtka_confidence_t (*fn)(rtka_confidence_t));
void rtka_tensor_normalize_confidence(rtka_tensor_t* tensor);

/* Memory layout optimization */
void rtka_tensor_make_contiguous(rtka_tensor_t* tensor);
bool rtka_tensor_is_contiguous(const rtka_tensor_t* tensor);
size_t rtka_tensor_memory_size(const rtka_tensor_t* tensor);

#endif /* RTKA_TENSOR_H */
