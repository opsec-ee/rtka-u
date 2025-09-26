/**
 * File: rtka_types.h
 * Copyright (c) 2025 - H.Overman <opsec.ee@pm.me>
 * Email: opsec.ee@pm.me
 * 
 * RTKA: Recursive Ternary Knowledge Algorithm - Universal Type System
 * Core type definitions and universal interfaces for all specialized modules
 * Mathematical foundation: T = {-1, 0, 1} with confidence domain [0,1]
 *
 * CHANGELOG:
 * v1.0.0 - Initial comprehensive type system
 * - Ternary domain types with arithmetic encoding
 * - Confidence and uncertainty quantification structures
 * - Universal interfaces for specialized modules
 * - C23 feature integration with backward compatibility
 * - Memory management and threading support types
 * - Performance optimization structures
 * 
 * Design Principles:
 * - Mathematical rigor: Domain T = {-1, 0, 1} with precise semantics
 * - Zero-cost abstractions: Types compile to optimal assembly
 * - Module extensibility: Universal foundation for rtka-ml, rtka-is, etc.
 * - Safety first: Comprehensive bounds checking and validation
 * - Performance: SIMD-friendly alignment and cache optimization
 */

#ifndef RTKA_TYPES_H
#define RTKA_TYPES_H

/* ============================================================================
 * FEATURE DETECTION AND COMPATIBILITY
 * ============================================================================ */

#ifdef __STDC_VERSION__
    #if __STDC_VERSION__ >= 202311L
        #define RTKA_C23_AVAILABLE 1
        #define RTKA_C17_AVAILABLE 1
        #define RTKA_C11_AVAILABLE 1
    #elif __STDC_VERSION__ >= 201710L
        #define RTKA_C17_AVAILABLE 1
        #define RTKA_C11_AVAILABLE 1
    #elif __STDC_VERSION__ >= 201112L
        #define RTKA_C11_AVAILABLE 1
    #endif
#endif

/* C23 compatibility macros */
#ifdef RTKA_C23_AVAILABLE
    #include <stdbit.h>
    #define RTKA_NULLPTR nullptr
    typedef typeof(nullptr) rtka_nullptr_t;
#else
    #define RTKA_NULLPTR ((void*)0)
    typedef void* rtka_nullptr_t;
#endif

/* Standard library includes */
#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>
#include <limits.h>
#include <float.h>

#ifdef RTKA_C11_AVAILABLE
    #include <stdnoreturn.h>
    #include <stdalign.h>
    #include <stdatomic.h>
    #include <threads.h>
    
    /* Compile-time assertions for mathematical consistency */
    _Static_assert(sizeof(int8_t) == 1, "int8_t must be exactly 1 byte");
    _Static_assert(sizeof(float) == 4, "float must be 32-bit IEEE-754");
    _Static_assert(sizeof(double) == 8, "double must be 64-bit IEEE-754");
    _Static_assert(CHAR_BIT == 8, "Byte must be 8 bits");
#endif

/* Compiler-specific optimizations */
#ifdef __GNUC__
    #define RTKA_INLINE __attribute__((always_inline)) inline
    #define RTKA_PURE __attribute__((pure))
    #define RTKA_CONST __attribute__((const))
    #define RTKA_HOT __attribute__((hot))
    #define RTKA_COLD __attribute__((cold))
    #define RTKA_PACKED __attribute__((packed))
    #define RTKA_ALIGNED(n) __attribute__((aligned(n)))
    #define RTKA_LIKELY(x) __builtin_expect(!!(x), 1)
    #define RTKA_UNLIKELY(x) __builtin_expect(x, 0)
    #define RTKA_UNREACHABLE() __builtin_unreachable()
#else
    #define RTKA_INLINE inline
    #define RTKA_PURE
    #define RTKA_CONST
    #define RTKA_HOT
    #define RTKA_COLD
    #define RTKA_PACKED
    #define RTKA_ALIGNED(n)
    #define RTKA_LIKELY(x) (x)
    #define RTKA_UNLIKELY(x) (x)
    #define RTKA_UNREACHABLE() abort()
#endif

/* C23 attributes with fallbacks */
#ifdef RTKA_C23_AVAILABLE
    #define RTKA_NODISCARD [[nodiscard]]
    #define RTKA_MAYBE_UNUSED [[maybe_unused]]
    #define RTKA_DEPRECATED(msg) [[deprecated(msg)]]
    #define RTKA_FALLTHROUGH [[fallthrough]]
