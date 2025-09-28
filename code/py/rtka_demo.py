#!/usr/bin/env python3
"""
RTKA: Recursive Ternary Knowledge Algorithm - Engineering Demonstration
Copyright (c) 2025 - H.Overman <opsec.ee@pm.me>
Advanced Recursion Python Implementation (v1.3)
Copyright (c) 2025 - H.Overman <opsec.ee@pm.me>
DOI: https://doi.org/10.5281/zenodo.17148691
ORCHID: https://orcid.org/0009-0007-9737-762X
 * 
 * PROPRIETARY AND CONFIDENTIAL
 * This file is proprietary to H. Overman and may not be reproduced,
 * distributed, or used without explicit written permission.
 *
"""

import numpy as np
from enum import IntEnum
from typing import List, Tuple, Dict, Optional
from dataclasses import dataclass
import time

class Ternary(IntEnum):
    """Core insight: Not everything is true or false"""
    FALSE = -1
    UNKNOWN = 0
    TRUE = 1
    
    def __str__(self):
        return {-1: "FALSE", 0: "UNKNOWN", 1: "TRUE"}[self.value]

@dataclass
class SensorReading:
    """Real sensors don't give perfect binary answers"""
    source: str
    value: Ternary
    confidence: float
    timestamp: float
    noise_level: float = 0.0

@dataclass
class RecursiveNode:
    """Track recursive evaluation with full context preservation"""
    value: Ternary
    confidence: float
    depth: int
    parent_context: str
    decision_path: List[str]
    evidence_collected: List[float]
    branch_taken: str
    timestamp: float
    
