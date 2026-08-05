# Raw Document Content

- Source file: Final_duc_phan_FA_2025_New_Synchronization_Partition_Design_for_Enhanced_Software_Update_Robustness_in_Toyota_26BEV/Final_duc_phan_FA_2025_New_Synchronization_Partition_Design_for_Enhanced_Software_Update_Robustness_in_Toyota_26BEV.docx

LGE VS [TOYOTA] [26BEV]

New Synchronization Partition Design for Enhanced Software Update Robustness

About this document

Document Information

## Table
| Issuing authority | LGEDV – Vehicle Network Team |
| --- | --- |
| Configuration ID | Toyota 26BEV - Progmgr |
| Status of document | In Progress |

Revision History

## Table
| Version | Date | Comment | Author | Approver |
| --- | --- | --- | --- | --- |
| 1.0 | 2025-08-29 | Initial Release | Duc.phan |  |
| 2.0 | 2025-09-09 | Add SDD of Sync Partition component | Duc.phan |  |
| 3.0 | 2025-09-25 | - Update Related Documents - Update SAD diagram - Add proof of problem identification - Update Software update component diagram - Update proposal 1 diagram - Update proposal 2 diagram - Add Verify the architecture design - Add Results and Conclusion | Duc.phan |  |

MENU

1 Introduction	7

1.1 Purpose	7

1.2 Scope	7

1.3 Audience	7

1.4 Conventions	7

1.5 Acronyms / Glossary	7

1.6 Related Documents	8

2 Background	9

2.1 Software update mechanism	9

2.2 Problem description	10

3 Overview	13

3.1 System context Diagram	13

3.2 SW update components	14

3.2.1 ProgramMgr	14

3.2.2 UpdateHandler	15

3.2.2.1 DeltaUpdate	15

3.2.2.2 Synchronization Partition	15

4 Architectural Drivers	16

4.1 Quality Attributes	16

4.2 Constraints	16

5 Architectural Analysis	17

5.1 Architectural Alternatives and Tactics	17

5.2 Software update package structure	17

5.3 Proposal #1: Nandwrite solution - Store temporary file and using nandwrite	19

5.4 Proposal #2: BufferWrite solution - Write source partition data directly to target partition	21

5.5 Architecture decision	22

5.5.1 Architecture comparison	22

5.5.2 Architecture decision	23

5.6 Verify the architecture design	24

5.7 Results and Conclusion	25

5.7.1 Actual measurement	25

5.7.2 Future plan	26

6 SW Architectural Representations	27

6.1 Static Design	27

6.2 Dynamic design	29

6.3 Algorithm Design	30

6.3.1 SyncPartitionParser class	30

6.3.1.1 parseFromFile()	30

6.3.1.2 parseDeviceInfo()	31

6.3.1.3 parsePartition()	31

6.3.1.4 parseSyncTarget ()	32

6.3.1.5 getElementText()	33

6.3.1.6 getElementTextAsUint()	33

6.3.1.7 getElementTextAsInt()	34

6.3.1.8 isValidConfig ()	34

6.3.2 SyncPartitionManager class	35

6.3.2.1 extractConfig()	35

6.3.2.2 loadAndParseConfig()	36

6.3.2.3 syncPartitions()	37

6.3.2.4 extractSwPackageConfig()	38

6.3.2.5 extractSyncPartitionConfig()	39

6.3.2.6 createBlockTable()	40

6.3.2.7 scanBadBlocks()	41

6.3.2.8 readMtdInfo()	42

6.3.2.9 copyPartitionData()	43

6.3.2.10 readNandBlock()	44

6.3.2.11 writeNandBlock()	44

6.3.2.12 eraseNandBlock()	45

6.3.2.13 copyPartitionForNonUBI()	46

6.3.2.14 copyPartitionForUBI()	47

6.3.2.15 extractMtdNum()	48

6.3.2.16 cleanup()	49

6.3.2.17 cleanupExtractedFiles()	50

6.3.3 CVarFlash class	51

6.3.3.1 OpenFlash()	51

6.3.3.2 CloseFlash()	52

6.3.3.3 SeekFlash()	52

6.3.3.4 ReadImage()	53

