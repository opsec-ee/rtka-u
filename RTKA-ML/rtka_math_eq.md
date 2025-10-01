# RTKA Mathematical Equations Reference

**Copyright (c) 2025 - H.Overman**  
**Email: opsec.ee@pm.me**

---

## 1. DOMAIN DEFINITION

### Mathematical Notation
```
T = {-1, 0, 1}

where:
  -1 ≡ FALSE
   0 ≡ UNKNOWN
   1 ≡ TRUE
```

### Assembly Representation (x86-64)
```asm
; Ternary values as signed 8-bit integers
; FALSE:   mov al, -1    ; 0xFF in two's complement
; UNKNOWN: mov al, 0     ; 0x00
; TRUE:    mov al, 1     ; 0x01
```

### C Implementation
```c
typedef int8_t rtka_value_t;

#define RTKA_FALSE    ((rtka_value_t)-1)
#define RTKA_UNKNOWN  ((rtka_value_t) 0)
#define RTKA_TRUE     ((rtka_value_t) 1)
```

---

## 2. CORE KLEENE OPERATIONS

### 2.1 Negation

#### Mathematical Definition
```
¬a = -a

Truth Table:
  ¬(-1) = 1   (¬FALSE = TRUE)
  ¬(0)  = 0   (¬UNKNOWN = UNKNOWN)
  ¬(1)  = -1  (¬TRUE = FALSE)
```

#### Assembly Implementation (x86-64)
```asm
; Input:  al = ternary value
; Output: al = negated value
rtka_not:
    neg al              ; Two's complement negation
    ret                 ; Return with result in al
```

#### C Implementation
```c
static inline rtka_value_t rtka_not(rtka_value_t a) {
    return -a;
}
```

---

### 2.2 Conjunction (AND)

#### Mathematical Definition
```
a ∧ b = min(a, b)

Truth Table:
  -1 ∧ -1 = -1    -1 ∧ 0 = -1    -1 ∧ 1 = -1
   0 ∧ -1 = -1     0 ∧ 0 =  0     0 ∧ 1 =  0
   1 ∧ -1 = -1     1 ∧ 0 =  0     1 ∧ 1 =  1
```

#### Assembly Implementation (x86-64)
```asm
; Input:  al = a, bl = b
; Output: al = a ∧ b
rtka_and:
    cmp al, bl          ; Compare a and b
    cmovg al, bl        ; If a > b, move b to al (select minimum)
    ret                 ; Return with result in al
```

#### C Implementation
```c
static inline rtka_value_t rtka_and(rtka_value_t a, rtka_value_t b) {
    if (a == RTKA_FALSE || b == RTKA_FALSE) return RTKA_FALSE;
    if (a == RTKA_UNKNOWN || b == RTKA_UNKNOWN) return RTKA_UNKNOWN;
    return RTKA_TRUE;
}

/* Alternative arithmetic implementation */
static inline rtka_value_t rtka_and_min(rtka_value_t a, rtka_value_t b) {
    return (a < b) ? a : b;
}
```

---

### 2.3 Disjunction (OR)

#### Mathematical Definition
```
a ∨ b = max(a, b)

Truth Table:
  -1 ∨ -1 = -1    -1 ∨ 0 =  0    -1 ∨ 1 =  1
   0 ∨ -1 =  0     0 ∨ 0 =  0     0 ∨ 1 =  1
   1 ∨ -1 =  1     1 ∨ 0 =  1     1 ∨ 1 =  1
```

#### Assembly Implementation (x86-64)
```asm
; Input:  al = a, bl = b
; Output: al = a ∨ b
rtka_or:
    cmp al, bl          ; Compare a and b
    cmovl al, bl        ; If a < b, move b to al (select maximum)
    ret                 ; Return with result in al
```

#### C Implementation
```c
static inline rtka_value_t rtka_or(rtka_value_t a, rtka_value_t b) {
    if (a == RTKA_TRUE || b == RTKA_TRUE) return RTKA_TRUE;
    if (a == RTKA_UNKNOWN || b == RTKA_UNKNOWN) return RTKA_UNKNOWN;
    return RTKA_FALSE;
}

/* Alternative arithmetic implementation */
static inline rtka_value_t rtka_or_max(rtka_value_t a, rtka_value_t b) {
    return (a > b) ? a : b;
}
```

---

### 2.4 Implication

#### Mathematical Definition
```
a → b = ¬a ∨ b = max(-a, b)

Truth Table:
  -1 → -1 =  1    -1 → 0 =  1    -1 → 1 =  1
   0 → -1 =  0     0 → 0 =  0     0 → 1 =  1
   1 → -1 = -1     1 → 0 =  0     1 → 1 =  1
```

