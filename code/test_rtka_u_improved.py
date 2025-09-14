#!/usr/bin/env python3
"""
Test module for rtka-u Algorithm (Improved)
Copyright (c) 2025 - H. Overman <opsec.ee@pm.me>

RTKA-U: Recursive Ternary Known Algorithm - Unknown
Demonstrates functionality with various inputs and edge cases
"""
import unittest
from typing import List, Optional
from rtka_u_improved import (
    rtka_and, rtka_or, rtka_not,
    confidence_and, confidence_or,
    recursive_ternary
)

class TestRTKAUImproved(unittest.TestCase):
    """Unit tests for RTKA-U algorithm (Improved)."""
    
    def test_basic_operations(self):
        """Test basic Kleene operations."""
        self.assertEqual(rtka_and(1, -1), -1)
        self.assertEqual(rtka_and(1, 0), 0)
        self.assertEqual(rtka_and(-1, 0), -1)
        self.assertEqual(rtka_and(1, 1), 1)
        
        self.assertEqual(rtka_or(1, -1), 1)
        self.assertEqual(rtka_or(1, 0), 1)
        self.assertEqual(rtka_or(-1, 0), 0)
        self.assertEqual(rtka_or(-1, -1), -1)
        
        self.assertEqual(rtka_not(1), -1)
        self.assertEqual(rtka_not(-1), 1)
        self.assertEqual(rtka_not(0), 0)
    
    def test_confidence_functions(self):
        """Test confidence calculations."""
        confidences = [0.9, 0.8, 0.7]
        self.assertAlmostEqual(confidence_and(confidences), 0.504, places=3)
        self.assertAlmostEqual(confidence_or(confidences), 0.994, places=3)
        
        # Edge cases
        self.assertEqual(confidence_and([1.0, 1.0]), 1.0)
        self.assertEqual(confidence_or([0.0, 0.0]), 0.0)
    
    def test_recursive_ternary(self):
        """Test recursive ternary operations."""
        inputs = [1, 0, -1]
        self.assertEqual(recursive_ternary(rtka_and, inputs), -1)
        
        result, conf = recursive_ternary(rtka_and, inputs, [0.9, 0.8, 0.7])
        self.assertEqual(result, -1)
        self.assertAlmostEqual(conf, 0.504, places=3)
        
        inputs = [-1, 0, 1]
        self.assertEqual(recursive_ternary(rtka_or, inputs), 1)
        
        result, conf = recursive_ternary(rtka_or, inputs, [0.9, 0.8, 0.7])
        self.assertEqual(result, 1)
        self.assertAlmostEqual(conf, 0.994, places=3)
    
    def test_early_exit(self):
        """Test early exit conditions."""
        inputs = [-1, 1, 0]
        confidences = [0.9, 0.8, 0.7]
        result, conf = recursive_ternary(rtka_and, inputs, confidences)
        self.assertEqual(result, -1)
        self.assertAlmostEqual(conf, 0.504, places=3)
        
        inputs = [1, -1, 0]
        result, conf = recursive_ternary(rtka_or, inputs, confidences)
        self.assertEqual(result, 1)
        self.assertAlmostEqual(conf, 0.994, places=3)
        
        # Test early exit without confidences
        self.assertEqual(recursive_ternary(rtka_and, [-1, 1, 0]), -1)
        self.assertEqual(recursive_ternary(rtka_or, [1, -1, 0]), 1)
    
    def test_edge_cases(self):
        """Test edge cases."""
        with self.assertRaises(ValueError):
            recursive_ternary(rtka_and, [])
        
        self.assertEqual(recursive_ternary(rtka_and, [1]), 1)
        result, conf = recursive_ternary(rtka_and, [1], [0.9])
        self.assertEqual(result, 1)
        self.assertEqual(conf, 0.9)
    
    def test_input_validation(self):
        """Test input validation."""
        # Test invalid ternary values
        with self.assertRaises(ValueError):
            rtka_and(2, 1)
        
        with self.assertRaises(ValueError):
            rtka_or(-2, 0)
        
        with self.assertRaises(ValueError):
            rtka_not(5)
        
        # Test type errors
        with self.assertRaises(TypeError):
            rtka_and("1", -1)
        
        with self.assertRaises(TypeError):
            rtka_not("0")
        
        # Test invalid ternary values in recursive function
        with self.assertRaises(ValueError):
            recursive_ternary(rtka_and, [2, 0, 1])
        
        with self.assertRaises(ValueError):
            recursive_ternary(rtka_and, [1, 0, -2])
    
    def test_confidence_validation(self):
        """Test confidence value validation."""
        # Test invalid confidence values
        with self.assertRaises(ValueError):
            recursive_ternary(rtka_and, [1, 0, -1], [1.5, 0.8, 0.7])
        
        with self.assertRaises(ValueError):
            recursive_ternary(rtka_and, [1, 0, -1], [-0.1, 0.8, 0.7])
        
        with self.assertRaises(ValueError):
            confidence_and([1.5, 0.5])
        
        with self.assertRaises(ValueError):
            confidence_or([-0.1, 0.5])
        
        # Test mismatched lengths
        with self.assertRaises(ValueError):
            recursive_ternary(rtka_and, [1, 0, -1], [0.9, 0.8])
        
        # Test empty confidence lists
        with self.assertRaises(ValueError):
            confidence_and([])
        
        with self.assertRaises(ValueError):
            confidence_or([])
    
    def test_comprehensive_scenarios(self):
        """Test comprehensive real-world scenarios."""
        # Test all TRUE values
        inputs = [1, 1, 1]
        confidences = [0.9, 0.8, 0.9]
        
        result, conf = recursive_ternary(rtka_and, inputs, confidences)
        self.assertEqual(result, 1)
        self.assertAlmostEqual(conf, 0.648, places=3)
        
        result, conf = recursive_ternary(rtka_or, inputs, confidences)
        self.assertEqual(result, 1)
        self.assertAlmostEqual(conf, 0.998, places=3)
        
        # Test mixed values with UNKNOWN
        inputs = [1, 0, -1, 0, 1]
        confidences = [0.9, 0.5, 0.8, 0.6, 0.95]
        
        result = recursive_ternary(rtka_and, inputs)
        self.assertEqual(result, -1)
        
        result = recursive_ternary(rtka_or, inputs)
        self.assertEqual(result, 1)

if __name__ == "__main__":
    unittest.main(verbosity=2)