class RTKASystem:
    """
    RECURSIVE Ternary Knowledge Algorithm
    Key innovation: Recursive depth tracking with data preservation at each level
    """
    
    def __init__(self):
        self.decision_history = []
        self.unknown_resolutions = 0
        self.false_positives_avoided = 0
        self.true_negatives_preserved = 0
        self.recursion_tree = []  # Full recursive evaluation history
        self.max_recursion_depth_reached = 0
        
    def kleene_and(self, a: Ternary, b: Ternary) -> Ternary:
        """Min operation: FALSE dominates, UNKNOWN preserves uncertainty"""
        return Ternary(min(a, b))
    
    def kleene_or(self, a: Ternary, b: Ternary) -> Ternary:
        """Max operation: TRUE dominates, UNKNOWN preserves uncertainty"""
        return Ternary(max(a, b))
    
    def confidence_and(self, c1: float, c2: float) -> float:
        """Multiplicative: both conditions must be confident"""
        return c1 * c2
    
    def confidence_or(self, c1: float, c2: float) -> float:
        """Inclusion-exclusion: either condition suffices"""
        return 1 - (1 - c1) * (1 - c2)
    
    def fuse_sensors(self, readings: List[SensorReading]) -> Tuple[Ternary, float, str]:
        """
        Critical capability: Combine multiple uncertain sensors
        Binary logic would force premature true/false decisions
        """
        if not readings:
            return Ternary.UNKNOWN, 0.0, "No data"
        
        # Weight by confidence and recency
        weights = []
        values = []
        
        for reading in readings:
            weight = reading.confidence * (1.0 - reading.noise_level)
            weights.append(weight)
            values.append(reading.value)
        
        # Weighted consensus
        if sum(weights) == 0:
            return Ternary.UNKNOWN, 0.0, "No confident readings"
        
        weighted_sum = sum(w * v for w, v in zip(weights, values))
        avg_confidence = sum(weights) / len(weights)
        
        # Decision boundaries with uncertainty zone
        normalized = weighted_sum / sum(weights)
        
        if normalized < -0.3:
            result = Ternary.FALSE
            confidence = avg_confidence
            reason = f"Consensus FALSE (score: {normalized:.2f})"
        elif normalized > 0.3:
            result = Ternary.TRUE
            confidence = avg_confidence
            reason = f"Consensus TRUE (score: {normalized:.2f})"
        else:
            result = Ternary.UNKNOWN
            confidence = avg_confidence * 0.5  # Reduced confidence in uncertainty
            reason = f"Insufficient consensus (score: {normalized:.2f})"
            self.unknown_resolutions += 1
        
        return result, confidence, reason
    
    def recursive_resolve(self, value: Ternary, confidence: float, 
                         max_depth: int = 3, context: str = "") -> Tuple[Ternary, float, int, List[RecursiveNode]]:
        """
        CORE RECURSIVE ENGINE: UNKNOWN spawns investigation branches
        Each level preserves complete context and decision history
        """
        recursion_path = []
        depth = 0
        current_val = value
        current_conf = confidence
        evidence_trail = []
        
        # Root node
        root_node = RecursiveNode(
            value=current_val,
            confidence=current_conf,
            depth=0,
            parent_context=context,
            decision_path=[context],
            evidence_collected=[],
            branch_taken="ROOT",
            timestamp=time.time()
        )
        recursion_path.append(root_node)
        
        while current_val == Ternary.UNKNOWN and depth < max_depth:
            # Gather evidence at this recursive level
            evidence = np.random.normal(0, 0.3)
            evidence_trail.append(evidence)
            
            # Determine branch based on evidence
            if evidence < -0.3:
                branch = "FALSE_BRANCH"
                next_val = Ternary.FALSE if evidence < -0.5 else Ternary.UNKNOWN
            elif evidence > 0.3:
                branch = "TRUE_BRANCH"
                next_val = Ternary.TRUE if evidence > 0.5 else Ternary.UNKNOWN
            else:
                branch = "UNKNOWN_BRANCH"
                next_val = Ternary.UNKNOWN
            
            # Update confidence with recursive decay
            evidence_quality = 1.0 - abs(evidence) / 2.0
            current_conf *= evidence_quality * (2/3)  # Mathematical decay per level
            
            # Create node for this recursive level
            node = RecursiveNode(
                value=next_val,
                confidence=current_conf,
                depth=depth + 1,
                parent_context=f"{context} -> Level {depth}",
                decision_path=root_node.decision_path + [f"Depth {depth+1}: {branch}"],
                evidence_collected=evidence_trail.copy(),
                branch_taken=branch,
                timestamp=time.time()
            )
            recursion_path.append(node)
            
            current_val = next_val
            depth += 1
            
            # Track maximum recursion depth
            if depth > self.max_recursion_depth_reached:
                self.max_recursion_depth_reached = depth
        
        # Store the complete recursion tree
        self.recursion_tree.extend(recursion_path)
        
        return current_val, current_conf, depth, recursion_path
    
    def recursive_and_with_tracking(self, values: List[Ternary], 
                                   confidences: List[float]) -> Tuple[Ternary, float, List[RecursiveNode]]:
        """
        Recursive AND that tracks every evaluation level
        Shows how UNKNOWN propagates through logical chains
        """
        all_nodes = []
        result = values[0]
        conf = confidences[0]
        
        for i, (val, c) in enumerate(zip(values[1:], confidences[1:]), 1):
            # If we hit UNKNOWN, spawn recursive investigation
            if result == Ternary.UNKNOWN or val == Ternary.UNKNOWN:
                # Recursive resolution attempt
                resolved, resolved_conf, depth, nodes = self.recursive_resolve(
                    Ternary.UNKNOWN, 
                    self.confidence_and(conf, c),
                    max_depth=5,
                    context=f"AND operation at position {i}"
                )
                all_nodes.extend(nodes)
                
                # Use resolved value
                result = self.kleene_and(result, resolved)
                conf = resolved_conf
            else:
                result = self.kleene_and(result, val)
                conf = self.confidence_and(conf, c)
                
                # Track non-recursive evaluation
                node = RecursiveNode(
                    value=result,
                    confidence=conf,
                    depth=0,
                    parent_context=f"Direct AND at position {i}",
                    decision_path=[f"Position {i}: {val}"],
                    evidence_collected=[],
                    branch_taken="DIRECT",
                    timestamp=time.time()
                )
                all_nodes.append(node)
            
            # Early termination
            if result == Ternary.FALSE:
                break
        
        return result, conf, all_nodes
    
    def make_decision(self, name: str, inputs: List[Tuple[Ternary, float]]) -> Dict:
        """
        Actual decision with uncertainty tracking
        """
        # Combine all inputs
        result = inputs[0][0]
        confidence = inputs[0][1]
        
        for val, conf in inputs[1:]:
            # Use AND for safety-critical (all must agree)
            # Use OR for opportunity detection (any suffices)
            result = self.kleene_and(result, val)
            confidence = self.confidence_and(confidence, conf)
            
            # Early termination optimization
            if result == Ternary.FALSE:
                break
        
        # Attempt resolution if uncertain
        if result == Ternary.UNKNOWN:
            result, confidence, depth, nodes = self.recursive_resolve(result, confidence, context=name)
        else:
            depth = 0
            nodes = []
        
        decision = {
            'name': name,
            'result': result,
            'confidence': confidence,
            'resolution_depth': depth,
            'recursive_nodes': len(nodes),
            'timestamp': time.time()
        }
        
        self.decision_history.append(decision)
        return decision

