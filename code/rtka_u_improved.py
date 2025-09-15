#!/usr/bin/env python3
"""
RTKA-U: Recursive Ternary Kleene Algorithm with Uncertainty Quantification

Implements three-valued Kleene logic with recursive evaluation and confidence
propagation, adhering to mathematical formulations for AND (min), OR (max), NOT (-a),
and recursive application with O(n) time complexity.

Author: H. Overman <opsec.ee@pm.me>
Version: 1.2.2
"""

from abc import ABC, abstractmethod
from typing import Tuple, Union, Optional, List
from enum import Enum
import math
import time
from dataclasses import dataclass


class TruthValue(Enum):
    """Three-valued Kleene logic truth values mapped to {-1, 0, 1}."""
    TRUE = 1    # T
    UNKNOWN = 0 # U
    FALSE = -1  # F
    
    def __str__(self) -> str:
        return {1: "T", 0: "U", -1: "F"}.get(self.value, "U")
    
    def __repr__(self) -> str:
        return f"TruthValue.{self.name}"


class LogicOperator(Enum):
    """Supported logical operators."""
    AND = "∧"
    OR = "∨"
    NOT = "¬"
    
    def __str__(self) -> str:
        return self.value


@dataclass
class EvaluationResult:
    """Result of logical expression evaluation."""
    value: TruthValue
    confidence: float
    evaluations: int = 0  # Number of subexpressions evaluated
    
    def __post_init__(self):
        """Validate confidence bounds."""
        if not (0.0 <= self.confidence <= 1.0):
            raise ValueError(f"Confidence must be in [0,1], got {self.confidence}")


class LogicExpression(ABC):
    """Abstract base class for logical expressions."""
    
    @abstractmethod
    def evaluate_rtka(self) -> EvaluationResult:
        """Evaluate expression using RTKA-U algorithm."""
        pass
    
    @abstractmethod
    def evaluate_naive(self) -> EvaluationResult:
        """Evaluate expression using naive approach (no optimization)."""
        pass
    
    @abstractmethod
    def __str__(self) -> str:
        """String representation of the expression."""
        pass


class AtomicProposition(LogicExpression):
    """Atomic proposition with a ternary value and confidence."""
    
    def __init__(self, value: Union[TruthValue, int], confidence: Optional[float] = None):
        if isinstance(value, int):
            if value not in [-1, 0, 1]:
                raise ValueError(f"Value must be in {{-1, 0, 1}}, got {value}")
            self.value = TruthValue(value)
        else:
            self.value = value
        
        if confidence is None:
            self.confidence = 1.0 if self.value != TruthValue.UNKNOWN else 0.5
        else:
            self.confidence = confidence
            if self.value in [TruthValue.TRUE, TruthValue.FALSE] and confidence != 1.0:
                raise ValueError("Definitive values (TRUE, FALSE) must have confidence 1.0")
            if not (0.0 <= confidence <= 1.0):
                raise ValueError(f"Confidence must be in [0,1], got {confidence}")
    
    def evaluate_rtka(self) -> EvaluationResult:
        """RTKA evaluation of atomic proposition."""
        return EvaluationResult(self.value, self.confidence, evaluations=1)
    
    def evaluate_naive(self) -> EvaluationResult:
        """Naive evaluation of atomic proposition."""
        return self.evaluate_rtka()
    
    def __str__(self) -> str:
        return f"{self.value}({self.confidence:.2f})"