6.3.3.5 WriteImage()	53

6.3.3.6 GetFlashSize()	54

6.3.3.7 GetBlockSize()	55

6.3.3.8 FSyncFlash()	56

6.3.3.9 CheckBadBlock()	56

6.3.3.10 EraseBlock()	57

Figures

Figure 1 Dual-bank update	9

Figure 2 Load Software problem	10

Figure 3 BSP load wrong partition in ICONN project	11

Figure 4 System context	13

Figure 5 Software update component diagram	14

Figure 6 Software update package structure	17

Figure 7 Synchronization partition configuration structure	18

Figure 8 Nandwrite solution	19

Figure 9 BufferWrite solution	21

Figure 10 Processing time chart	25

Figure 11 Class diagram	27

Figure 12 Sequence diagram of Synchronization partition	29

Figure 13 parseFromFile activity diagram	30

Figure 14 parseDeviceInfo activity diagram	31

Figure 15 parsePartition activity diagram	31

Figure 16 parseSyncTarget activity diagram	32

Figure 17 getElementText activity diagram	33

Figure 18 getElementTextAsUint activity diagram	33

Figure 19 getElementTextAsInt activity diagram	34

Figure 20 isValidConfig activity diagram	34

Figure 21 extractConfig activity diagram	35

Figure 22 loadAndParseConfig activity diagram	36

Figure 23 syncPartitions activity diagram	37

Figure 24 extractSwPackageConfig activity diagram	38

Figure 25 extractSyncPartitionConfig activity diagram	39

Figure 26 createBlockTable activity diagram	40

Figure 27 scanBadBlocks activity diagram	41

Figure 28 readMtdInfo activity diagram	42

Figure 29 copyPartitionData activity diagram	43

Figure 30 readNandBlock activity diagram	44

Figure 31 writeNandBlock activity diagram	44

Figure 32 eraseNandBlock activity diagram	45

Figure 33 copyPartitionForNonUBI activity diagram	46

Figure 34 copyPartitionForUBI activity diagram	47

Figure 35 extractMtdNum activity diagram	48

Figure 36 cleanup activity diagram	49

Figure 37 cleanupExtractedFiles activity diagram	50

Figure 38 OpenFlash activity diagram	51

Figure 39 CloseFlash activity diagram	52

Figure 40 SeekFlash activity diagram	52

Figure 41 ReadImage activity diagram	53

Figure 42 WriteImage activity diagram	53

Figure 43 GetFlashSize activity diagram	54

Figure 44 GetBlockSize activity diagram	55

Figure 45 FSyncFlash activity diagram	56

Figure 46 CheckBadBlock activity diagram	56

Figure 47 EraseBlock activity diagram	57

Tables

Table 1 Acronyms / Glossary	7

Table 2 Quality Attribute Requirements	16

Table 3 Constraint	16

Table 4 Architecture comparison of new design proposals	22

Table 6 Verification the architecture design	24

Table 5 Actual measurement of new design proposals	25

Table 7 Class description	28

1 Introduction

1.1 Purpose

This document specifies the software architecture for Synchronization Partition module of core NAD in TOYOTA 26BEV project. This design document also serves as a guideline on how Synchronization Partition module should be implemented and how the components should interact with each other.

1.2 Scope

This document describes the following about the Synchronization Partition module.

Architecture drivers

SW Architectural representations

Resource consumption objective

Interface design

Interaction design

Architectural alternatives

Software detailed design draft

1.3 Audience

The target audience of this document is:

Software architect who will evaluate the design of the software

Requirement engineer who will point out any contradiction between design and the requirement

Developer who will implement the design in actual code and interact with this software

TOYOTA 26BEV participants who want to understand the architecture of the Synchronization Partition module.

Test engineers who verify Software update feature for TOYOTA 26BEV project.

1.4 Conventions

1.5 Acronyms / Glossary

Table 1 Acronyms / Glossary

