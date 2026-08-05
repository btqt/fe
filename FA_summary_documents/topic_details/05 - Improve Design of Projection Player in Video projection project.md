# Improve Design of Projection Player in Video projection project

## Basic Information
- Author: Dang Thanh Cong (cong.dang)
- Topic: Improve Design of Projection Player in Video projection project
- Year: 2025
- Project: Cockpit 2022+ / VW Video projection project

## Problem Summary
This topic addresses two major architectural problems in the ProjectionPlayer component. First, the player state is managed by multiple boolean flags and repeated conditional logic, which makes the code difficult to understand and risky to extend. Second, the component is tightly coupled to the Cinemo library API, so changing the media library or supporting multiple libraries becomes difficult.

These problems directly affect maintainability and future scalability. As more playback states, events, or media-engine requirements are added, the current implementation becomes harder to evolve safely.

## Options and Selected Direction
- State Management Option 1 - State Pattern: Move behavior into dedicated state classes so transitions and responsibilities are clearer.
- State Management Option 2 - State-Event Mapping with Command Pattern: Use mappings and command objects to handle state/event combinations.
- Library Decoupling Option 1 - Facade Pattern: Wrap the Cinemo API behind a simpler interface.
- Library Decoupling Option 2 - Strategy Pattern: Separate library-specific behavior into interchangeable implementations.

The selected direction is State Pattern for the state-management problem and Strategy Pattern for decoupling the media library. This combination reduces complexity in ProjectionPlayer, isolates library-specific behavior, and creates a cleaner structure for extension and maintenance.

## Techniques and Design Patterns
- State pattern
- Strategy pattern
- Command pattern
- Single Responsibility Principle
- Dependency Inversion Principle

## Quality Attributes
The quality attributes are taken directly from the source:
- Reliability (High): Operate correctly based on the actions performed by the end user
- Maintainability (High): Easy to read and understand, ability of the system to support changes
- Scalability (Medium): Ability to extend without impacting to other parts of the program
