# TimeManager for BAM in BMW ICON

## Basic Information
- Author: Pham Vu Binh
- Topic: TimeManager for BAM in BMW ICON
- Year: 2024
- Project: BMW ICON / ICONICC

## Problem Summary
This topic studies how TimeManager should collect, synchronize, and provide a valid system time from several different sources such as Backend, GNSS, IPB, and WUC. The original design has several weaknesses: TimeManager does not know promptly when a source is updated, polling is sequential and causes delay, access to Backend happens too frequently, and the system becomes too dependent on immediate source availability.

These issues reduce both time accuracy and service robustness. They also make the design harder to extend when new time sources or new requirements appear.

## Options and Selected Direction
- Static View Option 1 - Proxy Pattern: Use proxies to access time-source data, with emphasis on caching and reduced access cost.
- Static View Option 2 - Facade Pattern: Use a simple unified access interface, which is easier to understand and extend.
- Static View Option 3 - Hybrid Proxy-Facade Pattern: Combine the extensibility of Facade with the performance benefits of Proxy.
- Dynamic View Option 1 - Sequential Polling: TimeManager requests time values in sequence.
- Dynamic View Option 2 - Async Notification without maintained time: Proxies notify TimeManager when updates arrive, but no maintained time is kept.
- Dynamic View Option 3 - Async Notification with maintained time: Proxies maintain time locally and notify updates while reducing direct source access.

The selected direction combines the Hybrid Proxy-Facade pattern for the static architecture with async notification using maintained time in the dynamic behavior. This combination improves extensibility, reduces backend access, avoids unnecessary time jumps, and helps keep the time difference to the required target.

## Techniques and Design Patterns
- Proxy pattern
- Facade pattern
- Hybrid Proxy-Facade design
- Asynchronous notification
- Maintained time cache
- SOME/IP with PTP-based delay calculation
- Priority-based time-source selection

## Quality Attributes
The quality attributes are taken directly from the source:
- Reliability (High): System time will be kept available based on priority of valid time sources. If Backend Time available, system time needs to be secured with Backend Time to ensure certificate key working. Diagnostic Trouble Code (DTC) will be raised when there is no time source available within one minute
- Performance (High): The difference between system time and valid chosen time source need to be kept as small as possible (target: 100ms)
- Extensibility (Medium): Time manager will be easy to expand if there is any new requirement
- Maintainability (Medium): The structure and logic inside service need to be simple to understand and modify
