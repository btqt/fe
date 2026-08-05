# Improve Internal Design of Network Management Handler

## Basic Information
- Author: chinh.nguyen
- Topic: Improve Internal Design of Network Management Handler
- Year: 2024
- Project: VW Cockpit project / Network Management Handler application

## Problem Summary
The Network Management Handler application has grown into a monolithic class that handles service discovery, subscriptions, NM PDU data processing, and network mode control in one place. In addition, many callback-based interactions make execution flow harder to trace and debug.

This structure makes the application difficult to maintain and extend to new vehicle platforms. The topic focuses on reorganizing responsibilities to make the code cleaner and easier to evolve.

## Options and Selected Direction
- Option 1 - Interface Pattern: Split the application logic into service classes such as `NMService` and `NMChannelService`, using a shared interface abstraction.
- Option 2 - Factory Pattern: Separate similar service logic, but centralize object creation and initialization in a dedicated `ServiceFactory`.

The selected option is the Factory Pattern. It gives a cleaner separation between creation logic and business logic, reduces the complexity of the main application class, and supports more maintainable expansion in the future.

## Techniques and Design Patterns
- Factory pattern
- Interface abstraction
- Single Responsibility Principle
- Callback reduction / callback elimination
- Separation of concerns

## Quality Attributes
The quality attributes are taken directly from the source:
- Maintainability (High)
- Modifiability (High)
- Extensibility (Mid)
