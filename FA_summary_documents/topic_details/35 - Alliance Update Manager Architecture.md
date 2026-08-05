# Alliance Update Manager Architecture

## Basic Information
- Author: Thinh Nguyen Van
- Topic: Alliance Update Manager Architecture
- Year: 2022
- Project: A-IVI2 FOTA / Alliance Update Manager (AUM)

## Problem Summary
This topic deals with the architecture of a complex vehicle software update process that must coordinate updates across IVI and several ECUs through multiple phases such as Download, Distribution, Installation, Activation, and Report. The system also needs to support suspend and resume behavior, which makes state management difficult in a purely linear design.

The architecture therefore needs a structure that can preserve campaign context and recover correctly from interruptions.

## Options and Selected Direction
- Option 1 - Sequential design: Process ECU updates in a largely linear flow without clear state decomposition.
- Option 2 - State machine design: Organize the update flow into explicit states and phases with event-driven transitions.

The selected option is the state machine design. It was chosen because it makes suspend/resume handling clearer, isolates each update phase, and supports a more maintainable implementation for complex update campaigns.

## Techniques and Design Patterns
- State machine pattern
- Event-driven architecture with event queues
- Phase-state decomposition
- Normal, Deferred, and Priority event queues
- Resume capability via stored state context
- Common API across ECU handlers

## Quality Attributes
The quality attributes are taken directly from the source:
- Operability: Campaign context preservation enables suspend/resume from any point
- Maintainability: Organized into phases and states; low impact on code changes; easy verification
- Interoperability: All ECU handlers support common API
- Expandability: Easy to add new states and phases
- Recoverability: Graceful error handling and state transitions
