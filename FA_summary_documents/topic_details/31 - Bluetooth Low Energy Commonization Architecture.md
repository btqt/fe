# Bluetooth Low Energy Commonization Architecture

## Basic Information
- Author: Manh.nguyen
- Topic: Bluetooth Low Energy Commonization Architecture
- Year: 2024
- Project: BMW ICON

## Problem Summary
The current BLE architecture is tightly coupled to a specific chipset stack inside BTManagerService. This makes it difficult to integrate a new BLE stack because chipset-specific behavior can affect existing components and create maintenance overhead.

The goal is to commonize the BLE architecture so new stacks can be added with minimal impact while keeping the code easier to maintain and extend.

## Options and Selected Direction
- Design 1 - Façade pattern approach: Introduce a façade adaptor layer to hide multiple BLE stacks behind one interface. This reduces impact on existing code but increases adaptor complexity.
- Design 2 - Strategy pattern approach: Build an abstract layer in the service and provide separate strategy adapters for different BLE stacks.

The selected option is the Strategy pattern approach. It supports cleaner extension, better compliance with the open-closed principle, and easier integration of future Bluetooth vendor stacks.

## Techniques and Design Patterns
- Strategy pattern
- Abstract service base class
- Concrete adapter classes
- Dependency inversion principle
- Abstraction layer for platform decoupling

## Quality Attributes
The quality attributes are taken directly from the source:
- Modifiability/Reusability (High)
- Maintainability (High)
- Extensibility (High)
- Performance (Medium)
- Reliability (Low)
