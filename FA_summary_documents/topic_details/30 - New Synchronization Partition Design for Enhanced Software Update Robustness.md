# New Synchronization Partition Design for Enhanced Software Update Robustness

## Basic Information
- Author: duc.phan
- Topic: New Synchronization Partition Design for Enhanced Software Update Robustness (Toyota 26BEV)
- Year: 2025
- Project: Toyota 26BEV

## Problem Summary
This topic addresses robustness problems in a dual-bank software update system. After an update completes, the active partition contains the new software while the inactive partition still contains the old version. If the BSP later boots from the wrong partition, the system can revert to old software and create rollback, compatibility, security, and stability problems.

The architectural objective is to synchronize partitions after update completion so the inactive side also contains the correct software image.

## Options and Selected Direction
- Proposal 1 - Nandwrite solution: Copy data using a temporary file and `nandwrite`. This is simpler, but requires much more storage and higher CPU usage.
- Proposal 2 - BufferWrite solution: Write directly from source partition to target partition without the temporary file. This is faster and lighter, but more complex in bad-block handling.

The selected option is BufferWrite. It was chosen because it provides better performance and lower CPU usage while still meeting the required robustness objective for partition synchronization.

## Techniques and Design Patterns
- Dual-bank partition synchronization
- NAND flash direct read/write
- Bad block table management
- Partition data mirroring
- MTD subsystem interaction

## Quality Attributes
The quality attributes are taken directly from the source:
- Performance (High)
- Reliability (High)
- Compatibility (Medium)
- Modifiability (Medium)