## Table
| No. | Acronyms | Description |
| --- | --- | --- |
| 1 | SAD | Software Architecture Design |
| 2 | SDD | Software Detailed Design |
| 3 | NAD | Network Access Device |
| 4 | ProgramMgr | Program Manager (Software update service in NAD) |
| 5 | MCU | Microcontroller Unit |
| 7 | BSP | Board Support Package |
| 8 | SW | Software |
| 9 | OTA | Over The Air |
| 10 | MTD | Memory Technology Devices |
| 11 | I/O | Input / Output |
| 12 | eMMC | Embedded Multi Media Card |

1.6 Related Documents

Memory Technology Device (MTD) Subsystem for Linux.

GitHub - vamanea/mtd-utils: mtd-utils

10. 26BEV Partition Table (NAND/eMMC) - <VS스마트제품개발담당> TOYOTA DCM - Collaboration Center

2 Background

2.1 Software update mechanism

Software update is the process of installing a newer version of the software on electronic devices such as computers, smartphones, telematics systems etc. This new version usually includes improvements, bug fixes, security patches, and sometimes new features.

Toyota 26BEV is a telematics system. It uses NAND flash memory to setup the system software. This memory is divided into partitions (banks) for applications, services to operate. It supports dual bank (dual partitions) software update mechanism.

In such dual bank architecture, a software upgrade mechanism is provided which uses Bank-A as SW executing region (active bank) and Bank-B as storage region for newer version of SW (inactive bank). Bank-A and Bank-B are decided to be active bank or inactive bank depending on Boot index. If Boot index points to any bank, that bank is active bank, the remaining bank will be inactive bank and vice versa. In Toyota 26BEV, BSP is responsible for managing Boot index and system boot up process.

![Document image](images/doc_image_001.png)
Image reference: doc_image_001.png

Figure 1 Dual-bank update

The SW in Bank-A (active bank) can erase Bank-B and program new SW to Bank-B without stopping software services of the system. The SW upgrade mechanism here also provides a rollback process to roll back to the original SW if a new SW woks abnormally. Such mechanism ensures the SW to be safe to update in any circumstance.

2.2 Problem description

As you know in Toyota 26BEV project, dual bank update mechanism is applied to update SW installed on NAND flash memory. Is there any problem that can occur when applying this mechanism?

Think about the problem after the new SW version is installed and activated successfully, at that time the SW version is installed on two different banks: one bank contains the old SW version, and the other bank contains the new SW version. What will happen when in a system reboot, the BSP boots up the system into the bank containing the old SW version?

![Document image](images/doc_image_002.png)
Image reference: doc_image_002.png

Figure 2 Load Software problem

Refer to the regarding problem in Toyota 26BEV project:
http://jira.lge.com/issue/browse/TMCBEV-2644

Refer to the regarding problem in ICONN project: https://jira.cc.bmwgroup.net/browse/ICONSD-119799

![Document image](images/doc_image_003.png)
Image reference: doc_image_003.png

Figure 3 BSP load wrong partition in ICONN project

If the system reboots and the BSP incorrectly selects the bank containing the old software version, several problems can arise:

Rollback to Previous Functionality: The system will revert to the older software version with its known bugs, potentially lacking critical security patches or new features. This can lead to unexpected behavior and compromise the system's stability and security.

Data Incompatibility: If the new software version introduced changes to data structures or file formats, booting the old version might lead to data corruption or incompatibility. The old software might not be able to correctly interpret data created by the newer version.

Feature Regression: Users might experience the loss of new features or improvements introduced in the updated software. This can lead to frustration and reduced functionality.

Security Vulnerabilities: Booting the old software might reintroduce known security vulnerabilities that were patched in the new version, making the system susceptible to attacks.

System Instability: In some cases, switching back to an older version after a newer version has been run can lead to system instability or unexpected crashes due to configuration changes or residual files from the newer version.

Diagnostics and Debugging Challenges: Troubleshooting issues becomes more complex when the system can boot into two different software versions. Determining the root cause of a problem requires knowing which version was running at the time of the issue.

How to mitigate this problem?

Enhanced robustness of BSP logic to minimize the possibility of that error:

Bank Selection Logic: The BSP must reliably select the bank containing the new software version after a successful update. Redundancy in this mechanism is crucial.

Version Checking: The BSP should verify the integrity and version of the software in each bank before booting. This can help detect corrupted updates or prevent accidental rollback.

