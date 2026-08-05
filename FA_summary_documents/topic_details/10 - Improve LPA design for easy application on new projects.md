# Improve LPA design for easy application on new projects

## Basic Information
- Author: Nguyen Truong Minh
- Topic: Improve LPA design for easy application on new projects
- Year: 2023
- Project: Local Profile Assistant (LPA) / LGE-VS-BMW-WAVE

## Problem Summary
The current LPA architecture mixes OEM-specific requirements with the GSMA-standard implementation in the same component. Because the standard part directly depends on project-specific platform services, the solution is hard to move to a new OEM project without significant redesign and integration work.

This makes the standard logic less reusable and forces project-specific modifications into the core implementation. The topic therefore focuses on separating standard responsibilities from project-specific adaptation.

## Options and Selected Direction
- Solution 1 - Tight coupling: Keep OEM-specific logic and GSMA-standard logic together, with direct dependency on platform services.
- Solution 2 - Separation with Communication Adapter: Split the architecture into OEM part, standard part, and a Communication Adapter that hides platform-specific interfaces.

The selected option is Solution 2. By separating the OEM part from the GSMA-standard part and introducing a Communication Adapter, the design makes project-specific changes easier, keeps the standard implementation reusable, and allows each component to evolve more independently.

## Techniques and Design Patterns
- Adapter pattern (Communication Adapter)
- Factory pattern (SimManagerFactory)
- Command pattern
- Separation of concerns
- Abstraction
- Interface-based design
- Modular decomposition

## Quality Attributes
The quality attributes are taken directly from the source:
- Modifiability
- Reusability
- Modularity
