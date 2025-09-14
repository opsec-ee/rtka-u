#!/usr/bin/env python3
"""
Test module for rtka-u Algorithm
Copyright (c) 2025 - H.Overman <opsec.ee@pm.me>

RTKA-U: Recursive Ternary Known Algorithm - Unknown
Demonstrates functionality with various inputs and edge cases
"""

from rtka_u_algorithm import (
    rtka_and, rtka_or, rtka_not,
    confidence_and, confidence_or,
    recursive_ternary
)

def main():
    print("=== RTKA-U Algorithm Tests ===\n")
    
    # Test basic RTKA operations
    print("Basic RTKA-U Operations:")
    print(f"rtka_and(1, -1) = {rtka_and(1, -1)}")
    print(f"rtka_and(1, 0) = {rtka_and(1, 0)}")
    print(f"rtka_and(-1, 0) = {rtka_and(-1, 0)}")
    
    print(f"rtka_or(1, -1) = {rtka_or(1, -1)}")
    print(f"rtka_or(1, 0) = {rtka_or(1, 0)}")
    print(f"rtka_or(-1, 0) = {rtka_or(-1, 0)}")
    
    print(f"rtka_not(1) = {rtka_not(1)}")
    print(f"rtka_not(-1) = {rtka_not(-1)}")
    print(f"rtka_not(0) = {rtka_not(0)}")
    
    # Test confidence functions
    print("\nConfidence Functions:")
    confidences = [0.9, 0.8, 0.7]
    print(f"confidence_and({confidences}) = {confidence_and(confidences)}")
    print(f"confidence_or({confidences}) = {confidence_or(confidences)}")
    
    # Test recursive ternary operations
    print("\nRecursive Ternary Operations:")
    
    # Test AND operations
    inputs = [1, 0, -1]
    result = recursive_ternary(rtka_and, inputs)
    print(f"recursive_ternary(rtka_and, {inputs}) = {result}")
    
    # Test with confidences
    confidences = [0.9, 0.8, 0.7]
    result_with_conf = recursive_ternary(rtka_and, inputs, confidences)
    print(f"recursive_ternary(rtka_and, {inputs}, {confidences}) = {result_with_conf}")
    
    # Test OR operations
    inputs = [-1, 0, 1]
    result = recursive_ternary(rtka_or, inputs)
    print(f"recursive_ternary(rtka_or, {inputs}) = {result}")
    
    # Test early exit conditions
    print("\nEarly Exit Conditions:")
    inputs = [-1, 1, 0]  # Should exit early with -1 for AND
    confidences = [0.9, 0.8, 0.7]
    result = recursive_ternary(rtka_and, inputs, confidences)
    print(f"Early exit AND: recursive_ternary(rtka_and, {inputs}, {confidences}) = {result}")
    
    inputs = [1, -1, 0]  # Should exit early with 1 for OR
    result = recursive_ternary(rtka_or, inputs, confidences)
    print(f"Early exit OR: recursive_ternary(rtka_or, {inputs}, {confidences}) = {result}")
    
    # Test edge cases
    print("\nEdge Cases:")
    try:
        recursive_ternary(rtka_and, [])
    except ValueError as e:
        print(f"Empty input list raises: {e}")
    
    # Single element
    result = recursive_ternary(rtka_and, [1])
    print(f"Single element: recursive_ternary(rtka_and, [1]) = {result}")
    
    result = recursive_ternary(rtka_and, [1], [0.9])
    print(f"Single element with confidence: recursive_ternary(rtka_and, [1], [0.9]) = {result}")

if __name__ == "__main__":
    main()
