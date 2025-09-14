from typing import Callable, List, Optional, Union, Tuple

"""
rtka-u Algorithm Implementation (Improved)
Copyright (c) 2025 - H. Overman <opsec.ee@pm.me>

RTKA-U: Recursive Ternary Known Algorithm - Unknown
A three-valued logic system with confidence calculations
"""

def _validate_ternary_value(value: int) -> None:
    """Validate that a value is a valid ternary value {-1, 0, 1}."""
    if value not in {-1, 0, 1}:
        raise ValueError(f"Invalid ternary value: {value}. Must be -1, 0, or 1.")

def _validate_confidence_value(value: float) -> None:
    """Validate that a confidence value is in [0, 1]."""
    if not (0.0 <= value <= 1.0):
        raise ValueError(f"Invalid confidence value: {value}. Must be in [0, 1].")

def rtka_and(a: int, b: int) -> int:
    """Compute Kleene AND for ternary values {-1, 0, 1} (FALSE, UNKNOWN, TRUE).
    
    Args:
        a: First ternary value.
        b: Second ternary value.
    Returns:
        Minimum of a and b (Kleene AND).
    Raises:
        ValueError: If inputs are not valid ternary values.
        TypeError: If inputs are not integers.
    """
    if not isinstance(a, int) or not isinstance(b, int):
        raise TypeError("Ternary values must be integers")
    _validate_ternary_value(a)
    _validate_ternary_value(b)
    return min(a, b)

def rtka_or(a: int, b: int) -> int:
    """Compute Kleene OR for ternary values {-1, 0, 1}.
    
    Args:
        a: First ternary value.
        b: Second ternary value.
    Returns:
        Maximum of a and b (Kleene OR).
    Raises:
        ValueError: If inputs are not valid ternary values.
        TypeError: If inputs are not integers.
    """
    if not isinstance(a, int) or not isinstance(b, int):
        raise TypeError("Ternary values must be integers")
    _validate_ternary_value(a)
    _validate_ternary_value(b)
    return max(a, b)

def rtka_not(a: int, _: Optional[None] = None) -> int:
    """Compute Kleene NOT for a ternary value {-1, 0, 1}.
    
    Args:
        a: Ternary value.
        _: Unused parameter for interface consistency.
    Returns:
        Negation of a (-1 → 1, 1 → -1, 0 → 0).
    Raises:
        ValueError: If input is not a valid ternary value.
        TypeError: If input is not an integer.
    """
    if not isinstance(a, int):
        raise TypeError("Ternary value must be an integer")
    _validate_ternary_value(a)
    return -a

def confidence_and(confidences: List[float]) -> float:
    """Compute confidence for AND operation as product of confidences.
    
    Args:
        confidences: List of confidence values in [0,1].
    Returns:
        Product of confidences.
    Raises:
        ValueError: If any confidence value is not in [0,1].
    """
    if not confidences:
        raise ValueError("Confidence list cannot be empty")
    
    result = 1.0
    for c in confidences:
        _validate_confidence_value(c)
        result *= c
    return result

def confidence_or(confidences: List[float]) -> float:
    """Compute confidence for OR operation as 1 - product(1 - c_i).
    
    Args:
        confidences: List of confidence values in [0,1].
    Returns:
        Confidence for OR operation.
    Raises:
        ValueError: If any confidence value is not in [0,1].
    """
    if not confidences:
        raise ValueError("Confidence list cannot be empty")
    
    result = 1.0
    for c in confidences:
        _validate_confidence_value(c)
        result *= (1 - c)
    return 1 - result

def recursive_ternary(
    operation: Callable[[int, int], int],
    inputs: List[int],
    confidences: Optional[List[float]] = None
) -> Union[int, Tuple[int, float]]:
    """Recursively apply a ternary operation with confidence propagation.
    
    Args:
        operation: Kleene operation (rtka_and, rtka_or, rtka_not).
        inputs: List of ternary values {-1, 0, 1}.
        confidences: Optional list of confidence values in [0,1].
    Returns:
        Ternary result, or (result, confidence) if confidences provided.
    Raises:
        ValueError: If inputs list is empty, values are invalid, or 
                   confidence/input lists have different lengths.
    """
    if not inputs:
        raise ValueError("Input list cannot be empty")
    
    # Validate all inputs
    for val in inputs:
        _validate_ternary_value(val)
    
    # Validate confidences if provided
    if confidences is not None:
        if len(confidences) != len(inputs):
            raise ValueError("Confidence and input lists must have same length")
        for conf in confidences:
            _validate_confidence_value(conf)
    
    # Base case: single element
    if len(inputs) == 1:
        return (inputs[0], confidences[0]) if confidences else inputs[0]
    
    # Early exit conditions with fixed return types
    if operation == rtka_and and inputs[0] == -1:
        if confidences:
            return (-1, confidence_and(confidences))
        else:
            return -1
    
    if operation == rtka_or and inputs[0] == 1:
        if confidences:
            return (1, confidence_or(confidences))
        else:
            return 1
    
    # Recursive case
    if confidences:
        tail_result, tail_confidence = recursive_ternary(
            operation, inputs[1:], confidences[1:]
        )
    else:
        tail_result = recursive_ternary(operation, inputs[1:])
        tail_confidence = None
    
    result = operation(inputs[0], tail_result)
    
    if confidences:
        if operation == rtka_and:
            confidence = confidence_and([confidences[0], tail_confidence])
        elif operation == rtka_or:
            confidence = confidence_or([confidences[0], tail_confidence])
        else:
            confidence = confidences[0]
        return result, confidence
    
    return result
