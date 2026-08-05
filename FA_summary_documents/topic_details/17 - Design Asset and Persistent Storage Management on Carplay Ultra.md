# Design Asset and Persistent Storage Management on Carplay Ultra

## Basic Information
- Author: Huy Quang Le
- Topic: Design Asset and Persistent Storage Management on Carplay Ultra
- Year: 2025
- Project: CarPlay Ultra / Phone Projection Team

## Problem Summary
CarPlay Ultra Local UI requires assets to be transferred from the vehicle file system into the Display plug-in and rendered locally inside the vehicle. The architecture must manage the full asset lifecycle (load, stage, foreground, delete, switch on iPhone change) and also handle persistent storage for vehicle state information.

The design challenge is choosing how to manage `AssetSession` objects across the CarPlay Engine layer while keeping the solution performant, reusable, and maintainable.

## Options and Selected Direction
- Proposal 1 - Manage multiple AssetSession: Each session is a distinct object managed across the client and engine. Improves performance and modularity via OOP but causes logic duplication because the client dependency persists.
- Proposal 2 - Singleton AssetManager (manage single callback): Use a single global `AssetManager` with callback-based handling. Simplifies engine code and reduces JNI data exchange, but requires careful thread-safety handling for the global object.

The selected option is Proposal 2 (Singleton AssetManager). It was chosen because it gives higher reusability by decoupling client and engine, and reduces duplicated logic at the cost of more careful thread management.

## Techniques and Design Patterns
- Singleton pattern for AssetManager
- Strategy and Factory design patterns
- SOLID principles (Open-Closed Principle)
- JNI interface management
- Callback-based session handling

## Quality Attributes
The quality attributes are taken directly from the source:
- QA.001 | Performance | High | The vehicle must load and foreground the assets on all Display plug-ins within 100 ms
- QA.002 | Reusability | Medium | When adding support for a new Apple requirement, modification to existing CarPlay Engine code shall not exceed 5% of LOC
- QA.003 | Reusability | Low | Maintainability fixes or small feature changes (< 50 LOC), implementation time shall not exceed 2 person-hours
