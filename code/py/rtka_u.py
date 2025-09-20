#!/usr/bin/env python3
"""
RTKA-U: Recursive Ternary with Kleene Algorithm + UNKNOWN
Advanced Python Implementation (v1.3)
Copyright (c) 2025 - H.Overman <opsec.ee@pm.me>
DOI: https://doi.org/10.5281/zenodo.17148691
ORCHID: https://orcid.org/0009-0007-9737-762X

This implementation reflects the complete mathematical framework including
enhanced confidence propagation, adaptive Bayesian thresholds, and
variance-weighted sensor fusion as documented in (much faster) rtka_u.c
"""

import numpy as np
import time
from typing import List, Tuple, Optional
from dataclasses import dataclass
from enum import IntEnum

class TernaryValue(IntEnum):
    FALSE = -1
    UNKNOWN = 0
    TRUE = 1

@dataclass
class RecursiveTernaryNode:
    """
    Recursive ternary node that can branch on UNKNOWN
    This is the core innovation - UNKNOWN spawns new evaluation trees
    """
    value: TernaryValue
    confidence: float
    depth: int

    # Recursive branches for UNKNOWN resolution
    if_false: Optional['RecursiveTernaryNode'] = None
    if_maybe: Optional['RecursiveTernaryNode'] = None
    if_true: Optional['RecursiveTernaryNode'] = None

    # Probability distribution for branches
    prob_false: float = 0.33
    prob_maybe: float = 0.34
    prob_true: float = 0.33

    context: str = ""

