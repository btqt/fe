# Concurrent Processing for Factory Manager

## Basic Information
- Author: Bao Duc Pham
- Topic: Concurrent Processing for Factory Manager
- Year: 2023
- Project: Honda TSU MY23 / Factory Manager Service (FMS)

## Problem Summary
The Factory Manager Service originally processes test commands sequentially, which limits production throughput during factory inspection. The test tool needs to send up to 10 commands concurrently so different hardware modules can be validated in parallel, but the existing design becomes a bottleneck and reduces units-per-hour during manufacturing.

The design challenge is not just to make execution parallel. It must also preserve safe behavior for commands that target the same hardware resource, while allowing independent commands to run concurrently.

## Options and Selected Direction
- Option 1 - Handler Looper Approach: Use a central dispatcher and dedicated handler/looper pairs for services. This enables some separation of work, but is less flexible for scaling and concurrent execution management.
- Option 2 - Thread Pool Approach: Use a worker pool with queued commands and dependency handling so independent commands can run in parallel while dependent ones remain controlled.

The selected option is the Thread Pool approach. It provides a major performance improvement, supports the target of 10 concurrent commands, avoids the command loss seen in the old system, and can be reused for new projects that use the Tiger framework.

## Techniques and Design Patterns
- Thread pool
- Command queueing
- Dependency tracking using Dependence ID
- Concurrent task dispatching with integrity validation

## Quality Attributes
The quality attributes are taken directly from the source:
- Reliability (High): FMS must be available to handle incoming test command even if it is waiting for other test commands to be processed
- Performance (High): FMS must be able to process 10 commands concurrently
- Integrity (Medium): Test commands must retain their content upon reception, and vice versa
- Reusability (Low): FMS after applying concurrent processing should be available for new projects using Tiger framework without re-implement concurrent processing
