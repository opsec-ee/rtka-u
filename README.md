## Recursive Ternary with Kleene Algorithm + UNKNOWN (RTKA-U)

By H. Overman ([opsec.ee@pm.me](mailto:opsec.ee@pm.me)) © 2025

This system extends strong Kleene logic to recursively process ternary truth values **T = {-1, 0, 1}** (FALSE, UNKNOWN, TRUE) with confidence propagation, preserving uncertainty unless logically required. It is efficient (**O(n)** time, **O(1)** space) and applicable to AI, sensor fusion, and decision-making.

RTKA-U’s ternary logic and confidence propagation enable services that require robust handling of uncertainty, incomplete data, or probabilistic reasoning.

<h2>Mathematical Formulation</h2>
<div align="center">
  <img src="https://github.com/opsec-ee/rtka-u/blob/main/images/rtka_u-v3.png" alt="RTKA-U Mathematical Formulations" />
</div>

## Implementation

[`rtka_u in Python`](code/py) \
[`rtka-u in C code`](code/c)


## Resources

- [Source Code](code/)
- [GitHub Repository](https://github.com/opsec-ee/rtka-u)
- [arXiv Paper](https://arxiv.org/abs/XXXX.XXXXX) (forthcoming)

For feedback or collaboration, contact <opsec.ee@pm.me>.

## License

This project is licensed under the Creative Commons Attribution-NonCommercial-ShareAlike 4.0 International License \
see the [LICENSE](LICENSE) file for details.

For commercial licensing, please contact opsec.ee@pm.me
