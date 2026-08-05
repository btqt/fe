# Call Manager Commonization Architecture

## Basic Information
- Author: Nam H. Tran
- Topic: Call Manager Commonization Architecture
- Year: 2021
- Project: AVN Application Unit / Call Bubble widget

## Problem Summary
The Call Bubble widget was implemented separately in multiple HMI applications, which caused thousands of call-state mismatch issues, duplicated logic, high CPU usage, and difficulty when expanding call-related features to other applications.

The architecture needs a common call-state management approach so applications consume one consistent source of truth instead of each maintaining their own logic.

## Options and Selected Direction
- Option 1 - Virtual Call Handler Architecture: Let Home HMI own call-state handling while other applications subscribe to updates.
- Option 2 - Call Manager Commonization (CMC): Introduce a dedicated Call Manager Service that centralizes call-state logic and exposes a common API to HMI applications.

The selected option is Call Manager Commonization. It centralizes call-state handling, reduces duplicated code, and provides more consistent behavior across all HMI applications.

## Techniques and Design Patterns
- Service-based centralization
- State management consolidation
- Common API pattern across HMI applications
- Performance optimization through reduced duplication
- Inter-process communication with phone-related services
- Synchronization mechanisms

## Quality Attributes
The quality attributes are taken directly from the source:
- Usability: Resolves 5000+ call state mismatches; consistent state across all HMI applications
- Reusability: Reduces feature expansion effort from 1MM to 0.05MM per feature
- Modifiability: Centralized changes reduce regression testing
- Performance: Home HMI CPU 11% (vs 16% before); Phone HMI 7% (vs 11% before); Settings HMI 0.5% (vs 7% before); Memory reduction across all HMI apps
- Maintainability: Single service responsible for call state logic