class Conjunction(LogicExpression):
    """Logical conjunction (AND) expression: min(a, b)."""
    
    def __init__(self, left: LogicExpression, right: LogicExpression):
        self.left = left
        self.right = right
    
    def evaluate_rtka(self) -> EvaluationResult:
        """RTKA evaluation with early termination (min)."""
        left_result = self.left.evaluate_rtka()
        evaluations = left_result.evaluations
        
        # Early termination: min(-1, x) = -1 with confidence 1.0
        if left_result.value == TruthValue.FALSE:
            return EvaluationResult(TruthValue.FALSE, 1.0, evaluations)
        
        right_result = self.right.evaluate_rtka()
        evaluations += right_result.evaluations
        
        # min(left, right) as per mathematical definition
        result_value = TruthValue(min(left_result.value.value, right_result.value.value))
        # Confidence is 1.0 if result is FALSE, else multiply confidences
        result_confidence = 1.0 if result_value == TruthValue.FALSE else left_result.confidence * right_result.confidence
        
        return EvaluationResult(result_value, result_confidence, evaluations)
    
    def evaluate_naive(self) -> EvaluationResult:
        """Naive evaluation (no early termination)."""
        left_result = self.left.evaluate_naive()
        right_result = self.right.evaluate_naive()
        
        result_value = TruthValue(min(left_result.value.value, right_result.value.value))
        # Confidence is 1.0 if result is FALSE, else multiply confidences
        result_confidence = 1.0 if result_value == TruthValue.FALSE else left_result.confidence * right_result.confidence
        
        evaluations = left_result.evaluations + right_result.evaluations
        return EvaluationResult(result_value, result_confidence, evaluations)
    
    def __str__(self) -> str:
        return f"({self.left} ∧ {self.right})"


class Disjunction(LogicExpression):
    """Logical disjunction (OR) expression: max(a, b)."""
    
    def __init__(self, left: LogicExpression, right: LogicExpression):
        self.left = left
        self.right = right
    
    def evaluate_rtka(self) -> EvaluationResult:
        """RTKA evaluation with early termination (max)."""
        left_result = self.left.evaluate_rtka()
        evaluations = left_result.evaluations
        
        # Early termination: max(1, x) = 1
        if left_result.value == TruthValue.TRUE:
            return EvaluationResult(TruthValue.TRUE, 1.0, evaluations)
        
        right_result = self.right.evaluate_rtka()
        evaluations += right_result.evaluations
        
        # max(left, right) as per mathematical definition
        result_value = TruthValue(max(left_result.value.value, right_result.value.value))
        result_confidence = 1.0 - (1.0 - left_result.confidence) * (1.0 - right_result.confidence)
        
        return EvaluationResult(result_value, result_confidence, evaluations)
    
    def evaluate_naive(self) -> EvaluationResult:
        """Naive evaluation (no early termination)."""
        left_result = self.left.evaluate_naive()
        right_result = self.right.evaluate_naive()
        
        result_value = TruthValue(max(left_result.value.value, right_result.value.value))
        result_confidence = 1.0 - (1.0 - left_result.confidence) * (1.0 - right_result.confidence)
        
        evaluations = left_result.evaluations + right_result.evaluations
        return EvaluationResult(result_value, result_confidence, evaluations)
    
    def __str__(self) -> str:
        return f"({self.left} ∨ {self.right})"


class Negation(LogicExpression):
    """Logical negation (NOT) expression: -a."""
    
    def __init__(self, operand: LogicExpression):
        self.operand = operand
    
    def evaluate_rtka(self) -> EvaluationResult:
        """RTKA evaluation of negation."""
        operand_result = self.operand.evaluate_rtka()
        
        # -a as per mathematical definition
        result_value = TruthValue(-operand_result.value.value)
        result_confidence = operand_result.confidence
        
        return EvaluationResult(result_value, result_confidence, operand_result.evaluations)
    
    def evaluate_naive(self) -> EvaluationResult:
        """Naive evaluation of negation."""
        return self.evaluate_rtka()
    
    def __str__(self) -> str:
        return f"¬{self.operand}"