def demonstrate_autonomous_vehicle():
    """
    Scenario: Autonomous vehicle obstacle detection
    Multiple sensors with varying reliability
    """
    print("\n" + "="*70)
    print("AUTONOMOUS VEHICLE: Multi-Sensor Obstacle Detection")
    print("="*70)
    
    rtka = RTKASystem()
    
    # Scenario 1: Clear obstacle
    print("\nScenario 1: Highway - Multiple sensors agree")
    sensors = [
        SensorReading("LIDAR", Ternary.TRUE, 0.95, time.time(), 0.05),
        SensorReading("RADAR", Ternary.TRUE, 0.90, time.time(), 0.10),
        SensorReading("CAMERA", Ternary.TRUE, 0.85, time.time(), 0.15),
    ]
    
    result, conf, reason = rtka.fuse_sensors(sensors)
    print(f"  Detection: {result} (confidence: {conf:.1%})")
    print(f"  Reason: {reason}")
    print(f"  Action: {'BRAKE' if result == Ternary.TRUE else 'CONTINUE'}")
    
    # Scenario 2: Conflicting sensors
    print("\nScenario 2: Fog conditions - Sensors disagree")
    sensors = [
        SensorReading("LIDAR", Ternary.FALSE, 0.40, time.time(), 0.60),  # Degraded by fog
        SensorReading("RADAR", Ternary.TRUE, 0.85, time.time(), 0.15),   # Works in fog
        SensorReading("CAMERA", Ternary.UNKNOWN, 0.20, time.time(), 0.80), # Can't see
    ]
    
    result, conf, reason = rtka.fuse_sensors(sensors)
    print(f"  Detection: {result} (confidence: {conf:.1%})")
    print(f"  Reason: {reason}")
    
    if result == Ternary.UNKNOWN:
        print(f"  Action: SLOW DOWN + INCREASE FOLLOWING DISTANCE")
        # Recursive resolution attempt
        resolved, resolved_conf, depth, nodes = rtka.recursive_resolve(result, conf)
        print(f"  After gathering {depth} additional readings: {resolved} ({resolved_conf:.1%})")
    
    # Scenario 3: Sensor failure
    print("\nScenario 3: Sensor malfunction")
    sensors = [
        SensorReading("LIDAR", Ternary.UNKNOWN, 0.0, time.time(), 1.0),  # Failed
        SensorReading("RADAR", Ternary.FALSE, 0.70, time.time(), 0.30),
        SensorReading("CAMERA", Ternary.FALSE, 0.65, time.time(), 0.35),
    ]
    
    result, conf, reason = rtka.fuse_sensors(sensors)
    print(f"  Detection: {result} (confidence: {conf:.1%})")
    print(f"  Reason: {reason}")
    print(f"  System health: DEGRADED - Relying on {sum(1 for s in sensors if s.confidence > 0)}/3 sensors")

