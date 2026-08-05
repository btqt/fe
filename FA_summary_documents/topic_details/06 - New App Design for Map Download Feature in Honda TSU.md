# New App Design for Map Download Feature in Honda TSU

## Basic Information
- Author: Cao Hai Dang
- Topic: New App Design for Map Download Feature in Honda TSU
- Year: 2024
- Project: Honda TSU (Honda MY26)

## Problem Summary
The topic designs a new Map Download application for Honda TSU to support autonomous-driving map workflows between MPU/ADAS and an external map server. TSU must handle high-frequency and high-volume requests while keeping the download completion time within targets under different vehicle-speed scenarios.

The architecture also has to satisfy technical constraints: the Map Download app acts as an HTTP server for MPU requests, uses `libCurl` for HTTPS communication to the server, and supports multiple concurrent connections (up to 5) with queueing and retry behavior.

## Options and Selected Direction
- Proposal 1 - Reuse Remote Interface Manager (RIM): Keep server communication in the existing RIM service and let MapDownloadApp focus on HTTP server and repository responsibilities.
- Proposal 2 - MapDownloadApp communicates directly with the server: Move server communication and thread-pool handling into MapDownloadApp for better direct data path performance.

The selected option is Proposal 1 (reuse RIM). The source decision states this option is more favorable for reusability, modifiability, and maintainability, while still meeting target download completion time in verification.

## Techniques and Design Patterns
- Service reuse and responsibility separation (single-responsibility-oriented split between app and RIM)
- Queue-based request handling (`MapDownloadQueue`) when maximum concurrent connections are reached
- Component-based internal decomposition (`HttpServer`, `MapDownloadOperator`, `MapDownloadStorage`)
- Multi-connection control with capped parallelism (5 connections)

## Quality Attributes
The quality attributes are taken directly from the source:
- Reliability | High | The TSU works stable even if using multiple connections while downloading
- Modifiability, Reusability | High | The application should be modular, break down into smaller and independent modules that perform specific function
- Performance | Medium | Minimize latency and avoid unnecessary CPU usage
- Maintainability | Medium | The module shall easy to maintain
