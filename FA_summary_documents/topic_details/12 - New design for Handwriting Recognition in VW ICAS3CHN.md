# New design for Handwriting Recognition in VW ICAS3CHN

## Basic Information
- Author: Nguyen Trung Hieu (Hieu4.nguyen)
- Topic: New design for Handwriting Recognition in VW ICAS3CHN
- Year: 2024
- Project: VW ICAS3CHN

## Problem Summary
The existing Handwriting Recognition implementation depends on an additional chipset in the touch screen module. This creates both supply and cost issues, and also introduces unnecessary overhead into the system. The OEM wants the HWR function to move to the head unit while preserving behavior and avoiding impact on HMI applications.

The architecture therefore needs to relocate the HWR logic while still meeting tight latency requirements and keeping the interface reusable for the existing application layer.

## Options and Selected Direction
- Proposal 1 - Integrate HWR into IVI Framework as HWRService: This keeps the interface relatively stable for applications, but still suffers from latency under high CPU load.
- Proposal 2 - Integrate HWR into Input Service: This reduces latency by placing the function closer to the input path, but increases the responsibility and complexity of the Input Service.

The selected option is integration into the Input Service. The experimental results show much lower latency, allowing the solution to meet the required timing target while keeping the application interface reusable.

## Techniques and Design Patterns
- Mediator pattern
- Proxy pattern for third-party HWR engine integration
- Event thread for queued processing
- Timer-triggered stroke recognition
- Touch data pipeline optimization

## Quality Attributes
The quality attributes are taken directly from the source:
- Availability
- Performance
- Reusability