def demonstrate_medical_diagnosis():
    """
    Scenario: Medical diagnosis with incomplete test results
    Cannot wait for all tests, must reason with uncertainty
    """
    print("\n" + "="*70)
    print("MEDICAL DIAGNOSIS: Reasoning with Incomplete Data")
    print("="*70)
    
    rtka = RTKASystem()
    
    print("\nPatient presents with symptoms. Running diagnostic protocol...")
    
    # Test results arrive over time
    test_results = [
        ("Blood Test A", Ternary.TRUE, 0.95),      # Indicates infection
        ("Blood Test B", Ternary.UNKNOWN, 0.0),    # Results pending
        ("Imaging", Ternary.TRUE, 0.80),           # Shows inflammation
        ("Culture", Ternary.UNKNOWN, 0.0),         # Takes 48 hours
    ]
    
    print("\nAvailable test results:")
    for test, result, conf in test_results:
        if result != Ternary.UNKNOWN:
            print(f"  {test:15} : {result} (confidence: {conf:.1%})")
        else:
            print(f"  {test:15} : PENDING")
    
    # Make decision with available data
    available = [(r, c) for _, r, c in test_results if r != Ternary.UNKNOWN]
    
    decision = rtka.make_decision("Initial Diagnosis", available)
    
    print(f"\nDiagnosis with {len(available)}/{len(test_results)} tests:")
    print(f"  Result: {decision['result']}")
    print(f"  Confidence: {decision['confidence']:.1%}")
    
    if decision['result'] == Ternary.TRUE and decision['confidence'] > 0.7:
        print(f"  Action: Begin treatment immediately")
    elif decision['result'] == Ternary.UNKNOWN:
        print(f"  Action: Order additional tests, monitor closely")
    else:
        print(f"  Action: Wait for pending results")
    
    # Simulate getting more results
    print("\n6 hours later - Culture preliminary results...")
    test_results[3] = ("Culture", Ternary.TRUE, 0.60)
    
    all_available = [(r, c) for _, r, c in test_results if r != Ternary.UNKNOWN]
    decision = rtka.make_decision("Updated Diagnosis", all_available)
    
    print(f"  Updated result: {decision['result']} ({decision['confidence']:.1%})")
    print(f"  Clinical confidence threshold met: {'YES' if decision['confidence'] > 0.75 else 'NO'}")

def demonstrate_security_threat():
    """
    Scenario: Network security threat assessment
    Multiple indicators with varying reliability
    """
    print("\n" + "="*70)
    print("SECURITY: Threat Assessment with Recursive Investigation")
    print("="*70)
    
    rtka = RTKASystem()
    
    print("\nAnalyzing network activity patterns...")
    
    # Various threat indicators
    indicators = [
        ("Traffic Volume", Ternary.TRUE, 0.70),      # Spike detected
        ("Known Signatures", Ternary.FALSE, 0.95),   # No matches
        ("Behavioral Analysis", Ternary.UNKNOWN, 0.50), # Ambiguous - NEEDS RECURSION
        ("Geographic Origin", Ternary.TRUE, 0.60),   # Suspicious location
        ("Port Scanning", Ternary.UNKNOWN, 0.45),    # Unclear - NEEDS RECURSION
    ]
    
    # Process each indicator
    print("\nThreat Indicators:")
    for indicator, value, confidence in indicators:
        status = "[!] " if value == Ternary.TRUE else "[OK] " if value == Ternary.FALSE else "[?] "
        print(f"  {status}{indicator:20} : {value} ({confidence:.0%} confidence)")
    
    # Process with recursive tracking
    print("\n[RECURSIVE ANALYSIS] Processing UNKNOWN indicators...")
    
    for i, (indicator, value, confidence) in enumerate(indicators):
        if value == Ternary.UNKNOWN:
            print(f"\n  Recursively investigating: {indicator}")
            resolved, conf, depth, nodes = rtka.recursive_resolve(
                value, confidence, max_depth=6, context=indicator
            )
            
            print(f"    Recursion depth reached: {depth}")
            print(f"    Final resolution: {resolved} ({conf:.1%})")
            
            # Show recursive path
            if depth > 0:
                print(f"    Decision path:")
                for node in nodes[:3]:  # Show first 3 levels
                    print(f"      Level {node.depth}: {node.branch_taken} (conf: {node.confidence:.1%})")
                if len(nodes) > 3:
                    print(f"      ... {len(nodes)-3} more levels ...")
            
            # Update the indicator with resolved value
            indicators[i] = (indicator, resolved, conf)
    
    # Overall threat assessment using OR
    threat_level = indicators[0][1]
    threat_conf = indicators[0][2]
    
    for _, val, conf in indicators[1:]:
        threat_level = rtka.kleene_or(threat_level, val)
        threat_conf = rtka.confidence_or(threat_conf, conf)
    
    print(f"\nThreat Assessment After Recursive Resolution:")
    print(f"  Level: {threat_level}")
    print(f"  Confidence: {threat_conf:.1%}")
    print(f"  Max recursion depth used: {rtka.max_recursion_depth_reached}")
    print(f"  Total recursive nodes created: {len(rtka.recursion_tree)}")
    
    # Response based on threat level
    if threat_level == Ternary.TRUE:
        print(f"  Response: ACTIVE THREAT - Initiating containment")
        print(f"  Actions: Rate limiting, enhanced logging, alert SOC")
    elif threat_level == Ternary.UNKNOWN:
        print(f"  Response: POTENTIAL THREAT - Elevated monitoring")
        print(f"  Actions: Increase logging verbosity, prepare response team")
    else:
        print(f"  Response: NO THREAT DETECTED - Normal operations")
        print(f"  Actions: Continue standard monitoring")

