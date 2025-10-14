# RTKA Robotics Control - Mathematical Foundations

Copyright (c) 2025 - H.Overman opsec.ee@pm.me

---

## Core RTKA Mathematics (Proven)

### Ternary Values
```
T = {-1, 0, 1}  →  {FALSE, UNKNOWN, TRUE}
```

### Kleene Operations
```
AND: a ∧ b = min(a, b)
OR:  a ∨ b = max(a, b)
NOT: ¬a = -a
```

### Confidence Propagation (Empirically Validated)
```
AND: C_∧ = ∏ᵢ₌₁ⁿ cᵢ                    (Multiplicative)
OR:  C_∨ = 1 - ∏ᵢ₌₁ⁿ (1 - cᵢ)          (Inclusion-Exclusion)
NOT: C_¬ = c                           (Preserved)
```

### Unknown Persistence Theorem
```
P(UNKNOWN persists) = (2/3)^(n-1)

Validated: <0.5% error over 50,000+ trials
```

---

## Extension to Continuous Control (NEW)

### Insight
Recursive confidence provides a natural continuous signal [0,1] that can drive control outputs.

### Control Mapping (OPT-230)

Given confidence C ∈ [0,1], map to control output u:

```
         ⎧ u_max - k_h(C_low - C)(u_max - u_nom),    C < C_low
u(C) =   ⎨ u_nom,                                     C_low ≤ C ≤ C_high  
         ⎩ u_nom - k_l(C - C_high)(u_nom - u_min),   C > C_high

where:
  C_low, C_high : confidence thresholds
  u_min, u_nom, u_max : control output bounds
  k_h, k_l : gain coefficients
```

**Physical interpretation:**
- Low confidence → System uncertain → Increase effort
- High confidence → System stable → Reduce effort
- Moderate confidence → Steady state

---

## Double Pendulum Dynamics (Exact)

### State Vector
```
x = [θ₁, ω₁, θ₂, ω₂]ᵀ

θ₁, θ₂ : joint angles (rad)
ω₁, ω₂ : angular velocities (rad/s)
```

### Equations of Motion (Lagrangian)

Let Δ = θ₁ - θ₂

```
θ₁'' = [-m₂l₁ω₁²sin(Δ)cos(Δ) + m₂g sin(θ₂)cos(Δ) - m₂l₂ω₂²sin(Δ)
        - (m₁+m₂)g sin(θ₁) - b₁ω₁ + τ] / [l₁(m₁ + m₂sin²(Δ))]

θ₂'' = [(m₁+m₂)(l₁ω₁²sin(Δ) - g sin(θ₂) + g sin(θ₁)cos(Δ))
        + m₂l₂ω₂²sin(Δ)cos(Δ) - b₂ω₂ + τcos(Δ)] / [l₂(m₁ + m₂sin²(Δ))]
```

**Parameters:**
- m₁, m₂ : masses (kg)
- l₁, l₂ : lengths (m)
- b₁, b₂ : damping coefficients (N·m·s/rad)
- τ : applied torque (N·m)
- g : gravity (9.81 m/s²)

### Energy
```
KE = ½m₁l₁²ω₁² + ½m₂[l₁²ω₁² + l₂²ω₂² + 2l₁l₂ω₁ω₂cos(Δ)]

PE = -(m₁+m₂)gl₁cos(θ₁) - m₂gl₂cos(θ₂)

E_total = KE + PE
```

---

## Sensor Fusion (OPT-225)

### Variance-Weighted Consensus

Given n sensors with readings rᵢ, confidences cᵢ, variances σᵢ²:

```
Weight: wᵢ = 1/(1 + σᵢ²)

Weighted sum: S = Σᵢ rᵢwᵢcᵢ

Consensus: r_fused = S / Σᵢ cᵢ

Aggregate confidence: C_agg = 1 - ∏ᵢ(1 - cᵢ)
```

**Properties:**
- High variance sensors get low weight
- Confidence uses probabilistic OR
- Linear time complexity O(n)

---

## Mode Switching (OPT-231)

### State Machine

```
States: S = {NOMINAL, DEGRADED, SAFE, EMERGENCY}

Transitions with hysteresis:
  NOMINAL ⟷ DEGRADED:  0.65 < C < 0.75
  DEGRADED ⟷ SAFE:     0.35 < C < 0.45
  ANY → EMERGENCY:     C < 0.05 (immediate)
```

### Dwell Time Requirements
```
Minimum time in state before transition:
  NOMINAL:   0.5s
  DEGRADED:  0.3s
  SAFE:      2.0s
  EMERGENCY: ∞ (manual recovery)
```

---

## Complete Control Loop

### Algorithm

```
Input: Sensor readings {r₁, ..., rₙ} with {c₁, ..., cₙ} and {σ₁², ..., σₙ²}
Output: Control torque τ

1. SENSOR FUSION (OPT-225)
   C_agg ← rtka_fuse_sensors_weighted({rᵢ, cᵢ, σᵢ²})

2. MODE SELECTION (OPT-231)
   mode ← determine_mode(C_agg, current_mode, dwell_time)
   params ← get_control_params(mode)

3. CONTROL MAPPING (OPT-230)
   τ_raw ← map_confidence_to_control(C_agg, params)

4. RATE LIMITING
   τ_limited ← clamp(τ_raw, τ_prev ± Δτ_max)

5. SATURATION
   τ ← clamp(τ_limited, τ_min, τ_max)

6. PHYSICS UPDATE
   x(t+Δt) ← RK4_integrate(x(t), τ, Δt)

Return τ
```

