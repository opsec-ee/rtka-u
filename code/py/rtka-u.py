# rtka-u.py
# Recursive Ternary with Kleene Algorithm + UNKNOWN
# Implementation in Python
# Author: H.Overman <opsec.ee@pm.me>

def kleene_and(a, b):
    return min(a, b)

def kleene_or(a, b):
    return max(a, b)

def kleene_not(a, _=None):  # Second arg ignored for unary op
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

def confidence_not(confidence, _=None):
    return confidence[0]

def recursive_ternary(operation, inputs, confidences=None):
    if not inputs:
        raise ValueError("Input list cannot be empty")
    if len(inputs) == 1:
        if confidences:
            return inputs[0], confidences[0]
        return inputs[0]
    
    # Early termination conditions
    if operation == kleene_and and inputs[0] == -1:
        return -1, confidence_and(confidences) if confidences else None
    if operation == kleene_or and inputs[0] == 1:
        return 1, confidence_or(confidences) if confidences else None
    
    # Recursive step
    tail_result, tail_confidence = recursive_ternary(operation, inputs[1:], confidences[1:] if confidences else None) if confidences else (recursive_ternary(operation, inputs[1:]), None)
    result = operation(inputs[0], tail_result)
    
    if confidences:
        if operation == kleene_and:
            confidence = confidence_and([confidences[0], tail_confidence])
        elif operation == kleene_or:
            confidence = confidence_or([confidences[0], tail_confidence])
        else:  # kleene_not
            confidence = confidences[0]
        return result, confidence
    
    return result

# Example usage
def main():
    # Test case: ∧ᵣ(0, 1, -1) with confidences [0.8, 0.9, 0.7]
    inputs = [0, 1, -1]
    confidences = [0.8, 0.9, 0.7]
    result, conf = recursive_ternary(kleene_and, inputs, confidences)
    print(f"AND result: {result}, Confidence: {conf}")  # Expected: -1, 0.504
    
    # Test case: ∨ᵣ(0, -1, 1) with confidences [0.8, 0.9, 0.7]
    inputs = [0, -1, 1]
    confidences = [0.8, 0.9, 0.7]
    result, conf = recursive_ternary(kleene_or, inputs, confidences)
    print(f"OR result: {result}, Confidence: {conf}")  # Expected: 1, 0.964
    
    # Test case: ¬ᵣ(0)
    inputs = [0]
    confidences = [0.8]
    result, conf = recursive_ternary(kleene_not, inputs, confidences)
    print(f"NOT result: {result}, Confidence: {conf}")  # Expected: 0, 0.8

if __name__ == "__main__":
    main()
