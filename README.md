## (RTKA-U) Recursive Ternary with Kleene Algorithm + UNKNOWN 

[![DOI](https://zenodo.org/badge/DOI/10.5281/zenodo.17148691.svg)](https://doi.org/10.5281/zenodo.17148691)

By H. Overman ([opsec.ee@pm.me](mailto:opsec.ee@pm.me)) © 2025

The RTKA-U framework implements an advanced extension of strong Kleene logic, \
enabling recursive processing of ternary truth values represented as T = {-1, 0, 1}, \
corresponding to FALSE, UNKNOWN, and TRUE states respectively. This mathematical \
foundation incorporates sophisticated confidence propagation mechanisms that preserve \
uncertainty throughout the computational pipeline, only resolving ambiguity when \
logical determination becomes mathematically necessary. The implementation achieves \
exceptional computational efficiency with O(n) time complexity and O(1) space \
requirements, making it suitable for real-time applications in resource-constrained \
environments.

The framework's ternary logic architecture combined with confidence propagation \
enables critical services across industries that demand robust handling of uncertainty, \
incomplete data, and probabilistic reasoning. This capability proves essential for \
artificial intelligence systems operating with partial information, sensor fusion \
networks processing noisy or conflicting inputs, and enterprise decision-making \
platforms where complete information rarely exists. The system transforms how \
organizations approach computational challenges involving ambiguity, providing \
deterministic processing methods for inherently non-deterministic problems while \
maintaining mathematical rigor and operational efficiency throughout the decision \
pipeline.

<h2>Mathematical Formulation</h2>
<div align="center">
  <img src="https://github.com/opsec-ee/rtka-u/blob/main/images/rtka_u-v4.png" alt="RTKA-U Mathematical Formulations" />
</div>

## Implementation

[`rtka-u in C code`](code/c/rtka_u_core_parallel_enhanced-v1_3.c) - Main \
[`rtka-u in Python`](code/py) - Proof


## Historic Data

RTKA-U is **not** like any existing system. [`Here's how it's fundamentally different`](doc/papers/rtka-u_markdown.md)

## Resources

- [Documentation](doc/rtka-u.pdf)
- [GitHub Repository](https://github.com/opsec-ee/rtka-u)
- [arXiv Paper](https://arxiv.org/abs/XXXX.XXXXX) (forthcoming)

feedback or collaboration are welcome

## License

This project is licensed under the Creative Commons Attribution-NonCommercial-ShareAlike 4.0 International License \
see the [LICENSE](LICENSE) file for details.

For commercial licensing, please contact opsec.ee@pm.me

_"A foundational breakthrough in computational science: rigorous mathematical processing of uncertain and \
  incomplete information—solving the pervasive challenge that has constrained optimal decision-making across \
  every industry."_
