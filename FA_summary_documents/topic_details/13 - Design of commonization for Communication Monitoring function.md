# Design of commonization for Communication Monitoring function

## Basic Information
- Author: Nguyen Trung Hieu (hieu5.nguyen)
- Topic: Design of commonization for Communication Monitoring function
- Year: 2024
- Project: MB_BR167M2_ICD

## Problem Summary
This topic deals with duplicated communication-monitoring logic across four software components. Each component implements similar monitoring behavior independently, which increases development time, wastes CPU resources through multiple timing runnables, and creates unstable execution behavior due to interaction with other higher-priority tasks.

The architectural goal is to centralize the common monitoring behavior while preserving the device-specific handling responsibilities where needed. The challenge is to improve performance and reuse without sacrificing modularity.

## Options and Selected Direction
- Proposal 1 - Directly Processing within ComMonitor: Put both monitoring and device control logic into a single component. This reduces scheduling overhead, but makes one component responsible for too much.
- Proposal 2 - Delegating Tasks to Individual SWC: Centralize monitoring detection while delegating device-specific control back to individual software components.

The selected option is Proposal 2. It keeps modular separation between monitoring and device-specific behavior while still reducing overhead, improving reuse, and lowering implementation effort for future projects.

## Techniques and Design Patterns
- Common monitoring architecture
- Centralized state detection
- AUTOSAR RTE-based interaction
- Delegated device control
- Event-driven state propagation

## Quality Attributes
The quality attributes are taken directly from the source:
- Performance
- Modularity
- Reliability
- Maintainability
- Scalability
- Reusability
- Simplicity
