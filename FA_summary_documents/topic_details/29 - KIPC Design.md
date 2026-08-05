# KIPC Design

## Basic Information
- Author: Tran Anh Kiet
- Topic: KIPC Design (Kernel Inter Process Communication Architecture Improvement)
- Year: 2024
- Project: VW Cockpit / VW-ICAS3/MIB3 GP Project

## Problem Summary
The current KIPC mechanism only provides basic send/receive capability and does not adequately support richer asynchronous or synchronous communication patterns. Developers therefore have to build custom logic around each message and manually handle raw byte streams, which reduces usability and creates maintenance problems.

The architecture needs a more scalable and easier-to-use communication model so application developers can work with higher-level message definitions instead of low-level transport details.

## Options and Selected Direction
- Solution 1 - Expand the KIPC Library: Add MessageProcessing and IPC utility layers around the current KIPC, improving scalability but still leaving developers with multiple libraries to manage.
- Solution 2 - Message Encapsulation: Introduce an abstraction layer and generate interfaces from a Message Description File so communication details are hidden from application developers.

The selected option is Message Encapsulation. It was chosen because it offers stronger usability, scalability, and maintainability by making message handling more abstract and structured.

## Techniques and Design Patterns
- Message Description File
- Message Processing Library
- IPC Library
- Abstraction layer
- Generated interfaces from metadata
- Unified error handling
- Code generation support

## Quality Attributes
The quality attributes are taken directly from the source:
- Scalability (High)
- Usability (Medium)
- Maintainability (Medium)
- Maintainability - Library utilities (Low)