class RTKAEvaluator:
    """Main RTKA-U algorithm evaluator with performance tracking."""
    
    def __init__(self):
        self.reset_stats()
    
    def reset_stats(self):
        """Reset performance statistics."""
        self.rtka_evaluations = 0
        self.naive_evaluations = 0
        self.rtka_time = 0.0
        self.naive_time = 0.0
    
    def evaluate(self, expression: LogicExpression, method: str = "rtka") -> EvaluationResult:
        """Evaluate expression with specified method."""
        if method == "rtka":
            start_time = time.perf_counter()
            result = expression.evaluate_rtka()
            end_time = time.perf_counter()
            self.rtka_evaluations += result.evaluations
            self.rtka_time += (end_time - start_time)
        elif method == "naive":
            start_time = time.perf_counter()
            result = expression.evaluate_naive()
            end_time = time.perf_counter()
            self.naive_evaluations += result.evaluations
            self.naive_time += (end_time - start_time)
        else:
            raise ValueError(f"Unknown evaluation method: {method}")
        return result
    
    def performance_summary(self) -> dict:
        """Return performance comparison summary."""
        speedup = (self.naive_time / self.rtka_time) if self.rtka_time > 0 else 0
        evaluation_reduction = (
            (self.naive_evaluations - self.rtka_evaluations) / self.naive_evaluations
        ) if self.naive_evaluations > 0 else 0
        return {
            "rtka_evaluations": self.rtka_evaluations,
            "naive_evaluations": self.naive_evaluations,
            "evaluation_reduction": evaluation_reduction,
            "rtka_time": self.rtka_time,
            "naive_time": self.naive_time,
            "speedup": speedup
        }


# Convenience functions
def And(left: LogicExpression, right: LogicExpression) -> Conjunction:
    """Create conjunction expression."""
    return Conjunction(left, right)


def Or(left: LogicExpression, right: LogicExpression) -> Disjunction:
    """Create disjunction expression."""
    return Disjunction(left, right)


def Not(operand: LogicExpression) -> Negation:
    """Create negation expression."""
    return Negation(operand)


def Var(value: Union[TruthValue, int], confidence: Optional[float] = None) -> AtomicProposition:
    """Create atomic proposition."""
    return AtomicProposition(value, confidence)


def evaluate_expression(values: List[int], confidences: List[float], operator: str) -> None:
    """Evaluate a user-defined expression with given values and confidences."""
    if len(values) != len(confidences):
        raise ValueError("Values and confidences must have the same length")
    if not values:
        raise ValueError("At least one value is required")
    
    evaluator = RTKAEvaluator()
    # Create atomic propositions
    props = [Var(val, conf) for val, conf in zip(values, confidences)]
    
    # Build expression based on operator
    if operator == "AND":
        expr = props[0]
        for prop in props[1:]:
            expr = And(expr, prop)
    elif operator == "OR":
        expr = props[0]
        for prop in props[1:]:
            expr = Or(expr, prop)
    elif operator == "NOT":
        if len(props) != 1:
            raise ValueError("NOT operator requires exactly one value")
        expr = Not(props[0])
    else:
        raise ValueError(f"Unsupported operator: {operator}")
    
    # Evaluate
    print(f"\nEvaluating {operator} expression: {expr}")
    result_rtka = evaluator.evaluate(expr, "rtka")
    result_naive = evaluator.evaluate(expr, "naive")
    
    print(f"RTKA result: {result_rtka.value} (conf: {result_rtka.confidence:.2f}, evals: {result_rtka.evaluations})")
    print(f"Naive result: {result_naive.value} (conf: {result_naive.confidence:.2f}, evals: {result_naive.evaluations})")
    
    stats = evaluator.performance_summary()
    print(f"Evaluation reduction: {stats['evaluation_reduction']:.1%}")
    print(f"Time speedup: {stats['speedup']:.2f}x")


if __name__ == "__main__":
    print("RTKA-U Algorithm Demo")
    print("=" * 40)
    
    # Example: User-defined AND expression
    evaluate_expression(
        values=[-1, 0, 1],  # FALSE, UNKNOWN, TRUE
        confidences=[1.0, 0.5, 1.0],
        operator="AND"
    )
    
    # Example: User-defined OR expression
    evaluate_expression(
        values=[1, 0, -1],  # TRUE, UNKNOWN, FALSE
        confidences=[1.0, 0.5, 1.0],
        operator="OR"
    )
    
    # Example: User-defined NOT expression
    evaluate_expression(
        values=[0],  # UNKNOWN
        confidences=[0.5],
        operator="NOT"
    )