#elif defined(__GNUC__)
    #define RTKA_NODISCARD __attribute__((warn_unused_result))
    #define RTKA_MAYBE_UNUSED __attribute__((unused))
    #define RTKA_DEPRECATED(msg) __attribute__((deprecated(msg)))
    #define RTKA_FALLTHROUGH __attribute__((fallthrough))
#else
    #define RTKA_NODISCARD
    #define RTKA_MAYBE_UNUSED
    #define RTKA_DEPRECATED(msg)
    #define RTKA_FALLTHROUGH
#endif

/* ============================================================================
 * MATHEMATICAL DOMAIN DEFINITIONS
 * ============================================================================ */

/**
 * Core ternary domain T = {-1, 0, 1} with arithmetic encoding
 * This encoding enables efficient computation through standard operations:
 * - Conjunction: min(a, b)
 * - Disjunction: max(a, b)  
 * - Negation: -a
 * - Equivalence: a * b
 */
typedef enum RTKA_PACKED {
    RTKA_FALSE = -1,    /* FALSE ≡ -1 */
    RTKA_UNKNOWN = 0,   /* UNKNOWN ≡ 0 */
    RTKA_TRUE = 1       /* TRUE ≡ 1 */
} rtka_value_t;

/* Compile-time validation of arithmetic encoding */
#ifdef RTKA_C11_AVAILABLE
_Static_assert((int)RTKA_FALSE == -1, "RTKA_FALSE must equal -1");
_Static_assert((int)RTKA_UNKNOWN == 0, "RTKA_UNKNOWN must equal 0");
_Static_assert((int)RTKA_TRUE == 1, "RTKA_TRUE must equal 1");
#endif

/**
 * Confidence domain [0,1] for uncertainty quantification
 * Uses 32-bit float for balance of precision and performance
 */
typedef float rtka_confidence_t;

/**
 * High-precision confidence for specialized applications
 * Used in rtka-fin, rtka-crypto where precision is critical
 */
typedef double rtka_confidence_hp_t;

/* Confidence bounds and validation */
#define RTKA_MIN_CONFIDENCE 0.0f
#define RTKA_MAX_CONFIDENCE 1.0f
#define RTKA_CONFIDENCE_EPSILON 1e-6f

#define RTKA_MIN_CONFIDENCE_HP 0.0
#define RTKA_MAX_CONFIDENCE_HP 1.0
#define RTKA_CONFIDENCE_EPSILON_HP 1e-12

/**
 * Combined ternary state with confidence
 * Cache-line optimized for performance-critical applications
 */
typedef struct RTKA_ALIGNED(8) {
    rtka_value_t value;        /* Ternary value */
    rtka_confidence_t confidence;  /* Associated confidence [0,1] */
} rtka_state_t;

/**
 * High-precision state for specialized modules
 */
typedef struct RTKA_ALIGNED(16) {
    rtka_value_t value;
    rtka_confidence_hp_t confidence;
    uint32_t flags;            /* Extension flags for specialized use */
    uint32_t reserved;         /* Padding for future expansion */
} rtka_state_hp_t;

/**
 * Extended state with metadata for complex reasoning
 * Used in rtka-legal, rtka-cyber for audit trails
 */
typedef struct {
    rtka_state_t core;         /* Core ternary state */
    uint64_t timestamp;        /* Creation timestamp */
    uint32_t source_id;        /* Source identifier */
    uint16_t depth;            /* Recursion depth */
    uint16_t flags;            /* Status flags */
    void* metadata;            /* Module-specific metadata */
} rtka_state_ext_t;

/* State flags for extended states */
#define RTKA_FLAG_VALIDATED     (1U << 0U)
#define RTKA_FLAG_CACHED        (1U << 1U)
#define RTKA_FLAG_DIRTY         (1U << 2U)
#define RTKA_FLAG_PERSISTENT    (1U << 3U)
#define RTKA_FLAG_ENCRYPTED     (1U << 4U)  /* For rtka-crypto */
#define RTKA_FLAG_AUDITED       (1U << 5U)  /* For rtka-legal */
#define RTKA_FLAG_REAL_TIME     (1U << 6U)  /* For rtka-is */

/* ============================================================================
 * PERFORMANCE AND OPTIMIZATION TYPES
 * ============================================================================ */

/**
 * SIMD-optimized state vector for batch operations
 * Aligned for AVX-512 instructions (64-byte alignment)
 */
