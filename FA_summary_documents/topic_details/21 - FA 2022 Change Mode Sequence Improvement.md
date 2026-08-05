# FA 2022 Change Mode Sequence Improvement

## Basic Information
- Author: Chuong.nguyen
- Topic: FA 2022 Change Mode Sequence Improvement
- Year: 2022
- Project: LG Vehicle Solution Development Center Vietnam

## Problem Summary
The current change-mode sequence design creates strong dependencies between services and makes the system difficult to extend. It also complicates coordination between fast mode changes, which should interrupt running animations, and normal mode changes, which should wait for animations to complete.

The architecture therefore needs a cleaner sequencing model so new elements can be added more easily and the overall coordination logic is easier to maintain.

## Options and Selected Direction
- Option 1 - Common Change mode Architecture (CCA): Introduce a common class so new elements can inherit shared behavior and implement the needed APIs, reducing direct dependencies between elements.
- Option 2 - Monitor Change mode Architecture (MCA): Centralize sequencing through a monitor so existing logic is easier to migrate, but the monitor itself becomes more complex.

The selected option is Common Change mode Architecture. Although it requires more restructuring effort, it provides a better long-term architecture for extension and maintenance by reducing inter-element dependencies.

## Techniques and Design Patterns
- Event-driven architecture
- Observer pattern
- State management
- Animation sequencing control
- Template-style common behavior design

## Quality Attributes
The quality attributes are taken directly from the source:
- Extensibility
- Maintainability
- Modularity
