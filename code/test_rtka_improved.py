#!/usr/bin/env python3
"""
Comprehensive Test and Visualization Suite for RTKA-U Improved (v1.2.2)
Combines testing with chart generation to validate the algorithm.
"""

import sys
import time
from rtka_u_improved import (
    TruthValue, LogicExpression, RTKAEvaluator, 
    And, Or, Not, Var
)

def test_confidence_handling():
    """Test the improved confidence handling in v1.2.2"""
    print("="*60)
    print("Testing RTKA-U Improved v1.2.2 Confidence Handling")
    print("="*60)
    
    evaluator = RTKAEvaluator()
    
    # Test 1: AND with FALSE result should have confidence 1.0
    print("\n1. AND with FALSE result (should have confidence 1.0):")
    expr = And(Var(0, 0.5), Var(-1, 1.0))  # UNKNOWN AND FALSE
    result = evaluator.evaluate(expr, "rtka")
    print(f"   U(0.5) ∧ F = {result.value} (confidence: {result.confidence})")
    assert result.value == TruthValue.FALSE
    assert result.confidence == 1.0, f"Expected 1.0, got {result.confidence}"
    print("   ✓ Passed: FALSE result has confidence 1.0")
    
    # Test 2: AND with UNKNOWN result maintains multiplicative confidence
    print("\n2. AND with UNKNOWN result (multiplicative confidence):")
    expr = And(Var(0, 0.6), Var(0, 0.7))  # UNKNOWN AND UNKNOWN
    evaluator.reset_stats()
    result = evaluator.evaluate(expr, "rtka")
    expected = 0.6 * 0.7
    print(f"   U(0.6) ∧ U(0.7) = {result.value} (confidence: {result.confidence})")
    print(f"   Expected: {expected}")
    assert result.value == TruthValue.UNKNOWN
    assert abs(result.confidence - expected) < 1e-10
    print("   ✓ Passed: Multiplicative confidence for UNKNOWN result")
    
    # Test 3: OR confidence propagation
    print("\n3. OR confidence propagation:")
    expr = Or(Var(0, 0.4), Var(0, 0.6))
    evaluator.reset_stats()
    result = evaluator.evaluate(expr, "rtka")
    expected = 1.0 - (1.0 - 0.4) * (1.0 - 0.6)
    print(f"   U(0.4) ∨ U(0.6) = {result.value} (confidence: {result.confidence})")
    print(f"   Expected: {expected}")
    assert abs(result.confidence - expected) < 1e-10
    print("   ✓ Passed: OR confidence formula correct")
    
    # Test 4: Early termination
    print("\n4. Early termination test:")
    # Build expression: FALSE AND (100 TRUE values)
    expr = Var(-1, 1.0)
    for _ in range(100):
        expr = And(expr, Var(1, 1.0))
    
    evaluator.reset_stats()
    result = evaluator.evaluate(expr, "rtka")
    print(f"   FALSE ∧ (100 TRUEs) evaluated in {result.evaluations} steps")
    assert result.evaluations == 1, f"Expected 1 evaluation, got {result.evaluations}"
    print("   ✓ Passed: Early termination at first FALSE")
    
    # Test 5: Chain confidence decay
    print("\n5. Chain confidence decay test:")
    for n in [5, 10, 20]:
        props = [Var(0, 0.8) for _ in range(n)]
        expr = props[0]
        for p in props[1:]:
            expr = And(expr, p)
        
        evaluator.reset_stats()
        result = evaluator.evaluate(expr, "rtka")
        expected = 0.8 ** n
        print(f"   {n} UNKNOWNs at 0.8: confidence = {result.confidence:.6f} (expected: {expected:.6f})")
        assert abs(result.confidence - expected) < 1e-10
    print("   ✓ Passed: Confidence decay follows c^n formula")

def test_kleene_logic_truth_tables():
    """Verify Kleene three-valued logic truth tables."""
    print("\n" + "="*60)
    print("Kleene Three-Valued Logic Truth Tables")
    print("="*60)
    
    evaluator = RTKAEvaluator()
    values = [(-1, "F"), (0, "U"), (1, "T")]
    
    # AND truth table
    print("\nAND (∧) Truth Table:")
    print("     F    U    T")
    print("   +------------")
    for v1, l1 in values:
        print(f" {l1} |", end="")
        for v2, l2 in values:
            # Handle confidence properly
            c1 = 1.0 if v1 != 0 else 0.5
            c2 = 1.0 if v2 != 0 else 0.5
            expr = And(Var(v1, c1), Var(v2, c2))
            evaluator.reset_stats()
            result = evaluator.evaluate(expr, "rtka")
            print(f"  {result.value} ", end="")
        print()
    
    # OR truth table
    print("\nOR (∨) Truth Table:")
    print("     F    U    T")
    print("   +------------")
    for v1, l1 in values:
        print(f" {l1} |", end="")
        for v2, l2 in values:
            c1 = 1.0 if v1 != 0 else 0.5
            c2 = 1.0 if v2 != 0 else 0.5
            expr = Or(Var(v1, c1), Var(v2, c2))
            evaluator.reset_stats()
            result = evaluator.evaluate(expr, "rtka")
            print(f"  {result.value} ", end="")
        print()
    
    # NOT truth table
    print("\nNOT (¬) Truth Table:")
    print(" Input | Output")
    print(" ------+-------")
    for v, label in values:
        c = 1.0 if v != 0 else 0.5
        expr = Not(Var(v, c))
        evaluator.reset_stats()
        result = evaluator.evaluate(expr, "rtka")
        print(f"   {label}   |   {result.value}")

