# New design for synchronizing Bluetooth remote device information in RSE Applications

## Basic Information
- Author: Le Trung Thuong
- Topic: New design for synchronizing “Bluetooth remote device information” in RSE Applications (BMW RSE27)
- Year: 2025
- Project: BMW RSE27

## Problem Summary
This topic addresses synchronization of Bluetooth remote-controller information across two Android user screens in the BMW RSE27 rear-seat entertainment system. The application runs as two processes and each side can interact with a different Bluetooth chipset, but the displayed remote-device information and user actions must remain consistent on both screens.

The architectural challenge is to keep the information exactly synchronized, support failover behavior, and still maintain acceptable response time when users connect, remove, or pair the remote device.

## Options and Selected Direction
- Proposal 1 - IPC Service Architecture (Dual Adapter): Keep dual adapters and synchronize data through an IPC channel. This gives full control and uses both Bluetooth chipsets, but increases complexity and adds IPC latency.
- Proposal 2 - Shared Adapter Architecture: Use a shared adapter as a single source of truth for both displays. This simplifies synchronization and gives better response time, but depends more on framework feasibility and uses only one chipset actively.

The selected direction is the Shared Adapter Architecture. It was chosen because it guarantees exact synchronization between the two screens, keeps button response within the target, and avoids synchronization conflicts caused by dual-adapter coordination.

## Techniques and Design Patterns
- Shared adapter architecture
- Single source of truth approach
- IPC-based synchronization (proposal comparison)
- Dual-process Android application coordination
- Failover handling

## Quality Attributes
The quality attributes are taken directly from the source:
- Reliability | High | The information about the remote controller (name, battery level) needs to be synchronized exactly between the 2 screens
- Availability | High | Remote works even if one Bluetooth chipset fails
- Performance | Medium | System response time for users needs to be kept as small as possible(Our target: button press response < 100ms)
- Maintainability | Medium | The structure and logic inside application need to be simple to understand and modify