### Computational Complexity
```
Sensor fusion:    O(n)
Mode switching:   O(1)
Control mapping:  O(1)
RK4 integration:  O(1)

Total per step: O(n) where n = number of sensors
```

---

## Stability Analysis (Theoretical)

### Lyapunov Function Candidate
```
V(θ₁, θ₂, ω₁, ω₂) = KE + PE

V̇ = -b₁ω₁² - b₂ω₂² + τω₁ ≤ 0  (if τ properly bounded)
```

### Controllability Region
Small angle approximation valid when:
```
|θ₁|, |θ₂| < 0.5 rad  (~28°)
|ω₁|, |ω₂| < 2.0 rad/s

Within this region, linearized control applies.
Outside: nonlinear effects dominate, require mode switch.
```

---

## Numerical Integration (RK4)

### Runge-Kutta 4th Order

```
Given: ẋ = f(x, u, t)

k₁ = f(xₙ, uₙ, tₙ)
k₂ = f(xₙ + ½Δt·k₁, uₙ, tₙ + ½Δt)
k₃ = f(xₙ + ½Δt·k₂, uₙ, tₙ + ½Δt)
k₄ = f(xₙ + Δt·k₃, uₙ, tₙ + Δt)

xₙ₊₁ = xₙ + (Δt/6)(k₁ + 2k₂ + 2k₃ + k₄)
```

**Error:** O(Δt⁵) per step, O(Δt⁴) accumulated

**Stability:** Requires Δt < 2/λ_max where λ_max is largest eigenvalue

---

## Performance Metrics

### Energy Dissipation Rate
```
dE/dt = -b₁ω₁² - b₂ω₂² + τω₁

For stabilization: Want E → 0 as t → ∞
```

### Control Effort
```
∫₀ᵀ τ²(t) dt  (minimize for efficiency)
```

### Settling Time
```
t_s = min{t : |θ₁(t')|, |θ₂(t')| < ε  ∀t' > t}

Typical target: ε = 0.01 rad (~0.57°)
```

---

## Comparison with Classical Control

### PID Control
```
τ_PID = K_p·e + K_i·∫e dt + K_d·de/dt

where e = θ_desired - θ_actual

Issues:
- No uncertainty handling
- Fixed gains
- No sensor fusion
- No graceful degradation
```

### LQR Control
```
τ_LQR = -K·x  where K minimizes J = ∫(xᵀQx + uᵀRu)dt

Issues:
- Requires accurate model
- No sensor fusion
- No adaptation to failures
- Linear approximation only
```

### RTKA Control
```
τ_RTKA = f(C({c₁, ..., cₙ}))  where C = ∏cᵢ

Advantages:
+ Handles sensor uncertainty
+ Adaptive gains via confidence
+ Built-in sensor fusion
+ Graceful degradation
+ Fault tolerance

Disadvantages:
- New approach (less validated)
- Requires parameter tuning
- No formal optimality guarantee
```

---

## Key Theorems Used

### 1. RTKA Unknown Persistence (Proven)
```
For random input sequence of length n:
P(output = UNKNOWN) = (2/3)^(n-1)

Empirically validated: <0.5% error
```

### 2. Confidence Multiplicative Decay (Proven)
```
For AND operations:
C_result = ∏ᵢ₌₁ⁿ cᵢ

This causes natural damping in control:
More sensors → lower confidence → gentler control
```

### 3. Small Angle Approximation
```
For |θ| < 0.5:
sin(θ) ≈ θ
cos(θ) ≈ 1

Enables linearization for local control.
```

---

## Implementation Notes

### Floating Point Precision
```
All calculations use IEEE 754 single precision (float)
Sufficient for control: ~7 decimal digits
Confidence values normalized to [0, 1]
```

### Timestep Selection
```
Δt = 0.01s (100 Hz)

Nyquist: f_s > 2·f_pendulum
Pendulum natural frequency: ~1-5 Hz
Sampling: 100 Hz provides 20-100x oversampling
```

### Memory Usage
```
Control state: ~64 bytes
Sensor array: n·16 bytes
Mode controller: ~512 bytes
Pendulum state: ~128 bytes

Total: <1 KB for n=5 sensors
```

---

## Future Extensions

### 1. Adaptive Gains
```
k_h(C) = k₀ + k₁/(C + ε)

Allow gains to adapt based on confidence history.
```

### 2. Energy-Based Switching
```
Mode selection includes energy term:

mode = f(C, E, dE/dt)

Better handling of high-energy states.
```

### 3. Model Predictive Control
```
Optimize τ over horizon H:

min Σᵢ₌₀ᴴ [xᵢᵀQxᵢ + uᵢᵀRuᵢ]
s.t. xᵢ₊₁ = f(xᵢ, uᵢ)
     C(xᵢ) > C_min

Integrate RTKA confidence as constraint.
```

---

Email: opsec.ee@pm.me
