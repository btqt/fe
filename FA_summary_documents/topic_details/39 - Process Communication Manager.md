# Process Communication Manager

## Basic Information
- Author: Phan Hoang Hai
- Topic: Process Communication Manager
- Year: 2021
- Project: CSU Unit Application team / Telematics IPC architecture

## Problem Summary
The telematics system contains duplicated IPC-library implementations across multiple services, making communication behavior difficult to modify consistently. Binder limitations also make large data transfer difficult, and file-based workarounds introduce extra latency and complexity.

The architecture therefore needs a standardized communication mechanism that reduces duplication and scales better across services.

## Options and Selected Direction
- Option 1 - Communication Common Library: Build one common IPC library containing shared communication methods.
- Option 2 - Process Communication Manager (PCM): Introduce a central PCM service with dedicated processes and a PCM library API for serialized communication.

The selected direction in the source depends on project size: PCM is preferred for extensive projects because it provides stronger isolation, extensibility, and reuse, while the common library can still fit smaller or Tiger 3.0-compatible projects.

## Techniques and Design Patterns
- IPC abstraction layer
- Message serialization/deserialization
- Process-based communication manager pattern
- Common API façade
- Event-based message dispatch
- Reusable communication protocol

## Quality Attributes
The quality attributes are taken directly from the source:
- Maintainability & Extensibility: PCM provides functional isolation, modifiability, portability, high extensibility; Common Lib provides Tiger 3.0 compatibility
- Performance: PCM reduces startup time by eliminating 7 redundant library imports (~100ms/7libs); reduces code duplication; improved with more processes
- Reusability: PCM easily ported to other projects; Common Lib not reusable due to business-specific code
- Stability: PCM provides error isolation; Common Lib has centralized risk
- Observability: PCM allows tracing which process handles message; better flow visibility
- Portability: PCM superior (business logic encapsulated in processes); Common Lib low (business logic embedded in library)
