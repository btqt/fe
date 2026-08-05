# Improve Carplay reconnection time after cold boot in Nissan project

## Basic Information
- Author: Hoan Ngoc Tran (hoan.tran)
- Topic: Improve Carplay reconnection time after cold boot in Nissan project
- Year: 2024
- Project: Nissan DA2 (AAOS)

## Problem Summary
This topic addresses the excessive CarPlay reconnection time after a cold boot. The system requires about 61 seconds in total, with most of the delay caused by Android Framework broadcast processing during startup. The OEM requirement is to complete reconnection within 10 seconds.

The design challenge is to shorten reconnection time without weakening security and without breaking interoperability with the rest of the AAOS environment.

## Options and Selected Direction
- Proposal 1 - Database Architecture: Use a ContentProvider-based communication path to avoid the broadcast bottleneck, improving response time but weakening security and reducing communication flexibility.
- Proposal 2 - Binder Architecture: Use direct Binder IPC between services, with caller identity verification and more standard AAOS integration.

The selected option is the Binder Architecture. It satisfies the required timing target, preserves stronger security through caller verification, and supports better interoperability with the AAOS system architecture.

## Techniques and Design Patterns
- Binder IPC
- Direct service-to-service communication
- Android service lifecycle integration
- Connection state handling

## Quality Attributes
The quality attributes are taken directly from the source:
- Performance
- Security
- Interoperability