Enhanced robustness of OTA SW update solution to minimize the possibility of that error:

Clear all inactive banks: All inactive banks contain the old SW version should to be cleared. It ensures that the System will not revert to the old SW version but it does not ensure that the BSP chooses to boot into an empty inactive bank, which can damage the system, services will not work and affect the users. This problem is more serious than the system running with old SW version.

Synchronize all active banks to the inactive banks: This solution will synchronize SW version on active banks to inactive banks. It ensures the system always works with new SW version even if BSP chooses wrong bank to boot up the system. It is highly recommended to prevent the problem of BSP.

3 Overview

3.1 System context Diagram

This is the software architectural design of Toyota 26BEV DCM. It consists of Tier-1, TMC and 3rd Party Software.

It has the layered architecture. Application Layer, Tiger Framework, TMC Specific Framework, Linux Libraries & Utilities and TMC software provided by OEM. (i.e Toyota Common Software)

ProgramMgr service is a component in Tiger Framework responsible for updating SW.

![Document image](images/doc_image_004.png)
Image reference: doc_image_004.png

Figure 4 System context

3.2 SW update components

![Document image](images/doc_image_005.png)
Image reference: doc_image_005.png

Figure 5 Software update component diagram

3.2.1 ProgramMgr

The ProgramMgr module is a process that controls all reprogramming installation stages. The main purpose of this module is to manage an update scenario when package is prepared and starting update is requested.

When the update package is downloaded and update request is received, ProgramMgr performs update software.
For Toyota 26BEV, it handles device reprogramming security feature such as verify signature, check tampering...

It deals with diagnostic requests like SID $31, $34, $36 etc...

3.2.2 UpdateHandler

UpdateHandler is responsible for updating SW - reading/writing updated data on memory area.

3.2.2.1 DeltaUpdate

DeltaUpdate is a part of UpdateHandler, it executes flashing using LGE update solution. The main purpose of this module is to update a target device when update packages are delivered. When ProgramMgr requests that the installation in target device be started, DeltaUpdate takes the following actions to update software.

- Extract SW update package
- Initialize attributes for update solution
- Start target device update

3.2.2.2 Synchronization Partition

Synchronization Partition is also a part of UpdateHandler, it is a new module. The main purpose of this module is to synchronize the new software on bank-A and bank-B, ensuring that the software on these two banks is the same after the software update process is completed.

After activating new software, the active bank has changed. Synchronization ensures the inactive bank is also updated to the latest version, creating consistency between the two banks. This is crucial for stable and reliable system operation.

If the active bank encounters a problem, switching to a synchronized inactive bank allows the system to resume normal operation with the same software version, avoiding conflicts or errors arising from version discrepancies.

When ProgramMgr requests that the synchronization in target device be started, Synchronization Partition module takes the following actions:

- Extract Synchronization Partition configuration from SW update package and load Synchronization Partition configuration.

- Initialize attributes for Synchronization solution such as partition information, good block table etc.

- Start to synchronize data for all reprogramming partitions.

4 Architectural Drivers

4.1 Quality Attributes

Table 2 Quality Attribute Requirements

## Table
| QA ID | Quality Attribute | Priority | Description |
| --- | --- | --- | --- |
| QA.01 | Performance | High | Need to optimize the use of system resources for sync partition: CPU usage, RAM, memory usage. |
| QA.02 | Reliability | High | Must ensure the system operates stably after updating the software. |
| QA.03 | Compatibility | Medium | Need to be able to accommodate system configuration changes without having to change the code. |
| QA.04 | Modifiability | Medium | The modification needs to keep as minimal as possible to avoid increasing the flashdriver size too much. |

4.2 Constraints

Table 3 Constraint

## Table
| ID | Constraint |
| --- | --- |
| C.01 | UpdateHandler module is designed to interact with system at low level. The new part synchronization partition must follow the current design. |
| C.02 | Ensures system operation even when switched to synchronized partition. |

5 Architectural Analysis

5.1 Architectural Alternatives and Tactics

To avoid interference with other OTA reprogramming stages, such as software installation, activation, or rollback, partition synchronization should ideally occur when the OTA master signals the conclusion of the update session by requesting the 26BEV to clear its reprogramming settings.

