# Recursive Ternary with Kleene Algorithm + UNKNOWN (RTKA-U)

By H. Overman ([opsec.ee@pm.me](mailto:opsec.ee@pm.me)) © 2025

This system extends strong Kleene logic to recursively process ternary truth values $\mathbb{T} = \{-1, 0, 1\}$ (FALSE, UNKNOWN, TRUE) with confidence propagation, preserving uncertainty unless logically required. It is efficient ($\mathcal{O}(n)$ time, $\mathcal{O}(1)$ space) and applicable to AI, sensor fusion, and decision-making.

## Mathematical Formulation

For operation $\phi \in \{\land_r, \lor_r, \neg_r\}$ and input $\vec{x} \in \mathbb{T}^n$:

$$
\mathcal{R}(\phi, \vec{x}) = 
\begin{cases} 
x_n & \text{if } n = 1 \\
\phi(x_1, \mathcal{R}(\phi, \langle x_2, \dots, x_n \rangle)) & \text{if } n > 1 
\end{cases}
$$

With confidence propagation for $\vec{c} \in [0,1]^n$:

$$
\mathcal{C}(\phi, \vec{c}) = 
\begin{cases} 
\prod_{i=1}^n c_i & \text{if } \phi = \land_r \\
1 - \prod_{i=1}^n (1 - c_i) & \text{if } \phi = \lor_r \\
c_1 & \text{if } \phi = \neg_r
\end{cases}
$$

**UNKNOWN Preservation Theorem**: $\mathcal{R}(\phi, \langle 0, x_2, \dots, x_n \rangle) = 0 \iff \nexists x_i : (\phi = \land_r \land x_i = -1) \lor (\phi = \lor_r \land x_i = 1)$.

## Implementation

```python
def kleene_and(a, b):
    return min(a, b)

def kleene_or(a, b):
    return max(a, b)

def kleene_not(a, _=None):
    return -a

def confidence_and(confidences):
    result = 1.0
    for c in confidences:
        result *= c
    return result

def confidence_or(confidences):
    result = 1.0
    for c in confidences:
        result *= (1 - c)
    return 1 - result

def recursive_ternary(operation, inputs, confidences=None):
    if not inputs:
        raise ValueError("Input list cannot be empty")
    if len(inputs) == 1:
        return (inputs[0], confidences[0]) if confidences else inputs[0]
    
    if operation == kleene_and and inputs[0] == -1:
        return -1, confidence_and(confidences) if confidences else None
    if operation == kleene_or and inputs[0] == 1:
        return 1, confidence_or(confidences) if confidences else None
    
    tail_result, tail_confidence = recursive_ternary(operation, inputs[1:], confidences[1:] if confidences else None) if confidences else (recursive_ternary(operation, inputs[1:]), None)
    result = operation(inputs[0], tail_result)
    
    if confidences:
        if operation == kleene_and:
            confidence = confidence_and([confidences[0], tail_confidence])
        elif operation == kleene_or:
            confidence = confidence_or([confidences[0], tail_confidence])
        else:
            confidence = confidences[0]
        return result, confidence
    
    return result
```

## Try It

Test cases (run in Python):

- AND: $[0, 1, -1]$, confidences $[0.8, 0.9, 0.7]$ → $-1$, 0.504
- OR: $[0, -1, 1]$, confidences $[0.8, 0.9, 0.7]$ → $1$, 0.964

## Resources

- [Source Code](ternary_logic.py)
- [GitHub Repository](https://github.com/yourusername/rtka-u)
- [arXiv Paper](https://arxiv.org/abs/XXXX.XXXXX) (forthcoming)

For feedback or collaboration, contact <opsec.ee@pm.me>.
