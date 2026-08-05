# Common Factory Service Design for AVN Virtualization Projects

## Basic Information
- Author: Nguyen Duong Nguyen (Nguyen2.nguyen)
- Topic: Common Factory Service Design for AVN Virtualization Projects
- Year: 2025
- Project: AVN Virtualization Projects / LGEDV Factory Inspection SW Team

## Problem Summary
The original Factory Service was designed for Android only, but new AVN virtualization projects distribute testing-related features across multiple virtual machines and potentially different operating systems. This makes the old single-platform design insufficient for multi-VM and multi-OS factory inspection.

The problem is therefore to create a common factory-service architecture that can work across Android, QNX, Linux, and inter-VM communication boundaries while still keeping project-specific adaptation manageable.

## Options and Selected Direction
- Option 1 - Android AIDL + Inter-VM Communication: Reuse the existing Android Factory Service and connect to services on other VMs through network communication.
- Option 2 - Service-Oriented Architecture Design: Split the design into reusable Core and project-specific Variant parts, with native and Java services connected through a server-client communication model.

The selected option is the Service-Oriented Architecture design. It was chosen because it provides stronger reusability, supports multiple platforms and operating systems, and offers better maintainability and interoperability than the Android-centered solution.

## Techniques and Design Patterns
- Service-Oriented Architecture (SOA)
- Inter-VM communication
- AIDL
- Core/Variant decomposition
- Factory server pattern
- Centralized logging

## Quality Attributes
The quality attributes are taken directly from the source:
- Reusability
- Interoperability
- Compatibility
- Performance