And Partition synchronization will operate in the background to minimize disruption to other system processes.

5.2 Software update package structure

To make Synchronization solution compatible with all projects, Software update package structure is adjusted, Synchronization partition configuration will be added as below:

## Table
| Software update config |
| --- |
| Package data |
| Sync partition config |

Figure 6 Software update package structure

This configuration part is in XML format and it based on the project’s partition table. It will contain necessary information of partitions such as PartitionName, PartitionSize, MountPath, Block device path. Refer to 26BEV partition table on this page http://collab.lge.com/main/pages/viewpage.action?pageId=2810646975

![Document image](images/doc_image_006.png)
Image reference: doc_image_006.png

Figure 7 Synchronization partition configuration structure

5.3 Proposal #1: Nandwrite solution - Store temporary file and using nandwrite

![Document image](images/doc_image_007.png)
Image reference: doc_image_007.png

Figure 8 Nandwrite solution

This partition synchronization solution employs a robust, albeit straightforward, method to ensure data integrity and minimize service disruption. The process begins by reading the complete data set from the source partition and storing it within a temporary file located in a designated, non-volatile memory area. This approach avoids potential data loss or corruption that could occur with direct partition-to-partition copying, especially in scenarios involving active system operations.

Following the complete data extraction, a system call is invoked to execute the nandwrite utility. This utility is specifically designed for writing data to NAND flash memory and incorporates mechanisms to handle bad blocks, a common occurrence in such storage media.  The temporary file containing the source partition data serves as the input for nandwrite, which then writes the data to the target partition. Nandwrite's inherent bad block management capabilities ensure that data is written to healthy sectors, preserving data integrity and preventing potential write failures.  This method effectively circumvents bad blocks, contributing to the reliability and robustness of the synchronization process.

This two-stage process of reading to a temporary file and then utilizing nandwrite offers several advantages.  It provides a complete and accurate copy of the source partition to the target partition, safeguards against data corruption due to bad blocks, and, by operating primarily in the background, minimizes the impact on other system processes.  Furthermore, this approach simplifies error handling and recovery.  If an error occurs during the nandwrite operation, the original source partition remains unaffected, allowing for retry attempts or alternative recovery strategies.

The requirement to store the entire source partition in a temporary file necessitates a significant amount of available memory. This memory overhead can be substantial, especially when dealing with large partitions. Furthermore, the process of reading from the source partition, writing to the temporary file, and then reading from the temporary file to write to the target partition involves extensive I/O operations, which consume considerable CPU cycles. This increased demand for both memory and CPU resources results in costly operating costs.

5.4 Proposal #2: BufferWrite solution - Write source partition data directly to target partition

![Document image](images/doc_image_008.png)
Image reference: doc_image_008.png

Figure 9 BufferWrite solution

An alternative partition synchronization solution bypasses the intermediary temporary file and direct nandwrite usage, opting for a more resource-conscious approach. This method reads data from the source partition incrementally into a designated buffer in memory.  This buffer is then written directly to the target partition.  By avoiding the creation and subsequent reading of a large temporary file, this method significantly reduces the storage footprint and minimizes the overall I/O operations, leading to performance gains, especially in systems with limited memory resources.

Crucially, this direct buffer-to-partition write method necessitates the implementation of custom bad block management.  Unlike leveraging the built-in capabilities of nandwrite, this solution requires the synchronization process to actively identify and handle bad blocks during the write operation.  This typically involves maintaining a map of known bad blocks, either pre-generated or dynamically updated, and implementing logic to skip or remap these sectors during the write process.  While adding complexity to the implementation, this custom bad block handling allows for finer control and potential optimization tailored to the specific characteristics of the target NAND flash memory.

This streamlined approach offers several advantages in resource-constrained environments.  By eliminating the need for a temporary file, it minimizes memory usage and reduces the overall execution time.  The direct write operation also reduces CPU overhead associated with file system operations.  However, the added complexity of implementing custom bad block management requires careful consideration and thorough testing to ensure data integrity and prevent potential data loss due to write failures on faulty sectors.  This trade-off between resource efficiency and implementation complexity makes this solution particularly suitable for Toyota 26BEV – an embedded systems.