def demonstrate_deep_recursion():
    """
    CORE DEMONSTRATION: Deep recursive evaluation with full tracking
    Shows how RTKA preserves context through multiple recursion levels
    """
    print("\n" + "="*70)
    print("DEEP RECURSION: The Heart of RTKA")
    print("="*70)
    
    rtka = RTKASystem()
    
    print("\nScenario: Complex decision requiring deep investigation")
    print("Initial state: UNKNOWN with 90% confidence")
    
    # Deep recursive resolution
    initial_value = Ternary.UNKNOWN
    initial_confidence = 0.90
    
    print("\nStarting recursive investigation (max depth: 10)...")
    result, final_conf, depth, nodes = rtka.recursive_resolve(
        initial_value, 
        initial_confidence, 
        max_depth=10,
        context="Complex System State"
    )
    
    print(f"\n[RECURSION COMPLETE]")
    print(f"  Recursion depth reached: {depth}")
    print(f"  Final value: {result}")
    print(f"  Final confidence: {final_conf:.4f}")
    print(f"  Nodes in recursion tree: {len(nodes)}")
    
    # Show the recursion tree structure
    print("\n[RECURSION TREE STRUCTURE]")
    for node in nodes:
        indent = "  " * node.depth
        print(f"{indent}Level {node.depth}: {node.value} ({node.confidence:.3f}) - {node.branch_taken}")
    
    # Demonstrate confidence decay
    print("\n[CONFIDENCE DECAY ANALYSIS]")
    print("Depth | Theoretical | Actual    | Branch")
    print("------|-------------|-----------|----------")
    for node in nodes:
        theoretical = (2/3) ** node.depth if node.depth > 0 else 1.0
        print(f"  {node.depth:2}  | {theoretical:.6f}  | {node.confidence:.6f} | {node.branch_taken}")
    
    # Demonstrate recursive AND with tracking
    print("\n[RECURSIVE AND CHAIN WITH TRACKING]")
    values = [Ternary.TRUE, Ternary.UNKNOWN, Ternary.TRUE, Ternary.UNKNOWN, Ternary.TRUE]
    confidences = [0.9, 0.7, 0.8, 0.6, 0.95]
    
    print(f"Input chain: {[str(v) for v in values]}")
    print(f"Confidences: {confidences}")
    
    result, conf, nodes = rtka.recursive_and_with_tracking(values, confidences)
    
    print(f"\nResult: {result} (confidence: {conf:.3f})")
    print(f"Recursive evaluations performed: {len([n for n in nodes if n.depth > 0])}")
    print(f"Direct evaluations: {len([n for n in nodes if n.depth == 0])}")
    
    # Show flagged recursive levels
    print("\n[FLAGGED RECURSIVE LEVELS WITH PRESERVED DATA]")
    for node in nodes:
        if node.depth > 2:  # Flag deep recursions
            print(f"  [FLAGGED] Depth {node.depth}:")
            print(f"    Context: {node.parent_context}")
            print(f"    Evidence collected: {[f'{e:.2f}' for e in node.evidence_collected[:3]]}")
            print(f"    Confidence: {node.confidence:.3f}")

