# Performance improvement of Broadcast SXM function

## Basic Information
- Author: Pham Phuong Tuan
- Topic: Performance improvement of Broadcast SXM function
- Year: 2022
- Project: SXM HMI Application

## Problem Summary
The SXM HMI application displays slow or blank screens because category and channel data arrive late through the EMMA API and related framework constraints. Users may wait more than one second before seeing usable content, and full initialization can take much longer.

The architectural task is to reduce visible latency for the user while keeping memory usage within an acceptable range.

## Options and Selected Direction
- Option 1 - Cache Layer: Return cached data immediately and refresh the display in the background when newer data arrives.
- Option 2 - Cyclic preload: Refresh lists periodically in the background regardless of user action.

The selected direction is the cache layer with one-time preload initialization. It improves perceived performance substantially without the higher ongoing CPU cost of cyclic preload.

## Techniques and Design Patterns
- Caching strategy with invalidation
- One-time preload initialization
- Conditional cache reset
- Background synchronization
- Memory estimation and optimization

## Quality Attributes
The quality attributes are taken directly from the source:
- Performance: QA01/QA02/QA03 require lists displayed within 1 second; cache layer achieves 0.001s response vs 0.425s baseline (424x faster)
- Usability: Reduce blank/loading screen visibility for user
- Memory efficiency: 3.34MB cache acceptable on system
- Responsiveness: Immediate cached data display then background update