5.5 Architecture decision

5.5.1 Architecture comparison

Table 4 Architecture comparison of new design proposals

## Table
|  | Proposal #1 | Proposal #2 |
| --- | --- | --- |
| Advantages | Simplicity and Robustness: Leverages the established nandwrite utility, simplifying implementation and benefiting from its built-in bad block management. This reduces the risk of data corruption due to bad blocks and ensures data integrity. Easier Error Handling: The source partition remains untouched until the nandwrite operation completes. This simplifies error handling and recovery, allowing for retries or alternative strategies if the write operation fails. | Low Memory Footprint: Eliminates the need for a temporary file, significantly reducing memory usage. This makes it suitable for memory-constrained systems. Reduced CPU Overhead: Fewer I/O operations and no file system management overhead result in lower CPU usage and faster execution times. Lower Cost: Reduced resource consumption translates to lower operational costs. Potential for Optimization: Custom bad block management allows for fine-grained control and potential optimization tailored to the specific characteristics of the NAND flash memory. |
| Disadvantages | High Memory Usage: Requires significant memory to store the entire source partition in a temporary file. This can be problematic for systems with limited memory resources. Higher CPU Overhead: Involves more I/O operations (reading and writing to the temporary file), leading to increased CPU usage and potentially longer execution times. Higher Cost: The increased demand on memory and CPU resources translates to higher operational costs, especially in resource-constrained environments. | Increased Implementation Complexity: Requires implementing custom bad block management, adding complexity to the development and testing process. |

5.5.2 Architecture decision

Based on the comparison results of the 2 proposals, and depends on specific requirements on quality attributes and constraints of the system - Minimizing resource usage, maximizing performance and ensuring system stability are paramount.

So the proposal #2 is selected to implement.

5.6 Verify the architecture design

To confirm the system's proper functionality and demonstrate the new solution's effectiveness in addressing the identified issue, the following verification steps are necessary:

Table 6 Verification the architecture design

## Table
| No. | Verification step | Current logic | Apply proposal solution |
| --- | --- | --- | --- |
| 1 | Check SW version of the active partition | / # cat /etc/version TdcLZ_Cc_2536_0041 | / # cat /etc/version TdcLZ_Cc_2536_0041 |
| 2 | Change Boot index to inactive partition | / # sldd progmgr requestjob 131076 3 2 requestJob ret : 0 outBuffer_(0) : 1 OK_done (0) | / # sldd progmgr requestjob 131076 3 2 requestJob ret : 0 outBuffer_(0) : 1 OK_done (0) |
| 3 | Check SW version of the current partition | / # cat /etc/version TdcLZ_Cc_2533_0099 | / # cat /etc/version TdcLZ_Cc_2536_0041 |
| 4 | Check System boot up successfully or not | / # sldd am get_bootcomplete start get bootcomplete ============================== Success get bootcomplete 1: ============================== | / # sldd am get_bootcomplete start get bootcomplete ============================== Success get bootcomplete 1: ============================== |
| 5 | Compare SW version of both partitions | Not same | Same |

5.7 Results and Conclusion

5.7.1 Actual measurement

Table 5 Actual measurement of new design proposals

## Table
| No. | Partition | Partition Size (KB) | Processing Time (ms) | Processing Time (ms) | CPU Usage (%) | CPU Usage (%) |
| --- | --- | --- | --- | --- | --- | --- |
| No. | Partition | Partition Size (KB) | Proposal #1 (Nandwrite) | Proposal #2 (BufferWrite) | Proposal #1 (Nandwrite) | Proposal #2 (BufferWrite) |
| 1 | tz_devcfg | 768 | 179 | 169 | 55% | 46% |
| 2 | keymaster | 1024 | 232 | 181 | 55% | 46% |
| 3 | cmnlib64 | 1536 | 364 | 256 | 55% | 46% |
| 4 | qhee | 2048 | 467 | 347 | 55% | 46% |
| 5 | uefi | 3584 | 766 | 608 | 55% | 46% |
| 6 | boot | 34816 | 7486 | 5819 | 55% | 46% |
| 7 | System | 411136 | 90169 | 71695 | 55% | 46% |
| Total | Total | Total | 99663 | 79075 |  |  |

