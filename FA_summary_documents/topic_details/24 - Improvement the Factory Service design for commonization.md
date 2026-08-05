# Improvement the Factory Service design for commonization

## Basic Information
- Author: Le Phuc Thai
- Topic: Improvement the Factory Service design for commonization
- Year: 2024
- Project: Renault/Nissan Project / Factory Service

## Problem Summary
This topic addresses limitations in the Factory Service architecture when moving to FactoryOS 2.0, which is container-based. The existing design causes slower boot behavior, lower UPH, and reliability problems such as container failures that sometimes require re-flashing the software image.

Another problem is that the design is too platform-dependent, which makes reuse across projects difficult. The task therefore focuses on improving performance and reliability while also making the Factory Service more reusable across platforms.

## Options and Selected Direction
- Design 1 - Direct call API from Android HIDL Service: Simplify the architecture and improve performance/reliability, but keep the solution tightly coupled to Android.
- Design 2 - CommonAPI Approach: Use CommonAPI-based IPC to support broader platform independence, better reuse, and cleaner cross-platform architecture.

The selected option is Design 2 using the CommonAPI approach. It was chosen because it improves boot performance and reliability while also making the Factory Service platform-independent and more reusable across projects.

## Techniques and Design Patterns
- CommonAPI middleware
- Abstract connection principle
- Unix Socket communication
- CommonAPI-based IPC

## Quality Attributes
The quality attributes are taken directly from the source:
- Performance (High priority)
- Reliability (High priority)
- Platform Independence (High priority)
- Reusability (Medium priority)
