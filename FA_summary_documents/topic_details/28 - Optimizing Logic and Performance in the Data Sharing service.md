# Optimizing Logic and Performance in the Data Sharing service

## Basic Information
- Author: tuan7.nguyen
- Topic: Optimizing Logic and Performance in the Data Sharing service
- Year: 2025
- Project: FPK Project (Volkswagen) / Manager Data Sharing (MgrDS)

## Problem Summary
The Data Sharing service suffers from both performance and structural issues. A new thread is created for every image download request, which increases resource usage and creates synchronization problems, while the `CDSUploadThread` is also overloaded with multiple unrelated responsibilities such as message processing, connection creation, and image management.

In addition, all images are modified even when modification is not needed, which adds unnecessary overhead. The combined result is slow and unreliable response behavior.

## Options and Selected Direction
- Proposal 1 - Thread Pool: Use a pool of worker threads to manage concurrent image downloads.
- Proposal 2 - Architecture and Connection Management: Keep a persistent connection, reduce thread complexity, and only modify images when necessary.

The selected option is Proposal 2. It improves performance significantly, reduces architectural complexity around upload behavior, and creates a cleaner separation for connection handling and future extension.

## Techniques and Design Patterns
- Persistent connection management
- Conditional image modification
- Single-thread upload model
- Connection reuse strategy
- Socket lifecycle management

## Quality Attributes
The quality attributes are taken directly from the source:
- Performance (High priority)
- Maintainability (Medium priority)
- Extensibility (Medium priority)