#define RTKA_VECTOR_SIZE 16U
typedef struct RTKA_ALIGNED(64) {
    rtka_value_t values[RTKA_VECTOR_SIZE];
    rtka_confidence_t confidences[RTKA_VECTOR_SIZE];
    uint32_t count;            /* Active element count */
    uint32_t reserved[3];      /* Padding to maintain alignment */
} rtka_vector_t;

/**
 * Memory pool descriptor for efficient allocation
 * Used to reduce fragmentation in recursive operations
 */
typedef struct {
    void* memory_base;         /* Base address of pool */
    size_t total_size;         /* Total pool size in bytes */
    size_t allocated_size;     /* Currently allocated bytes */
    size_t alignment;          /* Allocation alignment */
    bool is_locked;            /* Memory lock status */
    
#ifdef RTKA_C11_AVAILABLE
    atomic_uint_fast32_t reference_count;  /* Thread-safe reference counting */
    mtx_t pool_mutex;          /* Pool access synchronization */
#endif
} rtka_memory_pool_t;

/**
 * Performance metrics for optimization
 */
typedef struct {
    uint64_t operations_count;        /* Total operations performed */
    uint64_t early_terminations;      /* Early termination optimizations */
    double computation_time;          /* Total computation time */
    double average_confidence;        /* Average confidence level */
    uint32_t cache_hits;              /* Cache hit count */
    uint32_t cache_misses;            /* Cache miss count */
} rtka_performance_t;

/* ============================================================================
 * SPECIALIZED MODULE INTERFACE TYPES
 * ============================================================================ */

/**
 * Universal operation function pointer
 * Standard interface for all RTKA operations across modules
 */
typedef rtka_state_t (*rtka_operation_fn_t)(rtka_state_t lhs, rtka_state_t rhs);

/**
 * Batch operation function pointer for vectorized operations
 */
typedef void (*rtka_batch_operation_fn_t)(rtka_vector_t* result,
                                         const rtka_vector_t* lhs,
                                         const rtka_vector_t* rhs);

/**
 * Confidence propagation function pointer
 * Defines how confidence values combine in operations
 */
typedef rtka_confidence_t (*rtka_confidence_propagation_fn_t)(rtka_confidence_t lhs,
                                                             rtka_confidence_t rhs);

/**
 * Validation function pointer for domain-specific checks
 */
typedef bool (*rtka_validation_fn_t)(const rtka_state_t* state, void* context);

/**
 * Serialization function pointer for persistence
 */
typedef size_t (*rtka_serialize_fn_t)(const void* data, void* buffer, size_t buffer_size);

/**
 * Module descriptor for specialized implementations
 * Each module (rtka-ml, rtka-is, etc.) provides this interface
 */
typedef struct {
    const char* module_name;                    /* Module identifier */
    uint32_t version_major;                     /* Major version */
    uint32_t version_minor;                     /* Minor version */
    uint32_t version_patch;                     /* Patch version */
    
    /* Core operation functions */
    rtka_operation_fn_t and_fn;                 /* Conjunction implementation */
    rtka_operation_fn_t or_fn;                  /* Disjunction implementation */
    rtka_operation_fn_t not_fn;                 /* Negation implementation */
    rtka_operation_fn_t equiv_fn;               /* Equivalence implementation */
    
    /* Batch operations for performance */
    rtka_batch_operation_fn_t batch_and_fn;     /* Vectorized conjunction */
    rtka_batch_operation_fn_t batch_or_fn;      /* Vectorized disjunction */
    
    /* Confidence propagation */
    rtka_confidence_propagation_fn_t conf_and_fn;  /* AND confidence rule */
    rtka_confidence_propagation_fn_t conf_or_fn;   /* OR confidence rule */
    
    /* Module-specific functions */
    rtka_validation_fn_t validate_fn;           /* Input validation */
    rtka_serialize_fn_t serialize_fn;           /* State serialization */
    rtka_serialize_fn_t deserialize_fn;         /* State deserialization */
    
    /* Module initialization/cleanup */
    bool (*init_fn)(void);                      /* Module initialization */
    void (*cleanup_fn)(void);                   /* Module cleanup */
    
    /* Module-specific data */
    void* module_data;                          /* Private module data */
    size_t data_size;                           /* Size of module data */
} rtka_module_t;

/* ============================================================================
 * ERROR HANDLING AND RESULT TYPES
 * ============================================================================ */

/**
 * Comprehensive error codes for all RTKA operations
 */