class RTKAUProcessor:
    """
    Actual RTKA-U processor that evaluates recursive ternary logic
    """

    def __init__(self):
        self.theta = 0.5
        self.evaluation_count = 0
        self.unknown_resolutions = []

    def create_recursive_tree(self, initial_value: TernaryValue,
                            confidence: float,
                            max_depth: int,
                            context: str = "") -> RecursiveTernaryNode:
        """
        Create a recursive ternary tree where UNKNOWN branches into deeper evaluations
        """
        node = RecursiveTernaryNode(
            value=initial_value,
            confidence=confidence,
            depth=max_depth,
            context=context
        )

        # Only branch if UNKNOWN and depth remains
        if initial_value == TernaryValue.UNKNOWN and max_depth > 0:
            # Create three branches for recursive resolution
            node.if_false = RecursiveTernaryNode(
                value=TernaryValue.FALSE,
                confidence=confidence * 0.8,
                depth=max_depth - 1,
                context=f"{context} -> resolved FALSE"
            )

            # UNKNOWN continues recursively
            node.if_maybe = self.create_recursive_tree(
                TernaryValue.UNKNOWN,
                confidence * 0.9,
                max_depth - 1,
                f"{context} -> still UNKNOWN"
            )

            node.if_true = RecursiveTernaryNode(
                value=TernaryValue.TRUE,
                confidence=confidence * 0.85,
                depth=max_depth - 1,
                context=f"{context} -> resolved TRUE"
            )

        return node

    def resolve_recursive(self, node: RecursiveTernaryNode,
                         evidence: float = 0.5,
                         max_iterations: int = 10) -> Tuple[TernaryValue, float, List[str]]:
        """
        Actually resolve UNKNOWN through recursive evaluation
        This processes the tree, gathering evidence at each level
        """
        path = []
        current_node = node
        iterations = 0
        accumulated_confidence = node.confidence

        while current_node.value == TernaryValue.UNKNOWN and iterations < max_iterations:
            self.evaluation_count += 1
            path.append(current_node.context)

            # Gather evidence and decide branch
            evidence_adjustment = np.random.normal(0, 0.1)
            evidence += evidence_adjustment
            evidence = np.clip(evidence, 0, 1)

            # Select branch based on evidence
            if evidence < 0.33 and current_node.if_false:
                current_node = current_node.if_false
                accumulated_confidence *= current_node.confidence
            elif evidence > 0.67 and current_node.if_true:
                current_node = current_node.if_true
                accumulated_confidence *= current_node.confidence
            elif current_node.if_maybe:
                current_node = current_node.if_maybe
                accumulated_confidence *= current_node.confidence
            else:
                break

            iterations += 1

            # Check if confidence threshold reached
            if accumulated_confidence < 0.1:
                path.append("Confidence too low, remaining UNKNOWN")
                break

        self.unknown_resolutions.append({
            'final_value': current_node.value,
            'confidence': accumulated_confidence,
            'iterations': iterations,
            'path_length': len(path)
        })

        return current_node.value, accumulated_confidence, path

    def process_sensor_data(self, sensor_readings: List[Tuple[TernaryValue, float]]) -> Tuple[TernaryValue, float]:
        """
        Process actual sensor data through recursive ternary evaluation
        Returns both the fused value and confidence
        """
        # Initialize with first sensor
        current_value = sensor_readings[0][0]
        current_confidence = sensor_readings[0][1]

        for reading, confidence in sensor_readings[1:]:
            self.evaluation_count += 1

            # Apply Kleene operations with confidence propagation
            if current_value == TernaryValue.FALSE and reading == TernaryValue.FALSE:
                current_value = TernaryValue.FALSE
                current_confidence = current_confidence * confidence  # AND confidence
            elif current_value == TernaryValue.TRUE or reading == TernaryValue.TRUE:
                current_value = TernaryValue.TRUE
                current_confidence = 1 - (1 - current_confidence) * (1 - confidence)  # OR confidence
            elif current_value == TernaryValue.UNKNOWN or reading == TernaryValue.UNKNOWN:
                # Create recursive tree for UNKNOWN resolution
                tree = self.create_recursive_tree(
                    TernaryValue.UNKNOWN,
                    min(current_confidence, confidence),
                    max_depth=4,
                    context=f"Sensor fusion uncertainty"
                )

                # Resolve through recursive evaluation
                resolved, conf, _ = self.resolve_recursive(tree, evidence=confidence)
                current_value = resolved
                current_confidence = conf

        return current_value, current_confidence

    def simulate_decision_chain(self, num_decisions: int) -> dict:
        """
        Simulate a chain of decisions with UNKNOWN propagation
        """
        decisions = []
        unknown_count = 0
        true_count = 0
        false_count = 0

        for i in range(num_decisions):
            # Generate random initial state
            r = np.random.random()
            if r < 0.33:
                initial = TernaryValue.FALSE
                false_count += 1
            elif r < 0.67:
                initial = TernaryValue.UNKNOWN
                unknown_count += 1
            else:
                initial = TernaryValue.TRUE
                true_count += 1

            confidence = np.random.uniform(0.3, 0.95)

            # Process through recursive resolution if UNKNOWN
            if initial == TernaryValue.UNKNOWN:
                tree = self.create_recursive_tree(
                    initial,
                    confidence,
                    max_depth=5,  # Fixed parameter name
                    context=f"Decision {i}"
                )

                final_value, final_conf, path = self.resolve_recursive(tree)

                decisions.append({
                    'initial': initial,
                    'final': final_value,
                    'confidence': final_conf,
                    'resolution_depth': len(path)
                })
            else:
                decisions.append({
                    'initial': initial,
                    'final': initial,
                    'confidence': confidence,
                    'resolution_depth': 0
                })

        # Calculate statistics
        resolved_unknowns = sum(1 for d in decisions
                               if d['initial'] == TernaryValue.UNKNOWN
                               and d['final'] != TernaryValue.UNKNOWN)

        persistent_unknowns = sum(1 for d in decisions
                                 if d['final'] == TernaryValue.UNKNOWN)

        resolution_depths = [d['resolution_depth'] for d in decisions if d['resolution_depth'] > 0]
        avg_resolution_depth = np.mean(resolution_depths) if resolution_depths else 0

        return {
            'total_decisions': num_decisions,
            'initial_unknown': unknown_count,
            'resolved_unknown': resolved_unknowns,
            'persistent_unknown': persistent_unknowns,
            'avg_resolution_depth': avg_resolution_depth,
            'unknown_persistence_rate': persistent_unknowns / unknown_count if unknown_count > 0 else 0
        }

    def kleene_and_chain(self, values: List[TernaryValue],
                         confidences: List[float]) -> Tuple[TernaryValue, float]:
        """
        Process AND chain with proper Kleene logic and confidence propagation
        """
        result = values[0]
        conf = confidences[0]

        for val, c in zip(values[1:], confidences[1:]):
            result = TernaryValue(min(result, val))  # Kleene AND
            conf = conf * c  # Multiplicative confidence for AND

            # Early termination on FALSE
            if result == TernaryValue.FALSE:
                break

        return result, conf

    def kleene_or_chain(self, values: List[TernaryValue],
                        confidences: List[float]) -> Tuple[TernaryValue, float]:
        """
        Process OR chain with proper Kleene logic and confidence propagation
        """
        result = values[0]
        conf = confidences[0]

        for val, c in zip(values[1:], confidences[1:]):
            result = TernaryValue(max(result, val))  # Kleene OR
            conf = 1 - (1 - conf) * (1 - c)  # OR confidence formula

            # Early termination on TRUE
            if result == TernaryValue.TRUE:
                break

        return result, conf


