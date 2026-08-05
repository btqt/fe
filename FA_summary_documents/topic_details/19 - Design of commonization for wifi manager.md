# Design of commonization for wifi manager

## Basic Information
- Author: manh2.tran
- Topic: Design of commonization for wifi manager
- Year: 2025
- Project: LGEDV Vehicle Network Team

## Problem Summary
This topic focuses on the Wifi Manager architecture, where components depend too heavily on one another. That tight coupling makes the code difficult to scale when new features are added and also hurts maintainability because changing one part can affect many others.

The goal is to redesign the service so shared behavior can be reused across variants while keeping the architecture easier to test and extend.

## Options and Selected Direction
- Option 1 - Chain of Responsibility + Singleton: Decouple the handling flow so logic changes can be isolated with fewer side effects, while keeping shared coordination centralized.
- Option 2 - Chain of Responsibility + Abstract Factory: Keep the flow decoupled, but also separate object creation from usage so dependencies can be mocked more easily and the design becomes more extensible.

The selected option is Proposal 2, which combines Chain of Responsibility with Abstract Factory. It was chosen because it provides stronger extensibility and better testability while still preserving the reuse benefits of the first proposal.

## Techniques and Design Patterns
- Chain of Responsibility pattern
- Abstract Factory pattern
- Dependency injection
- Asynchronous event communication
- Variant-specific controller implementation

## Quality Attributes
The quality attributes are taken directly from the source:
- Reusability
- Modifiability
- Extensibility
- Maintainability
- Testability
- Performance
