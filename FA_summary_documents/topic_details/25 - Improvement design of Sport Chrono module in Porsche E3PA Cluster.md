# Improvement design of Sport Chrono module in Porsche E3PA Cluster

## Basic Information
- Author: Pham Dong Thai
- Topic: [25 FA] Improvement design of Sport Chrono module in Porsche E3PA Cluster
- Year: 2025
- Project: Porsche E3PA Cluster HMI / Sport Chrono Feature

## Problem Summary
The Sport Chrono module still contains many recurring issues, including wrong page list behavior, wrong screen transitions, and incorrect UI focus positions. The analysis shows that the existing SportChronoService is overloaded with multiple responsibilities such as state management, event distribution, data handling, and UI coordination.

The design also relies on scattered state variables and static data sharing between classes, which makes bugs easy to reintroduce and difficult to fix systematically. The goal is to reduce issue recurrence and make the feature easier to evolve.

## Options and Selected Direction
- State handling Option 1 - Center state manager: Keep state handling centralized.
- State handling Option 2 - State pattern: Move behavior into explicit state structures with clearer transitions and separation.
- Data exchange Option 1 - Separated managers: Separate data and event management responsibilities.
- Data exchange Option 2 - Central manager: Handle data and event responsibilities through a more centralized mechanism.

The selected direction combines the State pattern with separated managers for data and event handling. This combination reduces coupling, improves clarity in state transitions, and supports localized fixes without destabilizing the rest of the module.

## Techniques and Design Patterns
- State pattern
- Observer pattern
- Separated data and event managers
- Localized change approach

## Quality Attributes
The quality attributes are taken directly from the source:
- Maintainability (High priority)
- Modifiability (Medium priority)
- Performance (Low priority)