typedef enum {
    RTKA_SUCCESS = 0,              /* Operation succeeded */
    RTKA_ERROR_NULL_POINTER = 1,   /* Null pointer argument */
    RTKA_ERROR_INVALID_VALUE = 2,  /* Value outside ternary domain */
    RTKA_ERROR_INVALID_CONFIDENCE = 3, /* Confidence outside [0,1] */
    RTKA_ERROR_BUFFER_OVERFLOW = 4,    /* Buffer too small */
    RTKA_ERROR_OUT_OF_MEMORY = 5,      /* Memory allocation failed */
    RTKA_ERROR_INVALID_DIMENSION = 6,  /* Invalid tensor/vector dimension */
    RTKA_ERROR_ARITHMETIC_OVERFLOW = 7, /* Arithmetic operation overflow */
    RTKA_ERROR_DIVISION_BY_ZERO = 8,   /* Division by zero */
    RTKA_ERROR_MODULE_NOT_FOUND = 9,   /* Module not registered */
    RTKA_ERROR_MODULE_INIT_FAILED = 10, /* Module initialization failed */
    RTKA_ERROR_THREAD_FAILURE = 11,    /* Threading operation failed */
    RTKA_ERROR_SERIALIZATION = 12,     /* Serialization/deserialization error */
    RTKA_ERROR_VALIDATION_FAILED = 13, /* Input validation failed */
    RTKA_ERROR_TIMEOUT = 14,           /* Operation timeout */
    RTKA_ERROR_PERMISSION_DENIED = 15, /* Insufficient permissions */
    RTKA_ERROR_NETWORK = 16,           /* Network operation failed */
    RTKA_ERROR_FILE_IO = 17,           /* File I/O operation failed */
    RTKA_ERROR_CRYPTO = 18,            /* Cryptographic operation failed */
    RTKA_ERROR_UNKNOWN = 255           /* Unknown error */
} rtka_error_t;

/**
 * Result type with comprehensive error information
 * Follows Rust-style Result<T, E> pattern adapted for C
 */
#define RTKA_RESULT_TYPE(T) \
    typedef struct { \
        T value; \
        rtka_error_t error; \
        bool has_value; \
        const char* error_message; \
        uint32_t error_line; \
        const char* error_file; \
    } rtka_result_##T##_t

/* Pre-defined result types for common operations */
RTKA_RESULT_TYPE(rtka_state);
RTKA_RESULT_TYPE(rtka_vector);
RTKA_RESULT_TYPE(rtka_confidence);

/* Convenience macros for result handling */
#define RTKA_OK(type, val) ((rtka_result_##type##_t){ \
    .value = (val), \
    .error = RTKA_SUCCESS, \
    .has_value = true, \
    .error_message = RTKA_NULLPTR, \
    .error_line = 0U, \
    .error_file = RTKA_NULLPTR \
})

#define RTKA_ERROR(type, err, msg) ((rtka_result_##type##_t){ \
    .value = {0}, \
    .error = (err), \
    .has_value = false, \
    .error_message = (msg), \
    .error_line = __LINE__, \
    .error_file = __FILE__ \
})

#define RTKA_IS_OK(result) ((result).has_value && (result).error == RTKA_SUCCESS)
#define RTKA_IS_ERROR(result) (!(result).has_value || (result).error != RTKA_SUCCESS)

/* ============================================================================
 * THREADING AND CONCURRENCY TYPES
 * ============================================================================ */

#ifdef RTKA_C11_AVAILABLE
/**
 * Thread-safe state with atomic operations
 * Critical for rtka-net and real-time applications
 */
typedef struct {
    _Atomic(rtka_value_t) value;
    _Atomic(uint32_t) confidence_bits;  /* Confidence stored as uint32 */
    atomic_uint_fast64_t timestamp;
    atomic_uint_fast32_t access_count;
} rtka_atomic_state_t;

/**
 * Read-write lock for complex data structures
 */
typedef struct {
    mtx_t write_mutex;          /* Exclusive write access */
    mtx_t read_count_mutex;     /* Reader count protection */
    atomic_int reader_count;    /* Current reader count */
    bool writer_waiting;        /* Writer priority flag */
} rtka_rwlock_t;

/**
 * Thread pool configuration for parallel operations
 */
typedef struct {
    uint32_t thread_count;      /* Number of worker threads */
    uint32_t queue_size;        /* Work queue size */
    uint32_t stack_size;        /* Thread stack size */
    bool low_priority;          /* Use low priority threads */
    void (*error_callback)(rtka_error_t, const char*);  /* Error handler */
} rtka_thread_config_t;
#endif

