# Architecture improvement for CAN dispatching function of Micom Manager

## Basic Information
- Author: Cao Anh Hoang (hoang.cao)
- Topic: Architecture improvement for CAN dispatching function of Micom Manager
- Year: 2024
- Project: GM Info3.5 (AVN)

## Problem Summary
The existing CAN dispatching design relies on a monolithic `CANHandler` with large switch-case logic and fixed registration lists for many CAN frames. This makes the implementation difficult to understand, hard to modify safely, and less reusable across projects.

The topic focuses on breaking this monolithic behavior into a more extensible and maintainable architecture while still preserving correct CAN signal distribution to the right managers.

## Options and Selected Direction
- Proposal 1 - Observer pattern: Use a subject/observer structure where managers register their CAN-data interests dynamically and receive relevant updates.
- Proposal 2 - Chain of Responsibility pattern: Route each CAN frame through a chain of handlers that check whether they should process it.

The selected option is the Observer pattern. It supports stronger extensibility and maintainability by allowing managers to register interests without modifying the main CAN dispatcher logic, and it also improves reuse for future projects.

## Techniques and Design Patterns
- Observer pattern
- CAN frame parsing and signal extraction
- SPI communication with Micom
- IPC-based message distribution
- Dynamic registration mechanism

## Quality Attributes
The quality attributes are taken directly from the source:
- Modifiability/Extensibility
- Maintainability
- Reusability
