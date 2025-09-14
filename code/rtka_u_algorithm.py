"""
rtka-u Algorithm Implementation
Copyright (c) 2025 - H.Overman <opsec.ee@pm.me>

RTKA-U: Recursive Ternary Known Algorithm - Unknown
A three-valued logic system with confidence calculations
"""

def rtka_and(a, b):
    return min(a, b)

def rtka_or(a, b):
    return max(a, b)

def rtka_not(a, _=None):
    return -a

def confidence_and(confidences):
    result = 1.0
    for c in confidences:
        result *= c
    return result

def confidence_or(confidences):
    result = 1.0
    for c in confidences:
        result *= (1 - c)
    return 1 - result

def recursive_ternary(operation, inputs, confidences=None):
    if not inputs:
        raise ValueError("Input list cannot be empty")
    if len(inputs) == 1:
        return (inputs[0], confidences[0]) if confidences else inputs[0]
    
    if operation == rtka_and and inputs[0] == -1:
        return -1, confidence_and(confidences) if confidences else None
    if operation == rtka_or and inputs[0] == 1:
        return 1, confidence_or(confidences) if confidences else None
    
    if confidences:
        tail_result, tail_confidence = recursive_ternary(
            operation, 
            inputs[1:], 
            confidences[1:] if confidences else None
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