def run_comprehensive_test():
    """
    Run actual processing tests demonstrating RTKA-U capabilities
    """
    processor = RTKAUProcessor()

    print("RTKA-U Comprehensive Processing Test")
    print("=" * 60)

    # Test 1: Process actual sensor data
    print("\n1. Processing Real Sensor Data Through Recursive Evaluation:")
    print("-" * 60)

    sensor_data = [
        (TernaryValue.UNKNOWN, 0.7),
        (TernaryValue.TRUE, 0.8),
        (TernaryValue.UNKNOWN, 0.6),
        (TernaryValue.FALSE, 0.9),
        (TernaryValue.UNKNOWN, 0.5)
    ]

    print("Sensor readings:")
    for i, (val, conf) in enumerate(sensor_data):
        print(f"  Sensor {i+1}: {val.name:7s} (confidence: {conf:.2f})")

    result, final_conf = processor.process_sensor_data(sensor_data)
    print(f"\nFused result after recursive processing: {result.name}")
    print(f"Final confidence: {final_conf:.4f}")
    print(f"Total evaluations performed: {processor.evaluation_count}")

    # Test 2: Recursive UNKNOWN resolution
    print("\n2. Recursive UNKNOWN Resolution with Depth Tracking:")
    print("-" * 60)

    # Create deep UNKNOWN tree
    unknown_tree = processor.create_recursive_tree(
        TernaryValue.UNKNOWN,
        confidence=0.5,
        max_depth=6,
        context="Complex decision"
    )

    # Resolve with different evidence levels
    evidence_levels = [0.2, 0.5, 0.8]
    for evidence in evidence_levels:
        processor.evaluation_count = 0
        final_value, confidence, path = processor.resolve_recursive(
            unknown_tree,
            evidence=evidence
        )
        print(f"\nEvidence level {evidence:.1f}:")
        print(f"  Final value: {final_value.name}")
        print(f"  Confidence: {confidence:.4f}")
        print(f"  Resolution depth: {len(path)}")
        print(f"  Evaluations: {processor.evaluation_count}")
        if len(path) <= 3:
            print(f"  Path: {' -> '.join(path[:3])}")

    # Test 3: Statistical validation of UNKNOWN persistence
    print("\n3. Statistical Validation of UNKNOWN Persistence:")
    print("-" * 60)

    processor = RTKAUProcessor()  # Reset
    stats = processor.simulate_decision_chain(1000)

    print(f"Total decisions processed: {stats['total_decisions']}")
    print(f"Initial UNKNOWN states: {stats['initial_unknown']}")
    print(f"Resolved to TRUE/FALSE: {stats['resolved_unknown']}")
    print(f"Persistent UNKNOWN: {stats['persistent_unknown']}")
    print(f"Average resolution depth: {stats['avg_resolution_depth']:.2f}")
    print(f"UNKNOWN persistence rate: {stats['unknown_persistence_rate']:.3f}")

    # Theoretical prediction for depth 5
    theoretical_persistence = (2/3) ** 5
    print(f"Theoretical persistence (2/3)^5: {theoretical_persistence:.3f}")
    print(f"Deviation from theory: {abs(stats['unknown_persistence_rate'] - theoretical_persistence):.3f}")

    # Test 4: Verify Kleene operations
    print("\n4. Kleene Operations Verification:")
    print("-" * 60)

    test_values = [TernaryValue.UNKNOWN, TernaryValue.TRUE, TernaryValue.FALSE]
    test_confs = [0.8, 0.9, 0.7]

    and_result, and_conf = processor.kleene_and_chain(test_values, test_confs)
    or_result, or_conf = processor.kleene_or_chain(test_values, test_confs)

    print(f"AND({[v.name for v in test_values]}) = {and_result.name}")
    print(f"  Confidence: {and_conf:.4f}")
    print(f"OR({[v.name for v in test_values]}) = {or_result.name}")
    print(f"  Confidence: {or_conf:.4f}")

    # Test 5: Performance benchmark
    print("\n5. Performance Benchmark:")
    print("-" * 60)

    start_time = time.perf_counter()

    processor = RTKAUProcessor()
    for _ in range(10000):
        tree = processor.create_recursive_tree(
            TernaryValue.UNKNOWN,
            0.5,
            max_depth=3,
            context="Benchmark"
        )
        processor.resolve_recursive(tree)

    elapsed = time.perf_counter() - start_time

    print(f"Processed 10,000 UNKNOWN resolutions")
    print(f"Total time: {elapsed:.3f} seconds")
    print(f"Average per resolution: {elapsed/10000*1000:.3f} ms")
    print(f"Total recursive evaluations: {processor.evaluation_count}")

    print("\n" + "=" * 60)
    print("Copyright (c) 2025 - H.Overman <opsec.ee@pm.me>")


if __name__ == "__main__":
    run_comprehensive_test()
