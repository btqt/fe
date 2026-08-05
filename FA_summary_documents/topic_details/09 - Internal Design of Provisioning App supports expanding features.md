# Internal Design of Provisioning App supports expanding features

## Basic Information
- Author: Tuyen.dang
- Topic: Internal Design of Provisioning App supports expanding features
- Year: 2023
- Project: Provisioning App (BMW WAVE)

## Problem Summary
The current ProvisioningApp implementation is monolithic: it manages many service proxies, receivers, and notifications through one central class and one large handler with switch-case logic. This makes the application difficult to extend because any new service or new message-handling behavior affects the same core class.

The topic focuses on restructuring the internals so that new features can be added without breaking existing logic. The design also aims to remove duplicated request/notification handling and improve reuse for future projects.

## Options and Selected Direction
- Façade pattern: Abstract direct service interactions behind a simpler interface so ProvisioningApp does not depend directly on all service details.
- Communicator pattern: Encapsulate each service manager separately and initialize them lazily when needed.
- Chain of Responsibility pattern: Route requests and notifications through dedicated handlers instead of a monolithic switch-case block.
- Command pattern: Encapsulate requests as executable objects.

The selected direction combines the Communicator pattern with Chain of Responsibility. This combination separates service-specific logic from the central application class, reduces coupling, and allows new handlers or services to be added more independently. Command and Factory ideas are used as supporting techniques around the request flow.

## Techniques and Design Patterns
- Communicator pattern
- Chain of Responsibility pattern
- Command pattern
- Factory pattern
- Dependency injection
- Lazy initialization
- Encapsulation
- Abstraction
- Loose coupling

## Quality Attributes
The quality attributes are taken directly from the source:
- Performance
- Reliability
- Reusability
- Modifiability
- Maintainability
