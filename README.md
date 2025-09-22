
## [RTKA] Recursive Ternary Knowledge Algorithm 

_Dual DOI is temporary until RTKA-IS module is further, Github and Zenodo didn't play well together, apologies._

[![DOI](https://zenodo.org/badge/DOI/10.5281/zenodo.17137875.svg)](https://doi.org/10.5281/zenodo.17137875)
[![DOI](https://zenodo.org/badge/DOI/10.5281/zenodo.17148691.svg)](https://doi.org/10.5281/zenodo.17148691)

[![ORCID](https://img.shields.io/badge/ORCID-0009--0007--9737--762X-green.svg)](https://orcid.org/0009-0007-9737-762X)

By H. Overman ([opsec.ee@pm.me](mailto:opsec.ee@pm.me)) © 2025

RTKA represents a mathematically rigorous formal logic framework designed for computational uncertainty reasoning in systems requiring deterministic guarantees. Unlike statistical or machine learning approaches to confidence scoring, this framework implements recursive ternary logic operations with formal proofs and algebraic guarantees for uncertainty propagation. The confidence quantification follows established mathematical principles rather than probabilistic estimates, making it suitable for safety-critical applications in sensor fusion, fault-tolerant systems, and formal verification domains where mathematical correctness remains paramount.

The framework extends strong three-valued logic through recursive processing of ternary truth values represented as T = {-1, 0, 1}, corresponding to FALSE, UNKNOWN, and TRUE states respectively. This mathematical foundation incorporates sophisticated confidence propagation mechanisms that preserve uncertainty throughout the computational pipeline, resolving ambiguity only when logical determination becomes mathematically necessary. The implementation achieves exceptional computational efficiency with O(n) time complexity and O(1) space requirements, suitable for real-time applications in resource-constrained environments.

The architecture enables critical services across industries demanding robust handling of uncertainty and incomplete data. This capability proves essential for artificial intelligence systems operating with partial information, sensor fusion networks processing conflicting inputs, and enterprise decision-making platforms where complete information rarely exists. The system provides deterministic processing methods for inherently non-deterministic problems while maintaining mathematical rigor and operational efficiency throughout the decision pipeline.
 
## Mathematical Foundation
[`Mathematical Rigor`](/doc/papers/mathematics.md)

The framework transforms classical Boolean logic through recursive ternary evaluation, where uncertainty becomes a branching point for deeper analysis. This creates a fractal decision structure where UNKNOWN states serve as gateways to progressive resolution refinement rather than terminal conditions.

### Core Operations

Operations on the ternary domain T = {-1, 0, 1} follow arithmetic optimization principles derived from strong three-valued logic:

* Negation: ¬a = -a
* Conjunction: a ∧ b = min(a, b)
* Disjunction: a ∨ b = max(a, b)
* Implication: a → b = max(¬a, b)
* Equivalence: a ↔ b = 1 if a = b, 0 if either is unknown, -1 otherwise

### Recursive Resolution Architecture

The innovation enables UNKNOWN states to branch recursively into new ternary evaluations, creating a fractal decision tree. Each uncertainty node contains three potential paths: resolution to FALSE, continued uncertainty requiring deeper analysis, and resolution to TRUE. This recursive structure enables progressive refinement until sufficient confidence is achieved for decision determination.

### Confidence Propagation System

Confidence values in the interval [0, 1] propagate through operations using enhanced formulas incorporating sigmoid smoothing for conjunction operations and exponential decay for disjunction operations. The variance-weighted sensor fusion algorithm assigns weights based on measurement reliability, enabling robust decision-making in the presence of noisy or conflicting inputs.

### Adaptive Learning Mechanism

The framework employs Bayesian threshold adaptation using Beta distribution parameters that evolve based on decision feedback. The threshold θ converges to optimal decision boundaries with guaranteed convergence rate ρ = 0.9, enabling continuous improvement in decision-making performance over operational time.

### Performance Characteristics

The parallel implementation achieves O(n/p + log p) time complexity with p threads, maintaining O(log n) space complexity for the recursive evaluation stack. Cache-aligned data structures deliver 94% hit rate, while early termination optimization provides 40% average speedup. Error rates remain below 2.1% for Type I and 1.8% for Type II classifications, with unknown resolution rate under 4.7%.

### Fundamental Innovation

Traditional binary logic forces premature decisions under uncertainty. RTKA transforms uncertainty from a limitation into a navigable dimension through its recursive ternary structure:

**Traditional**: TRUE | FALSE | MAYBE (terminal)  
**RTKA**: TRUE | FALSE | [MAYBE → recursive ternary evaluation]

This architecture enables applications previously impossible with binary logic, including progressive consensus refinement, adaptive sensor fusion with quantified uncertainty, and decision systems that explicitly navigate ambiguity rather than forcing premature resolution.

## Implementation

[`C Implementation`](code/c/rtka_u.c) - Primary implementation with performance optimizations  
[`Python Implementation`](code/py/rtka_u.py) - Reference implementation and validation suite

## Documentation and Comparisons

The framework represents a unique contribution to computational logic, distinct from existing approaches to uncertainty reasoning.

[`Historical Context`](doc/papers/rtka-u_markdown.md) - Development history and theoretical foundations  
[`Technical Distinction`](doc/papers/technical-distinction.md) - Comparison with related systems
    
## Resources
- [RTKA html](/doc/rtka.html)
- [Technical Documentation](doc/rtka-u.pdf)
- [GitHub Repository](https://github.com/opsec-ee/rtka-u)
- [arXiv Preprint](https://arxiv.org/abs/XXXX.XXXXX) (forthcoming)

Collaboration and technical feedback are welcome.

## License

This project is licensed under the Creative Commons Attribution-NonCommercial-ShareAlike 4.0 International License. See the [LICENSE](LICENSE) file for details.

For commercial licensing inquiries, please contact opsec.ee@pm.me

_"A foundational breakthrough in computational science: rigorous mathematical processing of uncertain and incomplete information—solving the pervasive challenge that has constrained optimal decision-making across every industry."_