def demonstrate_mathematical_properties():
    """
    Show the mathematical properties that make RTKA work
    """
    print("\n" + "="*70)
    print("MATHEMATICAL PROPERTIES: Why Ternary Logic Works")
    print("="*70)
    
    rtka = RTKASystem()
    
    # Demonstrate UNKNOWN preservation
    print("\nProperty 1: UNKNOWN Preservation in Logic Chains")
    print("-" * 50)
    
    chain_length = 10
    values = [Ternary.TRUE] * 4 + [Ternary.UNKNOWN] + [Ternary.TRUE] * 5
    
    print(f"Input chain: {[str(v) for v in values]}")
    
    # AND chain
    result = values[0]
    for v in values[1:]:
        result = rtka.kleene_and(result, v)
        if result == Ternary.UNKNOWN:
            print(f"  AND: UNKNOWN detected and preserved")
            break
    
    # OR chain  
    result = values[0]
    for v in values[1:]:
        result = rtka.kleene_or(result, v)
    print(f"  OR: Result = {result} (TRUE dominates in OR)")
    
    # Property 2: Confidence decay
    print("\nProperty 2: Confidence Decay in Recursive Resolution")
    print("-" * 50)
    
    initial_conf = 1.0
    depth_results = []
    
    for depth in range(1, 8):
        theoretical = (2/3) ** depth
        print(f"  Depth {depth}: Theoretical persistence = {theoretical:.4f}")
        depth_results.append(theoretical)
    
    print(f"\nConverges to near-zero: {depth_results[-1]:.6f}")
    
    # Property 3: Early termination
    print("\nProperty 3: Early Termination Optimization")
    print("-" * 50)
    
    # AND with early FALSE
    values = [Ternary.TRUE, Ternary.TRUE, Ternary.FALSE] + [Ternary.UNKNOWN] * 7
    evaluations = 0
    result = values[0]
    
    for i, v in enumerate(values[1:], 1):
        evaluations += 1
        result = rtka.kleene_and(result, v)
        if result == Ternary.FALSE:
            print(f"  AND chain terminated early at position {i+1}/{len(values)}")
            print(f"  Saved {len(values) - evaluations} evaluations")
            break
    
    # OR with early TRUE
    values = [Ternary.FALSE, Ternary.FALSE, Ternary.TRUE] + [Ternary.UNKNOWN] * 7
    evaluations = 0
    result = values[0]
    
    for i, v in enumerate(values[1:], 1):
        evaluations += 1
        result = rtka.kleene_or(result, v)
        if result == Ternary.TRUE:
            print(f"  OR chain terminated early at position {i+1}/{len(values)}")
            print(f"  Saved {len(values) - evaluations} evaluations")
            break

def main():
    """
    Complete demonstration of RTKA capabilities
    """
    print("\n" + "="*70)
    print("RECURSIVE TERNARY KNOWLEDGE ALGORITHM")
    print("Engineering Systems for Real-World Uncertainty")
    print("="*70)
    print("\nTraditional binary logic forces premature decisions.")
    print("Real systems need to represent and process uncertainty.")
    print("RTKA's RECURSIVE nature allows deep investigation of UNKNOWN states.")
    print("Each recursive level preserves complete context and decision history.\n")
    
    # Run demonstrations
    demonstrate_deep_recursion()  # Show recursive depth first
    demonstrate_autonomous_vehicle()
    demonstrate_medical_diagnosis()
    demonstrate_security_threat()
    demonstrate_mathematical_properties()
    
    # Summary
    print("\n" + "="*70)
    print("KEY INSIGHTS: THE POWER OF RECURSION")
    print("="*70)
    print("""
1. RECURSION is the core innovation
   UNKNOWN states spawn investigation branches
   Each level preserves data and context

2. Confidence decays mathematically through recursion
   Follows (2/3)^n decay pattern
   Prevents infinite uncertainty loops

3. Recursive tracking preserves decision paths
   Every branch taken is recorded
   Complete audit trail for decisions

4. Flagged recursive levels enable analysis
   Deep recursions can be marked
   Context preserved at every depth

5. Early termination with recursion tracking
   Optimization doesn't lose information
   Full tree structure maintained

The RECURSIVE nature transforms UNKNOWN from a dead-end
into an active investigation process with full traceability.
""")
    
    print("Copyright (c) 2025 - H.Overman <opsec.ee@pm.me>")
    print("="*70)

if __name__ == "__main__":
    main()
