# Recursive Ternary with Kleene Algorithm + UNKNOWN (RTKA-U)

By H. Overman ([opsec.ee@pm.me](mailto:opsec.ee@pm.me)) © 2025

This system extends strong Kleene logic to recursively process ternary truth values **T = {-1, 0, 1}** (FALSE, UNKNOWN, TRUE) with confidence propagation, preserving uncertainty unless logically required. It is efficient (**O(n)** time, **O(1)** space) and applicable to AI, sensor fusion, and decision-making.

<h2>Mathematical Formulation</h2>
<div align="center">
  <img src="https://github.com/opsec-ee/rtka-u/blob/main/images/rtka-u.png" alt="RTKA-U Mathematical Formulations" />
</div>
<h2>Visualizations</h2>
<div align="center">
<img src="https://github.com/opsec-ee/rtka-u/blob/main/images/unknown_propagation.png" alt="UNKNOWN Propagation Plot" style="max-width: 100%;">
</div>

## Implementation

examples in [`rtka_u_algorithm`](code/rtka_u_algorithm.py)
            [`test_rtka_u`](code/test_rtka_u.py)

## Test cases (Python):

- AND: [0, 1, -1], confidences [0.8, 0.9, 0.7] → -1, 0.504
- OR: [0, -1, 1], confidences [0.8, 0.9, 0.7] → 1, 0.964

## Resources

- [Source Code](ternary_logic.py)
- [GitHub Repository](https://github.com/yourusername/rtka-u)
- [arXiv Paper](https://arxiv.org/abs/XXXX.XXXXX) (forthcoming)

For feedback or collaboration, contact <opsec.ee@pm.me>.
