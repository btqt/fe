# Design of Commonization for V2X Manager for telematic projects

## Basic Information
- Author: Nguyen Van Si
- Topic: Design of Commonization for V2X Manager for telematic projects
- Year: 2025
- Project: V2X Manager for telematic projects / JLR-VCM project

## Problem Summary
This topic addresses the absence of a reusable common architecture for V2X Manager across telematics projects. In the current design, multiple Tiger services send messages through Binder to a single-threaded Looper, which creates a performance bottleneck and limits message throughput.

The goal is to remove that bottleneck while still keeping the architecture maintainable and reusable for future projects.

## Options and Selected Direction
- Option 1 - Tiger service with multi-threading: Improve throughput with minimal RAM and CPU overhead, but make the code harder to maintain as the service grows.
- Option 2 - V2XMgr services with LGVF: Use LGVF-based services to improve throughput while preserving stronger maintainability and reuse.
- Option 3 - Split V2X Manager into multiple Tiger services: Improve bottleneck behavior and maintainability, but reduce reusability because the design becomes too fragmented.

The selected option is V2XMgr services with LGVF. It provides the best balance among performance, maintainability, and reusability, and the measured throughput exceeds the stated target.

## Techniques and Design Patterns
- Multi-threading
- LGVF-based service architecture
- Microservices-style decomposition
- Message queue processing
- Distributed services
- Thread pooling

## Quality Attributes
The quality attributes are taken directly from the source:
- Performance
- Reusability
- Maintainability