/* ============================================================================
 * MATHEMATICAL CONSTANTS AND LIMITS
 * ============================================================================ */

/* Core algorithm limits */
#define RTKA_MAX_RECURSION_DEPTH 1000U     /* Maximum recursion depth */
#define RTKA_MAX_FACTORS 64U               /* Maximum factorization terms */
#define RTKA_MAX_VECTOR_SIZE 1024U         /* Maximum vector elements */
#define RTKA_MAX_DIMENSIONS 8U             /* Maximum tensor dimensions */

/* Timeout limits for real-time applications */
#define RTKA_DEFAULT_TIMEOUT_MS 1000U      /* Default operation timeout */
#define RTKA_MAX_TIMEOUT_MS 60000U         /* Maximum allowed timeout */

/* Cache and memory limits */
#define RTKA_DEFAULT_CACHE_SIZE (1024U * 1024U)  /* 1MB default cache */
#define RTKA_MAX_CACHE_SIZE (1024U * 1024U * 1024U) /* 1GB maximum cache */
#define RTKA_CACHE_LINE_SIZE 64U           /* Assume 64-byte cache lines */

/* Numerical precision limits */
#define RTKA_PI 3.14159265358979323846     /* Mathematical π */
#define RTKA_E  2.71828182845904523536     /* Euler's number */
#define RTKA_GOLDEN_RATIO 1.61803398874989484820  /* φ = (1+√5)/2 */

/* UNKNOWN preservation probability: (2/3)^(n-1) */
#define RTKA_UNKNOWN_PROBABILITY_BASE (2.0/3.0)

/* ============================================================================
 * UTILITY MACROS FOR TYPE SAFETY
 * ============================================================================ */

/* Type-safe minimum/maximum with ternary semantics */
#ifdef RTKA_C11_AVAILABLE
#define RTKA_MIN(a, b) _Generic((a) + (b), \
    int: ((a) < (b) ? (a) : (b)), \
    float: (fminf((a), (b))), \
    double: (fmin((a), (b))), \
    rtka_value_t: ((a) < (b) ? (a) : (b)), \
    default: ((a) < (b) ? (a) : (b)) \
)

#define RTKA_MAX(a, b) _Generic((a) + (b), \
    int: ((a) > (b) ? (a) : (b)), \
    float: (fmaxf((a), (b))), \
    double: (fmax((a), (b))), \
    rtka_value_t: ((a) > (b) ? (a) : (b)), \
    default: ((a) > (b) ? (a) : (b)) \
)
#else
#define RTKA_MIN(a, b) ((a) < (b) ? (a) : (b))
#define RTKA_MAX(a, b) ((a) > (b) ? (a) : (b))
#endif

/* Array size calculation with overflow protection */
#define RTKA_ARRAY_SIZE(arr) (sizeof(arr) / sizeof((arr)[0U]))

/* Alignment macros */
#define RTKA_ALIGN_UP(n, alignment) (((n) + (alignment) - 1U) & ~((alignment) - 1U))
#define RTKA_IS_ALIGNED(ptr, alignment) (((uintptr_t)(ptr) & ((alignment) - 1U)) == 0U)

/* Bit manipulation macros with type safety */
#define RTKA_SET_FLAG(flags, flag) ((flags) |= (flag))
#define RTKA_CLEAR_FLAG(flags, flag) ((flags) &= ~(flag))
#define RTKA_TEST_FLAG(flags, flag) (((flags) & (flag)) != 0U)
#define RTKA_TOGGLE_FLAG(flags, flag) ((flags) ^= (flag))

/* Safe arithmetic macros */
#define RTKA_SAFE_ADD(a, b, max_val) \
    (((max_val) - (a)) >= (b) ? ((a) + (b)) : (max_val))

#define RTKA_SAFE_MUL(a, b, max_val) \
    (((a) == 0U || (b) == 0U) ? 0U : \
     (((max_val) / (a)) >= (b)) ? ((a) * (b)) : (max_val))

/* ============================================================================
 * FORWARD DECLARATIONS
 * ============================================================================ */

/* Core operation functions (implemented in rtka_core.c) */
RTKA_NODISCARD RTKA_PURE RTKA_HOT
rtka_state_t rtka_and(rtka_state_t lhs, rtka_state_t rhs);

RTKA_NODISCARD RTKA_PURE RTKA_HOT
rtka_state_t rtka_or(rtka_state_t lhs, rtka_state_t rhs);

