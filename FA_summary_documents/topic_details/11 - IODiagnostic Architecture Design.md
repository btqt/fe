# IODiagnostic Architecture Design

## Basic Information
- Author: Do Minh Khang (khang2.do)
- Topic: IODiagnostic Architecture Design
- Year: 2022
- Project: BMW ICONICC

## Problem Summary
This topic addresses the internal architecture of IODiagnostic, which must manage communication between the Wake Up Controller and BMW Node0-Diagnostic while supporting DTC reporting and multiple diagnostic jobs such as Read DID, Write DID, and RoutineControl. The design must satisfy functional timing while also remaining maintainable and reusable as diagnostic requirements evolve.

The key architectural issue is how to organize diagnostic data and processing logic so the system can absorb future changes without causing widespread modification. The task therefore focuses on data organization and responsibility allocation inside the diagnostic service.

## Options and Selected Direction
- Proposal 1 - Functional Approach: Organize handling logic by function, which keeps each function locally understandable but makes cross-cutting changes harder.
- Proposal 2 - Data Centralized Approach: Organize around centralized diagnostic data so updates and reuse become easier when requirements change.

The selected option is the Data Centralized Approach. It was chosen because it provides stronger modifiability and reusability, making it easier to adapt the architecture for new diagnostic requirements and to reuse the approach in similar diagnostic components.

## Techniques and Design Patterns
- Data-driven architecture
- SOME/IP communication
- DTC interface handling
- Diagnostic job interface design
- Sequence-based diagnostic flow design

## Quality Attributes
The quality attributes are taken directly from the source:
- Maintainability
- Modifiability
- Reusability