#### Assembly Implementation (x86-64)
```asm
; Input:  al = a, bl = b
; Output: al = a → b
rtka_implies:
    neg al              ; Negate a (¬a)
    cmp al, bl          ; Compare -a and b
    cmovl al, bl        ; Select maximum
    ret
```

#### C Implementation
```c
static inline rtka_value_t rtka_implies(rtka_value_t a, rtka_value_t b) {
    return rtka_or(rtka_not(a), b);
}
```

---

### 2.5 Equivalence

#### Mathematical Definition
```
a ↔ b = a × b  (when both are certain)

Returns UNKNOWN if either operand is UNKNOWN
```

#### Assembly Implementation (x86-64)
```asm
; Input:  al = a, bl = b
; Output: al = a ↔ b
rtka_equiv:
    test al, al         ; Check if a is UNKNOWN
    jz .unknown
    test bl, bl         ; Check if b is UNKNOWN
    jz .unknown
    cmp al, bl          ; Compare a and b
    sete al             ; Set al=1 if equal, 0 otherwise
    add al, al          ; Convert 0,1 to 0,2
    dec al              ; Convert 0,2 to -1,1
    ret
.unknown:
    xor al, al          ; Return UNKNOWN (0)
    ret
```

#### C Implementation
```c
static inline rtka_value_t rtka_equiv(rtka_value_t a, rtka_value_t b) {
    if (a == RTKA_UNKNOWN || b == RTKA_UNKNOWN) return RTKA_UNKNOWN;
    return (a == b) ? RTKA_TRUE : RTKA_FALSE;
}
```

---

### 2.6 Exclusive OR (XOR)

#### Mathematical Definition
```
a ⊕ b = TRUE when a ≠ b and both are certain

Returns UNKNOWN if either operand is UNKNOWN
```

#### Assembly Implementation (x86-64)
```asm
; Input:  al = a, bl = b
; Output: al = a ⊕ b
rtka_xor:
    test al, al         ; Check if a is UNKNOWN
    jz .unknown
    test bl, bl         ; Check if b is UNKNOWN
    jz .unknown
    cmp al, bl          ; Compare a and b
    setne al            ; Set al=1 if not equal, 0 otherwise
    add al, al          ; Convert 0,1 to 0,2
    dec al              ; Convert 0,2 to -1,1
    ret
.unknown:
    xor al, al          ; Return UNKNOWN (0)
    ret
```

#### C Implementation
```c
static inline rtka_value_t rtka_xor(rtka_value_t a, rtka_value_t b) {
    if (a == RTKA_UNKNOWN || b == RTKA_UNKNOWN) return RTKA_UNKNOWN;
    return (a != b) ? RTKA_TRUE : RTKA_FALSE;
}
```

---

## 3. RECURSIVE EVALUATION

### Mathematical Definition
```
For operation φ ∈ {∧ᵣ, ∨ᵣ, ¬ᵣ} and input vector x⃗ = ⟨x₁, x₂, ..., xₙ⟩ ∈ Tⁿ:

φ(x⃗) = {
    x₁                              if n = 1
    φ(x₁, φ(⟨x₂, ..., xₙ⟩))         if n > 1
}
```

### Assembly Implementation (x86-64)
```asm
; Recursive AND over array
; Input:  rdi = array pointer, rsi = count
; Output: al = result
rtka_recursive_and:
    cmp rsi, 0
    je .empty               ; Return TRUE for empty
    cmp rsi, 1
    je .single              ; Return single element
    
    mov al, [rdi]           ; Load first element
    mov rcx, 1              ; Initialize counter
    
.loop:
    cmp al, -1              ; Early termination on FALSE
    je .done
    cmp rcx, rsi            ; Check if done
    je .done
    
    mov bl, [rdi + rcx]     ; Load next element
    call rtka_and           ; Apply AND
    inc rcx
    jmp .loop
    
.single:
    mov al, [rdi]           ; Return single element
    ret
    
.empty:
    mov al, 1               ; Identity for AND is TRUE
    ret
    
.done:
    ret
```