![Document image](images/doc_image_009.png)
Image reference: doc_image_009.png

Figure 10 Processing time chart

The above measurements reflect the actual performance results when applying the two designs. Proposal #2 significantly optimizes CPU usage and processing time, demonstrating superior performance in partition synchronization compared to Proposal #1.

5.7.2 Future plan

Partition synchronization, crucial for system stability, is planned for implementation in the TOYOTA 26BEV project in December 2025 as part of a robustness activity.  Following this, its application in other projects will be evaluated.

6 SW Architectural Representations

6.1 Static Design

In the new design, ProgramMgr adds 3 classes for synchronizing partition.

SyncPartitionManager

SyncPartitionParser

CVarFlash

![Document image](images/doc_image_010.png)
Image reference: doc_image_010.png

Figure 11 Class diagram

The classes identified above are described in the below table:

Table 7 Class description

## Table
| Class/File | Description |
| --- | --- |
| CUpdateHandler | It is main class receiving all request from ProgramMgr |
| CDeltaProcess | It is in core part, responsible for installing new Software |
| CVarDeltaProc | It is in variant part, responsible for installing new Software |
| COpmodeProcess | It is in core part, responsible for extending process such as synchronize partition, verify software version etc. |
| CVarOpmodeProc | It is in variant part, responsible for extending process such as synchronize partition, verify software version etc. |
| CVarFlash | It works with partition memory: read/write data, check bad block |
| CPackage | It provides some function to extract the elements in a software package |
| CConfig | It parses software update configuration |
| CUtils | It provides some APIs to run system command |
| TiXmlDocument | It is an open source lib that provides APIs to parse xml file |
| SyncPartitionManager | It is a new class, responsible for synchronizing partition |
| SyncPartitionParser | It is a new class, responsible for parsing synchronization partition configuration |
| SyncPartitionConfig, SyncTarget, Partition, SwupPackageInfo | They are new structures to save parsed synchronization partition configuration |

6.2 Dynamic design

![Document image](images/doc_image_011.png)
Image reference: doc_image_011.png

Figure 12 Sequence diagram of Synchronization partition

6.3 Algorithm Design

6.3.1 SyncPartitionParser class

6.3.1.1 parseFromFile()

![Document image](images/doc_image_012.png)
Image reference: doc_image_012.png

Figure 13 parseFromFile activity diagram

6.3.1.2 parseDeviceInfo()

![Document image](images/doc_image_013.png)
Image reference: doc_image_013.png

Figure 14 parseDeviceInfo activity diagram

6.3.1.3 parsePartition()

![Document image](images/doc_image_014.png)
Image reference: doc_image_014.png

Figure 15 parsePartition activity diagram

6.3.1.4 parseSyncTarget ()

![Document image](images/doc_image_015.png)
Image reference: doc_image_015.png

Figure 16 parseSyncTarget activity diagram

6.3.1.5 getElementText()

![Document image](images/doc_image_016.png)
Image reference: doc_image_016.png

Figure 17 getElementText activity diagram

6.3.1.6 getElementTextAsUint()

![Document image](images/doc_image_017.png)
Image reference: doc_image_017.png

Figure 18 getElementTextAsUint activity diagram

6.3.1.7 getElementTextAsInt()

![Document image](images/doc_image_018.png)
Image reference: doc_image_018.png

Figure 19 getElementTextAsInt activity diagram

6.3.1.8 isValidConfig ()

![Document image](images/doc_image_019.png)
Image reference: doc_image_019.png

Figure 20 isValidConfig activity diagram

6.3.2 SyncPartitionManager class

6.3.2.1 extractConfig()

![Document image](images/doc_image_020.png)
Image reference: doc_image_020.png

Figure 21 extractConfig activity diagram

6.3.2.2 loadAndParseConfig()

![Document image](images/doc_image_021.png)
Image reference: doc_image_021.png

Figure 22 loadAndParseConfig activity diagram

6.3.2.3 syncPartitions()

![Document image](images/doc_image_022.png)
Image reference: doc_image_022.png

Figure 23 syncPartitions activity diagram