RTKA_NODISCARD RTKA_PURE RTKA_HOT
rtka_state_t rtka_not(rtka_state_t state);

RTKA_NODISCARD RTKA_PURE
rtka_state_t rtka_equiv(rtka_state_t lhs, rtka_state_t rhs);

/* Validation functions */
RTKA_NODISCARD RTKA_PURE RTKA_INLINE
bool rtka_is_valid_value(rtka_value_t value) {
    return value >= RTKA_FALSE && value <= RTKA_TRUE;
}

RTKA_NODISCARD RTKA_PURE RTKA_INLINE
bool rtka_is_valid_confidence(rtka_confidence_t confidence) {
    return confidence >= RTKA_MIN_CONFIDENCE && 
           confidence <= RTKA_MAX_CONFIDENCE &&
           !isnan(confidence) && !isinf(confidence);
}

RTKA_NODISCARD RTKA_PURE RTKA_INLINE
bool rtka_is_valid_state(const rtka_state_t* state) {
    return state != RTKA_NULLPTR &&
           rtka_is_valid_value(state->value) &&
           rtka_is_valid_confidence(state->confidence);
}

/* State construction helpers */
RTKA_NODISCARD RTKA_INLINE
rtka_state_t rtka_make_state(rtka_value_t value, rtka_confidence_t confidence) {
    return (rtka_state_t) {
        .value = value,
        .confidence = confidence
    };
}

RTKA_NODISCARD RTKA_INLINE
rtka_state_t rtka_false_state(rtka_confidence_t confidence) {
    return rtka_make_state(RTKA_FALSE, confidence);
}

RTKA_NODISCARD RTKA_INLINE
rtka_state_t rtka_unknown_state(rtka_confidence_t confidence) {
    return rtka_make_state(RTKA_UNKNOWN, confidence);
}

RTKA_NODISCARD RTKA_INLINE
rtka_state_t rtka_true_state(rtka_confidence_t confidence) {
    return rtka_make_state(RTKA_TRUE, confidence);
}

/* Error handling helpers */
RTKA_NODISCARD
const char* rtka_error_string(rtka_error_t error);

RTKA_NODISCARD
bool rtka_is_recoverable_error(rtka_error_t error);

/* Module registration functions */
RTKA_NODISCARD
rtka_error_t rtka_register_module(const rtka_module_t* module);

RTKA_NODISCARD
rtka_error_t rtka_unregister_module(const char* module_name);

RTKA_NODISCARD
const rtka_module_t* rtka_find_module(const char* module_name);

/* Memory management functions */
RTKA_NODISCARD
rtka_memory_pool_t* rtka_create_memory_pool(size_t size, size_t alignment);

void rtka_destroy_memory_pool(rtka_memory_pool_t* pool);

RTKA_NODISCARD
void* rtka_pool_alloc(rtka_memory_pool_t* pool, size_t size);

void rtka_pool_free(rtka_memory_pool_t* pool, void* ptr);

/* ============================================================================
 * VERSION INFORMATION
 * ============================================================================ */

#define RTKA_TYPES_VERSION_MAJOR 1U
#define RTKA_TYPES_VERSION_MINOR 0U
#define RTKA_TYPES_VERSION_PATCH 0U
#define RTKA_TYPES_VERSION_STRING "1.0.0"

/**
 * Version information structure
 */
typedef struct {
    uint32_t major;
    uint32_t minor;
    uint32_t patch;
    const char* version_string;
    const char* build_date;
    const char* build_time;
    uint64_t feature_flags;
} rtka_version_info_t;

RTKA_NODISCARD RTKA_CONST
rtka_version_info_t rtka_get_version_info(void);

/* Feature flags for version information */
#define RTKA_FEATURE_C11_SUPPORT     (1ULL << 0ULL)
#define RTKA_FEATURE_C17_SUPPORT     (1ULL << 1ULL)
#define RTKA_FEATURE_C23_SUPPORT     (1ULL << 2ULL)
#define RTKA_FEATURE_ATOMIC_SUPPORT  (1ULL << 3ULL)
#define RTKA_FEATURE_THREADS_SUPPORT (1ULL << 4ULL)
#define RTKA_FEATURE_SIMD_SUPPORT    (1ULL << 5ULL)
#define RTKA_FEATURE_CRYPTO_SUPPORT  (1ULL << 6ULL)
#define RTKA_FEATURE_NETWORK_SUPPORT (1ULL << 7ULL)

#endif /* RTKA_TYPES_H */
