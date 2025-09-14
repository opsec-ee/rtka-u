---
layout: default
title: Recursive Ternary with Kleene Algorithm + UNKNOWN
---

# Recursive Ternary with Kleene Algorithm + UNKNOWN (RTKA-U)

**By H. Overman (<opsec.ee@pm.me>) &copy; 2025**

RTKA-U is a framework that extends strong Kleene logic to recursively process ternary truth values \(\mathbb{T} = \{-1, 0, 1\}\) (FALSE, UNKNOWN, TRUE) with confidence propagation. It preserves uncertainty unless logically required, offering \(\mathcal{O}(n)\) time complexity and \(\mathcal{O}(1)\) space via tail recursion. Applications include AI inference, sensor fusion, and decision-making under uncertainty.

## Core Formulation

For operation \(\phi \in \{\land_r, \lor_r, \neg_r\}\) and inputs \(\vec{x} \in \mathbb{T}^n\):

\[
\mathcal{R}(\phi, \vec{x}) = 
\begin{cases} 
x_n & \text{if } n = 1 \\
\phi(x_1, \mathcal{R}(\phi, \langle x_2, \dots, x_n \rangle)) & \text{if } n > 1 
\end{cases}
\]

Confidence propagation for \(\vec{c} \in [0,1]^n\):

\[
\mathcal{C}(\phi, \vec{c}) = 
\begin{cases} 
\prod_{i=1}^n c_i & \text{if } \phi = \land_r \\
1 - \prod_{i=1}^n (1 - c_i) & \text{if } \phi = \lor_r \\
c_1 & \text{if } \phi = \neg_r
\end{cases}
\]

**UNKNOWN Preservation Theorem**: 

\[
\mathcal{R}(\phi, \langle 0, x_2, \dots, x_n \rangle) = 0 \iff \nexists x_i : (\phi = \land_r \land x_i = -1) \lor (\phi = \lor_r \land x_i = 1)
\]

## Implementation

The Python implementation is available in [ternary_logic.py](ternary_logic.py). Example test cases:
- AND: \([0, 1, -1]\), confidences \([0.8, 0.9, 0.7]\) → \(-1\), 0.504
- OR: \([0, -1, 1]\), confidences \([0.8, 0.9, 0.7]\) → \(1\), 0.964

## UNKNOWN Propagation

The probability of UNKNOWN persisting after \(n\) operations is approximately \((2/3)^{n-1}\):

![UNKNOWN Propagation Plot](unknown_propagation.png)

## Resources

- [Source Code](ternary_logic.py)
- [GitHub Repository](https://github.com/opse-ee/rtka-u)
- [arXiv Paper](https://arxiv.org/abs/XXXX.XXXXX) (forthcoming)

For feedback or collaboration, contact <opsec.ee@pm.me>.
