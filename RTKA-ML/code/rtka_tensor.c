/**
 * File: rtka_tensor.c
 * Copyright (c) 2025 - H.Overman opsec.ee@pm.me
 * 
 * PROPRIETARY AND CONFIDENTIAL
 * This file is proprietary to H. Overman and may not be reproduced,
 * distributed, or used without explicit written permission.
 *
 * For licensing inquiries: opsec.ee@pm.me
 *
 * RTKA Tensor Operations Implementation
 */

#include "rtka_tensor.h"
#include <string.h>
#include <stdlib.h>

/* Calculate strides from shape */
static void calculate_strides(uint32_t* strides, const uint32_t* shape, uint32_t ndim) {
    if (ndim == 0) return;
    
    strides[ndim - 1] = 1;
    for (int32_t i = ndim - 2; i >= 0; i--) {
        strides[i] = strides[i + 1] * shape[i + 1];
    }
}

/* Calculate total size */
static uint32_t calculate_size(const uint32_t* shape, uint32_t ndim) {
    uint32_t size = 1;
    for (uint32_t i = 0; i < ndim; i++) {
        size *= shape[i];
    }
    return size;
}

/* Create tensor */
rtka_tensor_t* rtka_tensor_create(const uint32_t* shape, uint32_t ndim) {
    if (ndim > RTKA_MAX_DIMENSIONS) return NULL;
    
    rtka_tensor_t* tensor = (rtka_tensor_t*)rtka_alloc_state();
    if (!tensor) return NULL;
    
    tensor->ndim = ndim;
    memcpy(tensor->shape, shape, ndim * sizeof(uint32_t));
    calculate_strides(tensor->strides, shape, ndim);
    tensor->size = calculate_size(shape, ndim);
    
    tensor->data = rtka_alloc_states(tensor->size);
    if (!tensor->data) {
        rtka_free_state((rtka_state_t*)tensor);
        return NULL;
    }
    
    tensor->flags = RTKA_TENSOR_CONTIGUOUS;
    return tensor;
}

/* Create zeros tensor */
rtka_tensor_t* rtka_tensor_zeros(const uint32_t* shape, uint32_t ndim) {
    rtka_tensor_t* tensor = rtka_tensor_create(shape, ndim);
    if (!tensor) return NULL;
    
    rtka_state_t zero_state = rtka_make_state(RTKA_FALSE, 1.0f);
    for (uint32_t i = 0; i < tensor->size; i++) {
        tensor->data[i] = zero_state;
    }
    
    return tensor;
}

/* Create unknown tensor */
rtka_tensor_t* rtka_tensor_unknown(const uint32_t* shape, uint32_t ndim) {
    rtka_tensor_t* tensor = rtka_tensor_create(shape, ndim);
    if (!tensor) return NULL;
    
    rtka_state_t unknown_state = rtka_make_state(RTKA_UNKNOWN, 0.5f);
    for (uint32_t i = 0; i < tensor->size; i++) {
        tensor->data[i] = unknown_state;
    }
    
    return tensor;
}

/* Element-wise AND */
rtka_tensor_t* rtka_tensor_and(const rtka_tensor_t* a, const rtka_tensor_t* b) {
    if (a->size != b->size) return NULL;
    
    rtka_tensor_t* result = rtka_tensor_create(a->shape, a->ndim);
    if (!result) return NULL;
    
    /* Use batch operations for efficiency */
    rtka_and_batch(a->data, b->data, result->data, a->size);
    
    return result;
}

/* Matrix multiplication - optimized with tiling */
rtka_tensor_t* rtka_tensor_matmul(const rtka_tensor_t* a, const rtka_tensor_t* b) {
    if (a->ndim != 2 || b->ndim != 2) return NULL;
    if (a->shape[1] != b->shape[0]) return NULL;
    
    uint32_t m = a->shape[0], k = a->shape[1], n = b->shape[1];
    uint32_t out_shape[] = {m, n};
    rtka_tensor_t* result = rtka_tensor_zeros(out_shape, 2);
    if (!result) return NULL;
    
    /* Tiled multiplication for cache efficiency */
    const uint32_t TILE = 64;
    
    for (uint32_t i0 = 0; i0 < m; i0 += TILE) {
        for (uint32_t j0 = 0; j0 < n; j0 += TILE) {
            for (uint32_t k0 = 0; k0 < k; k0 += TILE) {
                /* Process tile */
                uint32_t i_max = (i0 + TILE < m) ? i0 + TILE : m;
                uint32_t j_max = (j0 + TILE < n) ? j0 + TILE : n;
                uint32_t k_max = (k0 + TILE < k) ? k0 + TILE : k;
                
                for (uint32_t i = i0; i < i_max; i++) {
                    for (uint32_t j = j0; j < j_max; j++) {
                        rtka_state_t sum = result->data[i * n + j];
                        
                        for (uint32_t kk = k0; kk < k_max; kk++) {
                            rtka_state_t a_val = a->data[i * k + kk];
                            rtka_state_t b_val = b->data[kk * n + j];
                            rtka_state_t prod = rtka_combine_and(a_val, b_val);
                            sum = rtka_combine_or(sum, prod);
                        }
                        
                        result->data[i * n + j] = sum;
                    }
                }
            }
        }
    }
    
    return result;
}

/* Reduce along axis */
rtka_state_t rtka_tensor_reduce_and(const rtka_tensor_t* tensor) {
    if (tensor->size == 0) return rtka_make_state(RTKA_TRUE, 1.0f);
    
    rtka_state_t result = tensor->data[0];
    
    for (uint32_t i = 1; i < tensor->size; i++) {
        if (RTKA_UNLIKELY(result.value == RTKA_FALSE)) break;
        result = rtka_combine_and(result, tensor->data[i]);
    }
    
    return result;
}

/* Check contiguous */
bool rtka_tensor_is_contiguous(const rtka_tensor_t* tensor) {
    return (tensor->flags & RTKA_TENSOR_CONTIGUOUS) != 0;
}

/* Make contiguous */
void rtka_tensor_make_contiguous(rtka_tensor_t* tensor) {
    if (rtka_tensor_is_contiguous(tensor)) return;
    
    rtka_state_t* new_data = rtka_alloc_states(tensor->size);
    if (!new_data) return;
    
    /* Copy with new strides */
    uint32_t indices[RTKA_MAX_DIMENSIONS] = {0};
    
    for (uint32_t i = 0; i < tensor->size; i++) {
        new_data[i] = *rtka_tensor_get(tensor, indices);
        
        /* Increment indices */
        for (int32_t d = tensor->ndim - 1; d >= 0; d--) {
            indices[d]++;
            if (indices[d] < tensor->shape[d]) break;
            indices[d] = 0;
        }
    }
    
    tensor->data = new_data;
    calculate_strides(tensor->strides, tensor->shape, tensor->ndim);
    tensor->flags |= RTKA_TENSOR_CONTIGUOUS;
}

/* Free tensor */
void rtka_tensor_free(rtka_tensor_t* tensor) {
    if (!tensor) return;
    /* Memory is managed by pool/stack allocators */
    rtka_free_state((rtka_state_t*)tensor);
}