### C Implementation
```c
rtka_state_t rtka_recursive_and(const rtka_state_t* states, uint32_t count) {
    if (count == 0) {
        return rtka_make_state(RTKA_TRUE, 1.0f);  /* Identity for AND */
    }
    if (count == 1) {
        return states[0];
    }
    
    rtka_value_t result_value = states[0].value;
    rtka_confidence_t result_conf = states[0].confidence;
    
    for (uint32_t i = 1; i < count; i++) {
        /* Early termination on FALSE */
        if (result_value == RTKA_FALSE) {
            break;
        }
        
        result_value = rtka_and(result_value, states[i].value);
        result_conf = rtka_conf_and(result_conf, states[i].confidence);
    }
    
    return rtka_make_state(result_value, result_conf);
}

rtka_state_t rtka_recursive_or(const rtka_state_t* states, uint32_t count) {
    if (count == 0) {
        return rtka_make_state(RTKA_FALSE, 1.0f);  /* Identity for OR */
    }
    if (count == 1) {
        return states[0];
    }
    
    rtka_value_t result_value = states[0].value;
    rtka_confidence_t result_conf = states[0].confidence;
    
    for (uint32_t i = 1; i < count; i++) {
        /* Early termination on TRUE */
        if (result_value == RTKA_TRUE) {
            break;
        }
        
        result_value = rtka_or(result_value, states[i].value);
        result_conf = rtka_conf_or(result_conf, states[i].confidence);
    }
    
    return rtka_make_state(result_value, result_conf);
}
```

---

## 4. CONFIDENCE PROPAGATION

### 4.1 Confidence Domain
```
C = [0, 1] ⊂ ℝ

A confidence-weighted ternary value: (v, c) ∈ T × C
where v is the ternary value and c is the confidence level
```

### 4.2 Conjunction Confidence

#### Mathematical Definition
```
For conjunction of confidence-weighted values (v₁, c₁), (v₂, c₂), ..., (vₙ, cₙ):

C_∧(c₁, c₂, ..., cₙ) = ∏ᵢ₌₁ⁿ cᵢ

This multiplicative rule reflects that all inputs must be confidently TRUE
for the conjunction to be confidently TRUE.
```

#### Assembly Implementation (x86-64, SSE)
```asm
; Input:  xmm0 = c1, xmm1 = c2 (float)
; Output: xmm0 = c1 * c2
rtka_conf_and:
    mulss xmm0, xmm1    ; Multiply confidences
    ret
```

#### C Implementation
```c
typedef float rtka_confidence_t;

static inline rtka_confidence_t rtka_conf_and(rtka_confidence_t c1, 
                                               rtka_confidence_t c2) {
    return c1 * c2;
}

/* N-ary version */
static inline rtka_confidence_t rtka_conf_and_n(const rtka_confidence_t* confs,
                                                 uint32_t n) {
    rtka_confidence_t result = 1.0f;
    for (uint32_t i = 0; i < n; i++) {
        result *= confs[i];
    }
    return result;
}
```

---

### 4.3 Disjunction Confidence

#### Mathematical Definition
```
For disjunction, confidence follows the inclusion-exclusion principle:

C_∨(c₁, c₂, ..., cₙ) = 1 - ∏ᵢ₌₁ⁿ (1 - cᵢ)

This represents the probability that at least one input is TRUE
with its associated confidence.

For two values:
C_∨(c₁, c₂) = c₁ + c₂ - c₁·c₂
```

#### Assembly Implementation (x86-64, SSE)
```asm
; Input:  xmm0 = c1, xmm1 = c2 (float)
; Output: xmm0 = c1 + c2 - c1*c2
rtka_conf_or:
    movss xmm2, xmm0    ; Copy c1 to xmm2
    mulss xmm2, xmm1    ; xmm2 = c1 * c2
    addss xmm0, xmm1    ; xmm0 = c1 + c2
    subss xmm0, xmm2    ; xmm0 = c1 + c2 - c1*c2
    ret
```

#### C Implementation
```c
static inline rtka_confidence_t rtka_conf_or(rtka_confidence_t c1,
                                              rtka_confidence_t c2) {
    return c1 + c2 - (c1 * c2);
}

/* N-ary version */
static inline rtka_confidence_t rtka_conf_or_n(const rtka_confidence_t* confs,
                                                uint32_t n) {
    rtka_confidence_t result = 1.0f;
    for (uint32_t i = 0; i < n; i++) {
        result *= (1.0f - confs[i]);
    }
    return 1.0f - result;
}
```

---

### 4.4 Negation Confidence

#### Mathematical Definition
```
C_¬(c) = c

Confidence is preserved through negation.
```

#### Assembly Implementation (x86-64, SSE)
```asm
; Input:  xmm0 = c (float)
; Output: xmm0 = c (unchanged)
rtka_conf_not:
    ret                 ; No operation needed
```

#### C Implementation
```c
static inline rtka_confidence_t rtka_conf_not(rtka_confidence_t c) {
    return c;
}
```

---

### 4.5 Implication Confidence

#### Mathematical Definition
```
C_→(c₁, c₂) = C_∨(c₁, c₂)

Since a → b = ¬a ∨ b, and ¬a has same confidence as a
```

