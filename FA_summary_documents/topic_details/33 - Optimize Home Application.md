# Optimize Home Application

## Basic Information
- Author: Tran Duc Cong
- Topic: Optimize Home Application (Performance Improvement)
- Year: 2023
- Project: JLR P-IVI AVN System

## Problem Summary
The Home application suffers from slow boot behavior, poor information consistency between home tiles and feature applications, and performance issues such as delay or stuck UI behavior. The source analysis points to inefficient data processing, redundant tile updates from multiple services, and unnecessary creation of all tile views even when they are not visible.

The architectural task is therefore to reduce boot cost and update overhead while keeping the application easier to manage and more responsive.

## Options and Selected Direction
- Proposal 1 - Generalize Home Tiles Architecture: Let features request tile updates through a generalized tile model. This reduces repeated processing but still leaves the tile layer dependent on feature-application behavior.
- Proposal 2 - MVC Architecture: Split responsibilities into Model, View, and Controller parts with lazy loading and selective tile creation/update.

The selected option is MVC architecture. It was chosen because it reduces boot time significantly, limits unnecessary view creation, and provides better structure for handling updates and screen transitions.

## Techniques and Design Patterns
- Model-View-Controller pattern
- Lazy-load component initialization
- Selective tile creation based on visibility
- Controller-based data processing
- Conditional update management

## Quality Attributes
The quality attributes are taken directly from the source:
- Performance - Booting time (High)
- Performance - View switch (Medium)
- Performance - App switch (High)
- Usability (High)
