# Architecture design for NGeCall supports maintenance and expansion

## Basic Information
- Author: hoang2.nguyen
- Topic: Architecture design for NGeCall supports maintenance and expansion
- Year: 2024
- Project: BMW-WAVE / NGeCall application

## Problem Summary
This topic focuses on restructuring the NGeCall application so it can be maintained and extended more safely. The original `eCallNGProcess` implementation is extremely large and monolithic, with very high cyclomatic complexity and too many responsibilities gathered in one class.

Because of this structure, adding regional variants or new features requires widespread modification and increases the risk of unintended side effects. The architecture therefore needs clearer separation of concerns and better support for extension and unit testing.

## Options and Selected Direction
- Option 1 - Mediator Pattern: Use a mediator class to control interactions among the NGeCall components. This improves structure, but risks turning the mediator into another large central object.
- Option 2 - Interface Classes: Split the application into separate functional classes such as Data, Trigger, and Call, and use interfaces/polymorphism to separate responsibilities.

The selected option is Interface Classes. It provides better maintainability because region-specific branching can be resolved earlier in initialization, supports easier unit testing with mockable abstractions, and keeps the code more modular than the mediator-based approach.

## Techniques and Design Patterns
- Interface-based architecture
- Polymorphism
- Single Responsibility Principle
- Open/Closed Principle
- Functional class decomposition

## Quality Attributes
The quality attributes are taken directly from the source:
- Maintainability (High)
- Modifiability (High)
- Reusability (High)
