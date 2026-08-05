# Handling Cinemo events in Media service

## Basic Information
- Author: Dinh Cong Bang
- Topic: Handling Cinemo events in Media service
- Year: 2023
- Project: Media service (ICAS3.1 CHN)

## Problem Summary
The Media Native service contains several player classes such as JukeBoxPlayer, ApplePlayer, and BTApplePlayer, and each one handles both playback control and Cinemo event handling. This creates duplicated logic that cannot be reused across players and also causes unnecessary thread usage because each player polls Cinemo events independently.

As a result, the design wastes resources and becomes harder to maintain whenever a new player or event type is introduced. The key architectural problem is the lack of separation between playback behavior and common event-handling logic.

## Options and Selected Direction
- Decouple Players from Cinemo event handling: Keep players focused on playback only and move event processing to a separate mechanism.
- Commonize event handling logic: Create shared interfaces and handlers for the same Cinemo events across multiple players.
- Proposal 1 - Singleton + Chain of Responsibility: Use one shared event queue and route each event through a handler chain until the correct handler processes it.
- Proposal 2 - Singleton + Observer: Use one shared event source and notify multiple handlers through observer registration.

The selected option is Proposal 1 with Singleton and Chain of Responsibility. It reduces thread usage, keeps responsibility separated per handler, avoids unnecessary broadcast of every event to all handlers, and supports future expansion with less change to existing code.

## Techniques and Design Patterns
- Singleton pattern
- Chain of Responsibility pattern
- Event-driven architecture
- Handler chain
- Separation of concerns
- Loose coupling

## Quality Attributes
The quality attributes are taken directly from the source:
- Extensibility
- Resource Utilization
- Efficiency
- Maintainability
- Reusability
- Modifiability
