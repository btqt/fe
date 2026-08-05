# Alliance Car VMS Service in the Renault project

## Basic Information
- Author: Tran The Dan
- Topic: Alliance Car VMS Service in the Renault project
- Year: 2024
- Project: Renault AIVI2

## Problem Summary
This topic investigates a startup-time and responsiveness problem in the Renault AIVI2 infotainment system. The welcome sequence on the IVI side is delayed by more than 3 seconds compared with the instrument cluster, which creates a visible mismatch in user experience.

The analysis identifies garbage collection inside the `com.alliance.car` process as the main cause. VMS-related objects consume a large portion of the process memory, so GC pauses block the AllianceCarPowerService and delay handling of the welcome-sequence event.

## Options and Selected Direction
- Option 1 - Lazy Initialization: Defer creation of VMS-related objects until after the welcome sequence finishes. This improves startup responsiveness with smaller architectural impact, but VMS still remains in the same process and GC can still affect runtime behavior later.
- Option 2 - Service Separation: Move VMS into a separate external service process with its own lifecycle and garbage collector. This isolates GC impact from the main Alliance Car Service and results in cleaner module boundaries.

The selected direction is Service Separation. Although it increases memory consumption slightly, it achieves the strongest isolation, significantly reduces GC execution time, improves startup behavior, and gives a cleaner architecture for future maintenance and extension.

## Techniques and Design Patterns
- Service separation / process isolation
- External service architecture
- AIDL / Binder-based inter-process communication
- Lifecycle separation for GC isolation

## Quality Attributes
The quality attributes are taken directly from the source:
- Performance (High): Time between receiving WelcomeSequenceStatus = 11 and starting welcome sequence shall be less than 100ms
- The Alliance Car VMS services should not affect the Alliance Car Service performance in the runtime including start-up and other life cycle
- Stability (High): The Alliance Car VMS services shall handle resource allocation well and the system should work stable at any stages
- Maintainability (Medium): Must be clean and clear codebase, easy to maintain and update
