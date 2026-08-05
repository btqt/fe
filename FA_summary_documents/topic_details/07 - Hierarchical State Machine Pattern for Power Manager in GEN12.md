# Hierarchical State Machine Pattern for Power Manager in GEN12

## Basic Information
- Author: Phuong DAO
- Topic: Hierarchical State Machine Pattern for Power Manager in GEN12
- Year: 2022
- Project: Power Manager (GEN12)

## Problem Summary
This topic studies how to redesign the Power Manager state machine so it can satisfy tight boot-time constraints while remaining maintainable when change requests are added. The existing Finite State Machine approach relies on function-pointer tables and duplicates event-handling logic across states, which makes the code harder to modify and reuse.

The problem is therefore architectural rather than purely functional. The service already works, but its structure creates duplication and limits long-term maintainability and reuse across projects.

## Options and Selected Direction
- Finite State Machine (FSM): A traditional state-machine implementation using function pointers and direct handling logic in each state.
- Hierarchical State Machine (HSM): An object-oriented state design with super-state and sub-state organization to reduce duplicate handling logic and make extension easier.

The selected direction is Hierarchical State Machine. It was chosen because it reduces duplication, increases cohesion, lowers coupling through abstraction and interface injection, and has already shown reuse potential across projects such as GEN12 and TOY 24DCM.

## Techniques and Design Patterns
- Hierarchical State Machine
- State pattern
- Interface injection
- Abstraction
- Encapsulation
- Abstract base classes
- Composition over inheritance

## Quality Attributes
The quality attributes are taken directly from the source:
- Maintainability
- Modifiability
- Reusability
