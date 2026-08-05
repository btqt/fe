# ECall Application Skeleton Architecture

## Basic Information
- Author: Tien.Nguyen
- Topic: ECall Application Skeleton Architecture
- Year: 2023
- Project: Emergency Call Application / Tiger Platform 3.0

## Problem Summary
The current ECall implementations suffer from duplicated logic across regional or feature variants, especially in service communication. Another issue is that the state-machine design is applied poorly, causing one large class to accumulate too many responsibilities and making the source code hard to extend and maintain.

Because of this, the same fixes must be repeated in multiple ECall applications, and the system also pays a performance cost through redundant procedures and extra processes. The goal is to build a reusable application skeleton that can be adapted more easily to new variants.

## Options and Selected Direction
- New Design 1 - Add service adapter block: Commonize service communication logic through wrappers.
- New Design 2 - ECall Application Skeleton: Build the application from four main blocks: App main, Processor, Service wrapper, and Utils.

The selected option is New Design 2, the full ECall Application Skeleton. It creates a cleaner structure, reduces duplicated service logic, lowers process count, and makes future feature or variant changes easier to apply.

## Techniques and Design Patterns
- Service wrapper / adapter pattern
- Processor-based architecture
- Observer pattern
- Message queue handling
- Enum-based state definition

## Quality Attributes
The quality attributes are taken directly from the source:
- Performance (High priority)
- Reusability/Portability (Medium priority)
- Modifiability (Medium priority)
- Extensibility (Medium priority)