def test_performance_comparison():
    """Compare RTKA vs Naive evaluation performance."""
    print("\n" + "="*60)
    print("Performance Comparison: RTKA vs Naive")
    print("="*60)
    
    evaluator = RTKAEvaluator()
    
    sizes = [10, 50, 100, 200, 300]
    
    print("\n1. Best Case (FALSE at beginning):")
    print("Size | RTKA Evals | Naive Evals | Reduction | Speedup")
    print("-" * 55)
    
    for size in sizes:
        # Build expression: FALSE AND TRUE AND TRUE...
        expr = Var(-1, 1.0)
        for _ in range(size - 1):
            expr = And(expr, Var(1, 1.0))
        
        # RTKA evaluation
        evaluator.reset_stats()
        start = time.perf_counter()
        rtka_result = evaluator.evaluate(expr, "rtka")
        rtka_time = time.perf_counter() - start
        rtka_evals = evaluator.rtka_evaluations
        
        # Naive evaluation
        evaluator.reset_stats()
        start = time.perf_counter()
        naive_result = evaluator.evaluate(expr, "naive")
        naive_time = time.perf_counter() - start
        naive_evals = evaluator.naive_evaluations
        
        reduction = (naive_evals - rtka_evals) / naive_evals * 100 if naive_evals > 0 else 0
        speedup = naive_time / rtka_time if rtka_time > 0 else 0
        
        print(f"{size:4} | {rtka_evals:10} | {naive_evals:11} | {reduction:8.1f}% | {speedup:7.2f}x")
    
    print("\n2. Worst Case (FALSE at end):")
    print("Size | RTKA Evals | Naive Evals | Reduction | Speedup")
    print("-" * 55)
    
    for size in sizes:
        # Build expression: TRUE AND TRUE... AND FALSE
        expr = Var(1, 1.0)
        for _ in range(size - 2):
            expr = And(expr, Var(1, 1.0))
        expr = And(expr, Var(-1, 1.0))
        
        evaluator.reset_stats()
        rtka_result = evaluator.evaluate(expr, "rtka")
        rtka_evals = evaluator.rtka_evaluations
        
        evaluator.reset_stats()
        naive_result = evaluator.evaluate(expr, "naive")
        naive_evals = evaluator.naive_evaluations
        
        reduction = (naive_evals - rtka_evals) / naive_evals * 100 if naive_evals > 0 else 0
        
        print(f"{size:4} | {rtka_evals:10} | {naive_evals:11} | {reduction:8.1f}% |   N/A")

def main():
    """Run all tests and generate visualizations."""
    print("╔" + "="*58 + "╗")
    print("║" + " "*10 + "RTKA-U IMPROVED v1.2.2 TEST SUITE" + " "*14 + "║")
    print("╚" + "="*58 + "╝")
    
    try:
        # Run tests
        test_confidence_handling()
        test_kleene_logic_truth_tables()
        test_performance_comparison()
        
        print("\n" + "="*60)
        print("ALL TESTS PASSED ✓")
        print("="*60)
        
        print("\nNow generating visualization charts...")
        print("-" * 40)
        
        # Try to run the chart generators
        try:
            print("\n1. Generating unknown propagation charts...")
            import generate_unknown_propagation_fixed
            print("   ✓ Unknown propagation charts generated")
        except Exception as e:
            print(f"   ✗ Failed to generate unknown propagation charts: {e}")
        
        try:
            print("\n2. Generating confidence decay charts...")
            import generate_confidence_decay_fixed
            print("   ✓ Confidence decay charts generated")
        except Exception as e:
            print(f"   ✗ Failed to generate confidence decay charts: {e}")
        
        print("\n" + "="*60)
        print("COMPREHENSIVE TEST COMPLETE")
        print("="*60)
        
    except AssertionError as e:
        print(f"\n✗ TEST FAILED: {e}")
        sys.exit(1)
    except Exception as e:
        print(f"\n✗ UNEXPECTED ERROR: {e}")
        sys.exit(1)

if __name__ == "__main__":
    main()