6.3.2.4 extractSwPackageConfig()

![Document image](images/doc_image_023.png)
Image reference: doc_image_023.png

Figure 24 extractSwPackageConfig activity diagram

6.3.2.5 extractSyncPartitionConfig()

![Document image](images/doc_image_024.png)
Image reference: doc_image_024.png

Figure 25 extractSyncPartitionConfig activity diagram

6.3.2.6 createBlockTable()

![Document image](images/doc_image_025.png)
Image reference: doc_image_025.png

Figure 26 createBlockTable activity diagram

6.3.2.7 scanBadBlocks()

![Document image](images/doc_image_026.png)
Image reference: doc_image_026.png

Figure 27 scanBadBlocks activity diagram

6.3.2.8 readMtdInfo()

![Document image](images/doc_image_027.png)
Image reference: doc_image_027.png

Figure 28 readMtdInfo activity diagram

6.3.2.9 copyPartitionData()

![Document image](images/doc_image_028.png)
Image reference: doc_image_028.png

Figure 29 copyPartitionData activity diagram

6.3.2.10 readNandBlock()

![Document image](images/doc_image_029.png)
Image reference: doc_image_029.png

Figure 30 readNandBlock activity diagram

6.3.2.11 writeNandBlock()

![Document image](images/doc_image_030.png)
Image reference: doc_image_030.png

Figure 31 writeNandBlock activity diagram

6.3.2.12 eraseNandBlock()

![Document image](images/doc_image_031.png)
Image reference: doc_image_031.png

Figure 32 eraseNandBlock activity diagram

6.3.2.13 copyPartitionForNonUBI()

![Document image](images/doc_image_032.png)
Image reference: doc_image_032.png

Figure 33 copyPartitionForNonUBI activity diagram

6.3.2.14 copyPartitionForUBI()

![Document image](images/doc_image_033.png)
Image reference: doc_image_033.png

Figure 34 copyPartitionForUBI activity diagram

6.3.2.15 extractMtdNum()

![Document image](images/doc_image_034.png)
Image reference: doc_image_034.png

Figure 35 extractMtdNum activity diagram

6.3.2.16 cleanup()

![Document image](images/doc_image_035.png)
Image reference: doc_image_035.png

Figure 36 cleanup activity diagram

6.3.2.17 cleanupExtractedFiles()

![Document image](images/doc_image_036.png)
Image reference: doc_image_036.png

Figure 37 cleanupExtractedFiles activity diagram

6.3.3 CVarFlash class

6.3.3.1 OpenFlash()

![Document image](images/doc_image_037.png)
Image reference: doc_image_037.png

Figure 38 OpenFlash activity diagram

6.3.3.2 CloseFlash()

![Document image](images/doc_image_038.png)
Image reference: doc_image_038.png

Figure 39 CloseFlash activity diagram

6.3.3.3 SeekFlash()

![Document image](images/doc_image_039.png)
Image reference: doc_image_039.png

Figure 40 SeekFlash activity diagram

6.3.3.4 ReadImage()

![Document image](images/doc_image_040.png)
Image reference: doc_image_040.png

Figure 41 ReadImage activity diagram

6.3.3.5 WriteImage()

![Document image](images/doc_image_041.png)
Image reference: doc_image_041.png

Figure 42 WriteImage activity diagram

6.3.3.6 GetFlashSize()

![Document image](images/doc_image_042.png)
Image reference: doc_image_042.png

Figure 43 GetFlashSize activity diagram

6.3.3.7 GetBlockSize()

![Document image](images/doc_image_043.png)
Image reference: doc_image_043.png

Figure 44 GetBlockSize activity diagram

6.3.3.8 FSyncFlash()

![Document image](images/doc_image_044.png)
Image reference: doc_image_044.png

Figure 45 FSyncFlash activity diagram

6.3.3.9 CheckBadBlock()

![Document image](images/doc_image_045.png)
Image reference: doc_image_045.png

Figure 46 CheckBadBlock activity diagram

6.3.3.10 EraseBlock()

![Document image](images/doc_image_046.png)
Image reference: doc_image_046.png

Figure 47 EraseBlock activity diagram
