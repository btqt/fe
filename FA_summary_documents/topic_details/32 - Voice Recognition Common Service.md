# Voice Recognition Common Service

## Basic Information
- Author: Tong Tran Hoang De
- Topic: Voice Recognition Common Service
- Year: 2025
- Project: PIVI system / JLR NGI

## Problem Summary
This topic addresses the problem of supporting multiple voice engines such as Alexa, Cerence, and TmallGenie through separate services. The existing structure increases CPU and memory usage, makes extension harder, complicates audio-focus handling, and creates duplicated or fragmented service logic.

The design goal is to provide a common service layer so multiple engines can be managed consistently through one architecture.

## Options and Selected Direction
- Alternative 1 - Centralized IPC: Consolidate IPC packages but still keep engine-specific managers and handlers. This reuses some existing code, but still duplicates service logic for each engine.
- Alternative 2 - Centralized Manager: Build one common VRService with unified handlers and adapter-based integration of voice engines.

The selected option is the Centralized Manager. It creates a single common interface for voice engines, reduces duplication, and makes new engine integration easier by adding adapters instead of new full services.

## Techniques and Design Patterns
- Adapter pattern
- Centralized service architecture
- Common interface (`IVoiceEngine`)
- IPC package consolidation
- Engine factory pattern

## Quality Attributes
The quality attributes are taken directly from the source:
- Performance (5)
- Reusability (4)
- Extensibility (4)
- Maintainability (3)
- Modifiability (3)
- Resource Efficiency (5)
- Implementation Effort (3)