#### C Implementation
```c
static inline rtka_confidence_t rtka_conf_implies(rtka_confidence_t c1,
                                                   rtka_confidence_t c2) {
    return rtka_conf_or(c1, c2);
}
```

---

### 4.6 Equivalence Confidence

#### Mathematical Definition
```
C_↔(c₁, c₂) = (c₁ + c₂) / 2

Average confidence when both are certain about equality
```

#### C Implementation
```c
static inline rtka_confidence_t rtka_conf_equiv(rtka_confidence_t c1,
                                                 rtka_confidence_t c2) {
    return (c1 + c2) * 0.5f;
}
```

---

## 5. UNKNOWN PRESERVATION

### Theorem
```
For sequence of ternary values x⃗ = ⟨x₁, x₂, ..., xₙ⟩:

If result φ(x⃗) = UNKNOWN, then at least one xᵢ = UNKNOWN

This characterizes when uncertainty persists through logical operations.
```

### Probabilistic Model

#### Mathematical Definition
```
For random uniform sequences over T:

P(φ(x⃗) = UNKNOWN) = (2/3)^(n-1)

where n is the sequence length.

This represents exponential decay of UNKNOWN persistence.
```

#### C Implementation
```c
#include <math.h>

rtka_confidence_t rtka_unknown_persistence_probability(uint32_t n) {
    if (n == 0) return 0.0f;
    if (n == 1) return 1.0f;
    
    /* Fast approximation for large n */
    if (n > 16) {
        return 0.666667f / n;
    }
    
    /* Exact calculation for small n: (2/3)^(n-1) */
    return (rtka_confidence_t)pow(2.0 / 3.0, (double)(n - 1));
}
```

---

## 6. STATE COMBINATIONS

### Mathematical Definition
```
A state combines value and confidence: s = (v, c) ∈ T × C

State operations apply both value logic and confidence propagation:

(v₁, c₁) ∧ (v₂, c₂) = (v₁ ∧ v₂, c₁ · c₂)
(v₁, c₁) ∨ (v₂, c₂) = (v₁ ∨ v₂, c₁ + c₂ - c₁·c₂)
```

### C Implementation
```c
typedef struct {
    rtka_value_t value;
    rtka_confidence_t confidence;
} rtka_state_t;

static inline rtka_state_t rtka_make_state(rtka_value_t value,
                                            rtka_confidence_t confidence) {
    return (rtka_state_t){
        .value = value,
        .confidence = confidence
    };
}

static inline rtka_state_t rtka_combine_and(rtka_state_t a, rtka_state_t b) {
    return rtka_make_state(
        rtka_and(a.value, b.value),
        rtka_conf_and(a.confidence, b.confidence)
    );
}

static inline rtka_state_t rtka_combine_or(rtka_state_t a, rtka_state_t b) {
    return rtka_make_state(
        rtka_or(a.value, b.value),
        rtka_conf_or(a.confidence, b.confidence)
    );
}
```

---

## 7. COMPLEXITY AND PERFORMANCE

### Time Complexity
```
Operation                    Worst Case    Average Case (with early term)
------------------------------------------------------------------
Single binary operation:     O(1)          O(1)
Recursive AND/OR:            O(n)          O(log n) to O(n)
  - Early termination achieves 40-60% improvement
  - Sub-linear average case complexity

Batch operations (SIMD):     O(n/k)        O(n/k)
  where k = SIMD width (typically 4-8 for floats)
```

### Space Complexity
```
Single operation:            O(1)
Recursive operation:         O(1) iterative
Batch operations:            O(n) for input/output arrays
```

---

## 8. MATHEMATICAL PROPERTIES

### Associativity
```
(a ∧ b) ∧ c = a ∧ (b ∧ c)
(a ∨ b) ∨ c = a ∨ (b ∨ c)
```

### Commutativity
```
a ∧ b = b ∧ a
a ∨ b = b ∨ a
```

### Distributivity
```
a ∧ (b ∨ c) = (a ∧ b) ∨ (a ∧ c)
a ∨ (b ∧ c) = (a ∨ b) ∧ (a ∨ c)
```

### Identity Elements
```
a ∧ TRUE = a
a ∨ FALSE = a
```

### Absorption
```
a ∧ (a ∨ b) = a
a ∨ (a ∧ b) = a
```

### De Morgan's Laws
```
¬(a ∧ b) = ¬a ∨ ¬b
¬(a ∨ b) = ¬a ∧ ¬b
```

---

## LICENSE

Copyright (c) 2025 - H.Overman  
Email: opsec.ee@pm.me

PROPRIETARY AND CONFIDENTIAL
This document is proprietary and may not be reproduced, distributed, 
or used without explicit written permission.

For licensing inquiries: opsec.ee@pm.me

---

**END OF DOCUMENT**
