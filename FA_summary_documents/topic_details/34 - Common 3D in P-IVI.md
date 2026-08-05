# Common 3D in P-IVI

## Basic Information
- Author: Truong Quoc Hoang
- Topic: Common 3D in P-IVI
- Year: 2023
- Project: P-IVI (Proteus IVI)

## Problem Summary
Three P-IVI applications such as 4x4i, Camera, and Home each handle the same 3D car model independently. This causes duplicated logic and resources, longer loading time, higher CPU and memory usage, and synchronization mismatches between applications.

The architectural objective is to commonize 3D handling so rendering and model logic are shared instead of duplicated across applications.

## Options and Selected Direction
- Option 1 - 3D Optimization: Optimize 3D implementation separately inside each application.
- Option 2.1 - Logic in 4x4i service, HMI in a new 3D app: Move logic partially into a service and separate the rendering application.
- Option 2.2 - Logic in a new 3D service, HMI in a new 3D app: Separate logic and HMI into dedicated modules, but with higher resource cost.
- Option 2.3 - Logic and 3D in hybrid service: Combine 3D logic and rendering in one hybrid service/application.

The selected option is Proposal 2.3, the hybrid service. It centralizes both 3D logic and rendering, significantly reduces load time and resource usage, and simplifies reuse across multiple applications.

## Techniques and Design Patterns
- MVC architecture
- Hybrid service/application design
- Request management and validation rules
- Viewport and camera positioning control
- Coordinate transformation for 3D models
- State management for shared 3D content

## Quality Attributes
The quality attributes are taken directly from the source:
- Performance: 66% memory reduction (from 837.5MB to 787.9MB total), CPU reduced from 31.81% to 12.78% (60% reduction), load time reduced from 5919ms to 1827ms (69% reduction)
- Maintainability: Centralized 3D logic easier to maintain and restructure
- Reusability: Single 3D service reusable across applications
- Extensibility: Easy to add new applications using 3D models
- Testability: Focused testing on common 3D service logic
