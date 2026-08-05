# Improving performance of event notification in Power Manager

## Basic Information
- Author: Hoang Quang Trung (trung.hoang)
- Topic: Improving performance of event notification in Power Manager
- Year: 2025
- Project: Toyota 24DCM

## Problem Summary
The topic addresses a performance bottleneck in the Power Manager service. When the user turns off ignition, the Remote Air Conditioner should stop within about 1.5 seconds, but the actual response time is around 4 seconds. The main reason is the current notification mechanism creates and destroys a dedicated thread for each receiver, and with about 38 receivers this thread lifecycle overhead grows to roughly 760 ms for a single notification cycle.

Because the Power Manager is a mission-critical service, the issue is not only about speed. Delayed notification of power-state changes can also affect system reliability, including battery-related behavior and other services that depend on timely power events.

## Options and Selected Direction
- Option 1 - On-Demand Thread (current): Create and destroy a thread for each notification task. This is simple and reliable, but very slow because the OS scheduling and thread lifecycle cost is high.
- Option 2 - Thread Cache Pool: Pre-create permanent threads for all receivers so notifications can start immediately. This improves response time, but wastes memory and CPU because many threads stay idle most of the time.
- Option 3 - Fixed Thread Pool: Keep a fixed number of worker threads, such as 2 or 4, to process notifications. This reduces resource usage, but delays can still happen if all threads are busy.
- Option 4 - Dynamic Thread Pool: Start from a small baseline pool and scale up when processing time exceeds a threshold, then scale down after an idle period.

The selected solution is the Dynamic Thread Pool. It gives near-fixed-pool performance while managing resources more efficiently than a permanent cache pool. The design also improves reliability because it can react to long-running handlers by adding threads temporarily. In the detailed design stage, the selected integration style is the generic interface approach so the thread pool can be reused by other services instead of being tied only to Power Manager.

## Techniques and Design Patterns
- Dynamic thread pool with scale-up and scale-down control
- Generic interface integration for reuse across services
- Event-driven notification handling
- Threshold-based resource tuning

## Quality Attributes
The quality attributes are taken directly from the source:
- Performance (High): From 760ms to under 250ms for 1 event, total time for 6 notifies < 1.5 seconds
- Reliability (High): Ensure all services/apps receive notifications even when other services/apps fail
- Resource Efficiency (Medium): Efficiently manage resources since power notifications occur infrequently, avoiding waste while ensuring rapid response when needed
- Maintainability (Medium): The new design must ensure that the system is able to support the changes in the future
- Reusability (Medium): New design would be beneficial if it can be reused for other services since all services utilize this notification mechanism
- Simplicity (Low): Balance implementation complexity with operational reliability for mission-critical systems
