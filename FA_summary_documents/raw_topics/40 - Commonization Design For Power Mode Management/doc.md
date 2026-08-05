# Document Content

**Source:** FA_Thuong4_nguyen_Commonization_Design_For_Power_Mode_Management_v1.0_Final.docx

FA Project: Commonization Design of Power Mode Management In  PowerManager
Software Design Document
TABLE OF CONTENT
1.	About this Document	3
1.1	Document Information	3
1.2	Revision History	3
1.3	Purpose	3
1.4	Scope	4
1.5	Related Documents	4
1.6	Abbreviations / Terms	4
2.	Project Context	5
2.1	Introduction	5
2.2	SW Architecture	7
3.	Architectural Driver	11
3.1	Problem Description	11
3.1.1	Code Duplication and Cascading Complexity:	11
3.1.2	Maintenance and Scalability Problems	12
3.2	Functional Requirements and Quality Attributes	13
3.2.1	Functional Requirement	13
3.2.2	Quality Attributes	14
4.	Architectural Alternatives	15
4.1	Solution 1: BitMask-Driven State Machine	15
4.1.1	State Pattern	17
4.1.2	State Transition Policy for encapsulating State transition conditions	18
4.1.3	Factory Pattern to create states of State Machine	19
4.1.4	Bit-Mask Pattern for processing power events and checking state transition	20
4.2	Solution 2: New Component With Strategy Pattern Approach	23
5.	Architectural Diagram	26
5.1	Proposal 1: Design New Component Based-BitMask Approach	26
5.1.1	PowerManager Component Diagram	26
5.1.2	Class Diagram	29
5.1.3	Dynamic View	31
5.2	Proposal 2: Design New Component Based-Data Structure Approach	35
5.2.1	PowerManager Component Diagram	35
5.2.2	Class diagram of  Power Variant	36
5.2.3	Class diagram of  PowerModeEventHandler	37
5.2.4	Dynamic View	38
6.	Comparison between the alternatives	39
7.	Architectural decision and rationale	40
7.1	Architectural decision	40
7.2	Conclusion	41
8.	How to verify final architecture design	42
8.1	Implementation	42
8.2	Design Verification	47
9.	Conclusion	50
9.1	Conclusion	50
9.2	Area for Improvement	50
9.3	Future Plan	50

# About this Document

## Document Information

## Revision History

## Purpose
This document specifies the common architecture for power mode management in the TCUA project.
Besides that, the design document also serves as a guideline for software component implementation and inter-component communication within the system.

## Scope
This document specifies the Software Detailed Design of PowerManager in TCUA project.
Architectural Drivers
Bit Mask-Driven Power State Machine
Power Mode Management Design
Architectural Alternatives

## Related Documents
[1]	EVAC_TCUA_Core_SRD
[2]	EVAC_eCall_SRD
[3]	EVAC_bCall_SRD
[4]	EVAC_SVT_SRD
[5]	STJLR_CoreTelematics

## Abbreviations / Terms

# Project Context

## Introduction
Background:
The TCUA (Telematics Control Unit Module-A) is critical telematics component in the JLR-EVA3 (Jaguar Land Rover Electric Vehicle Architecture 3) project. This module enables seamless communication between the vehicle and external networks, devices, supporting a comprehensive range of functionalities including:
Over-the-air (OTA) software updates.
Safety and critical applications (eCall, bCall, SVT, …)
Enabling Remote vehicle monitoring and debugging feature.
System Architecture:
The TCUA employs hardware architecture consisting of three main components as following:
AP (Application Processor): where user applications and high-level software services execute
CP (Connectivity Processor): responsible for providing external connectivity interfaces and network communication protocols.
MCU (Microcontroller Unit): responsible for low-level device communication, interfacing with external ECUs and sensors.
Figure 1. TCUA HW Components
Power Mode of System and sub-system
Figure 2. Power mode table of TCUA and sub-system
In accordance with hardware power consumption requirements, the TCUA system implements three primary power modes: Normal, Listen, and Sleep.
Normal Mode: The system operates at full functionality.
Listen Mode: Suspends all applications and services except essential communication processor (CP) services. Achieves very low power consumption while maintaining rapid wake-up capability in response to external events such as remote wake-up and network wake-up.
In Listen Mode, the Application Processor (AP) can perform self-wake-up via its Real-Time Clock (RTC) to execute scheduled maintenance tasks.
Sleep Mode: All components are shutdown, achieving near-zero current consumption with only minimal power required for the MCU-RTC component. The system supports self-wake-up through MCU-RTC timers or external wake-up events including network wake-up, eCall button, and other designated trigger sources. Remote control functionality is unavailable during Sleep Mode.
Power Management Context
Effective power management is critical for optimizing system performance and extending battery life in automotive embedded systems. The PowerManager component within the Application Processor (AP) orchestrates power mode transitions across TCUA subsystems, coordinating their operational states based on the following key dependencies:
OEM-specific power management policies and requirements.
Sub-system priority (the order of power transition).
Critical safety and communication application states
The following sections detail the software architecture and implementation of the power management system for TCUA.

## SW Architecture
Overview
In the telematics system’s software architecture for the JLR project as the figure below, the PowerManager component is located within the Tiger Framework layer and operates as a dedicated service on the Application Processor (AP), delivering centralized power management capabilities across all subsystems.
Figure 3. TCUA SW Architecture
PowerManager interacts with other services, applications to process Power Mode Events and then determines the state transition to optimize system power consumption according to OEM power mode strategy.
Figure 4. External Communication of PowerManager
OEM Power State Machine
The PowerManager serves as the master controller in the power management architecture, orchestrating coordinated power mode transitions across subsystems according to the state machine defined below:
Figure 5. TCUA Power State Machine
Core Strategy
The state machine implements a hierarchical power management approach that:
Automatically adapts to vehicle conditions (network activity, emergency status, …)
Prioritizes critical functions while aggressively managing non-essential power consumption.
Provides deterministic behavior for power transitions based on well-defined triggers.
Supports regulatory compliance for automotive safety and emergency communication requirements.
This state machine describes power transitions strategy in TCUA, with a variety of states and multiple transition conditions.
Power Manager Static View
Figure 6. PowerManager Static view
Core Functionality:
Power Mode Event Processing: Processes power mode events from services and applications across AP, CP, and MCU sub-systems.
State Transition Management: Transitions the power mode of the entire system based on OEM requirements and operational policies.
Sub-system Coordination: Controls each sub-system's power mode to ensure consistent power states across the distributed architecture.
Power Manager’s current Design (block diagram)
Note: This diagram only shows Power Components for Power Management feature
Figure 7. TCUA Powermanager component diagram
The PowerManager design for TCUA bases on the TigerFramework's PowerManager architecture.
The PowerManager architecture consists of two distinct components: core part and variant part. The core part provides common functionality applicable across multiple OEMs utilizing the TigerFramework, while the variant part is customized to meet specific OEM requirements and implementation details.
All power mode events from system resources are centrally processed by the PowerExtService component. This component evaluates the current power mode status against predefined power-state-machine requirements and initiates mode transitions through the power management core when conditions are satisfied.
When power mode of the system changes, Power core will control sub-systems according to predefined subsystem state configurations (shown in Figure 2).

# Architectural Driver

## Problem Description
The power mode requirements for TCUA system necessitate complex event-driven state transitions involving multiple conditions with varying priorities and overlapping power mode events.
However, so far, there is no specific design for it. The current PowerExtService implementation of PowerManager employs a sequential checking approach to process events, evaluate transitions, and coordinate power mode changes according to OEM-specific strategies. This evaluation method encounters significant architectural challenges as following:

### Code Duplication and Cascading Complexity:
Power mode events frequently appear across multiple state transition conditions, creating a cascading evaluation problem. When any single event changes state, the system must re-evaluate all conditions containing that event, resulting in:
Redundant Logic: Identical event checks replicated across multiple condition functions.
Performance Degradation: O (N×M) complexity where N = events per condition, M = total conditions.
Code Bloat: Exponential growth in source code size as conditions increase.
Maintenance Overhead: Event modifications require updates across multiple condition implementations.
For example, in below requirements of TCUA, many events are overlapped in state transition conditions
Additionally, state transition conditions must be evaluated according to a predefined priority hierarchy to ensure deterministic system behavior. When multiple conditions are simultaneously satisfied, higher priority conditions override lower priority ones. However, the sequential checking approach implements this priority management through hard-coded nested if-else chains, creating inflexible and maintenance-intensive code structures.
Example: Priority override scenario:
Condition 1: Polling state transition requirements are met
Condition 2:  Listen state transition when eCall is in callback state is also met
Result: System transitions to Listen state due to higher priority of bCall conditions
Another case:  when SVT is active, PowerManager shall transition according to SVT requirement other than following Power Mode requirement.

### Maintenance and Scalability Problems
High Development Overhead for System Changes:
Adding New Conditions:
Create condition function → Update priority sequence → Modify event handler sequence → Test for conflicts.
Modifying Existing Conditions:
Update function logic → Update documentation → Re-test interactions → Validate side effects.
Changing Priorities:
Reorder checking sequence → Update calling code → Re-validate all transition logic.
Each modification requires cascading changes across multiple code modules and extensive regression testing to ensure system stability.
Prone to Side effects by other logics changes regarding power mode events.
To address the challenges identified above, I would like to propose a new architectural design to enhance PowerManager stability, maintainability, and scalability while ensuring functionality and performance. This solution provides a reusable power mode management that can be leveraged across future projects.

## Functional Requirements and Quality Attributes

### Functional Requirement

### Quality Attributes

# Architectural Alternatives
To address the Power Mode Complexity Management challenges identified above, the following architectural solutions are proposed:
Bit-Mask-Driven State Machine: Create a common component responsible for centralized complex power mode event management and state transition.
Strategy Pattern For Event-Processing and State Transition Evaluation: is an alternative implementation option.
Design Target
Functionality: Provide a common design for power mode management in telematics systems.
Scalability: Easy to add new state transition scenarios or state transition conditions.
Reusability: The design can be applied across different projects
Maintainability: Clear separation of concerns and configuration-driven approach

## Proposal 1: Bit-Mask-Driven State Machine
Overall concept of Bit-Mask-Driven Power State Machine
Figure 8. The overall concept of BitMask-Driven PowerStateMachine
Detailed Concept
Event Consolidation via Bit-Mask Pattern:
Instead of managing events as separate variables, all events are encoded as individual bits in a single integer.
Pattern-Based State Transitions: 
State transitions are determined by bitwise pattern matching rather than sequential condition checking —where each event condition is evaluated individually through if-else statements or loops at different code locations.
Separation of Event Logic from State Logic: 
Event processing (Bit-Mask operations) is separated from state behavior (State pattern).
Configuration-Driven Transition Policies: 
Transition conditions are defined as data structures rather than hardcoded logic. Each state has own policies to transition to other states.
Composite Event Conditions: 
Complex transitions require multiple-events condition, handled efficiently through bitwise operations.
Design Deployment
Figure 9. Structure of Bit-Mask-Driven Power State Machine
This design implements a hybrid pattern that combines:
State Pattern: For state behavior management and transition.
Factory and Strategy Pattern: For state creation, encapsulate to reduce implementation effort.
Bit-Mask Pattern: For efficient power-mode-event processing which is responsible by PowerModeEventBiSet.
Observer Pattern: For state change notifications.

### State Pattern
Based on OEM power management requirements shown in the figure 5 and Functional requirements, the State Pattern design architecture is selected to implement the power management feature.
Figure 10. State Pattern design
Implementation Architecture:
Abstract Interface (IState): Defines common state operations including checkStateTransition(), enter(), exit(), getStateId(), and setTransitionPolicy()
Concrete State Classes: Five primary states (Normal, Listen, Polling, Sleep, BackupBattery) each implementing state-specific behavior
Encapsulated state-specific logic for maintainability.
State Transition Mechanism:
Each state implements the checkStateTransition() method using BitMask-based condition evaluation.
State transitions are determined through power event conditions evaluation against configurable transition policies, ensuring priority based decision-making.

### State Transition Policy for encapsulating State transition conditions
Current challenge as mentioned in Chapter 3:
OEM requirements mandate that each power state support multiple transition targets with different trigger conditions, all evaluated in priority order. Implementing this individually for each state would result in code duplication and maintenance complexity.
Solution:
Implements a policy-based transition architecture where each power state maintains a set of configurable transition policies. Each policy encapsulates the following:
A target state:
Required conditions (Bit-Mask patterns).
Benefits:
This design commonizes transition logic across all states, enables flexible configuration for different TCU variants, and ensures consistent priority-based evaluation while maintaining code maintainability.
A state can have multiple policies, each policy is arranged in a specific order according to it’s priority in the OEM requirement. This can be easily for maintaining compared to hard-code in if-else statement.
Figure 11. Transition Policies for each state

### Factory Pattern to create states of State Machine
In State Pattern design mentioned above, there are many states that share the same logic and mechanisms. With a traditional implementation, each state machine would need to be deployed using the same pattern and core logic, but with different configurations (transition policy). This approach leads to increased implementation effort and code duplication.
Therefore, through Factory design pattern we can minimize redundant code and simplify the process of adding or modifying states.
Figure 12. State Transition Checking Algorithm
This Factory pattern provides the following benefits:
Policy-driven state creation: Creates states with pre-configured transition policies
Easy extensibility: New states can be added through configuration changes only, without code modifications
Code reuse: Minimizes redundant implementation across different states
Centralized management: Consolidates state creation logic and policy definitions in a single location

### Bit-Mask Pattern for processing power events and checking state transition
The design pattern employs a Bit-Mask approach that uses individual bits within a single integer (bitSet) to represent the current state of all power mode events. When power mode events change, the corresponding bits in the bitSet are updated accordingly.
Figure 13. Bit-Mask  for power mode events
Handle power mode events:
Use a bitSet (single integer) to store the current state of all power mode events, where each power mode event is assigned a unique bit position within the bitSet. When power mode events change state, the corresponding bits are updated accordingly:
Set bit to 1 when the power mode event becomes ACTIVE
Clear bit to 0 when the power mode event becomes INACTIVE
Figure 14. Power Mode Events on BitSet
Check state transition condition
Each state has transition policies that defines the specific power mode events required for transitioning to the next state.
After updating power mode events in the BitSet, the current state performs bitwise pattern matching against the predefined event masks in the policy to determine the next valid transition.
When the pattern comparison matches, the corresponding state transition condition is satisfied and the current state will return the next state to the PowerStateMachine.
PowerStateMachine will trigger the transition to the nextState
Figure 15. State transition checking by conditions-mask
Overall State Transition Checking-Flow
Figure 16. State Transition Checking-Flow
Detail of checking state transition in each state
Figure 17. State Transition Checking-Logic

## Proposal 2: New Component with Strategy Pattern Approach
Key concept
The strategy pattern approach uses a table of transition Policies specinfying the power mode event condtions, preserving rich semantic information like inactive, sleep, active, expired, and operational modes beyond simple binary states.
State transition validation operates through comparison of current power-mode-event status against power-state-machine requirements defined in the policies.
Policies are arranged in the priority order so it is easy to maintain compared with hard-code in if-selse statement.
This method prioritizes flexibility and semantic clarity over performance, trading computational efficiency for extensibility, maintainability, and debugging transparency in complex automotive power management system
Figure 19. Strategy pattern concept
Core components of this approach:
CurrentPmEventStatus: To store the current status of power mode events. When the status of power mode events changes, it will be updated to CurrentPmEventStatus then check the state transition by comparing with stateTransitionStrategy.
StateTransitionStrategy: This defines policies which include the next state and the conditions of power mode events.
Structure of Policies for checking state transitions
Figure 18. Data Structure For State Transition Set
Handle power mode events:
When power mode events change, the PowerModeEventHandler updates status of event to the table that stores the current status of power mode events.
Figure 20. Update power mode events to current event status storage
Check state transition conditions
During state transition checks, the PowerModeEventHandler:
Iterates through all TransitionStrategy policies
Compares each policy's expected power mode event status with the current status in CurrentEventStatus
When all expected states in a policy match the current event status, stops the search and returns the policy's specified next state
Figure 21.  State Transition Checking

# Archtectural Diagram

## Proposal 1: Design New Component Based-BitMask Approach

### PowerManager Component Diagram
Below image shows the PowerManager’s components regarding Power Mode Management.
Figure 22. PowerManager static view
In the static view, there is new common design for OEM power state management (Bit-Mask-Driven Power State Machine component in green color).
This component is dedicated for handling power mode events and managing the state transition based of OEM requirements.
Detailed Component Desciption
Detailed structure of Bit Mask-Driven Power State Machine component
Figure 23. Structure of Bit Mask-Driven Power State Machine
Operational Flow:
Setup: PowerStateMachine initializes using StateMachineCreator, which builds states from StateTemplate, IStateMachineCallBack and StateTransitionPolicy configurations
Event Handling: PowerStateMachine receives power mode events and updates the internal PowerModeEventBitSet
Transition Check: Current state (via IState interface) evaluates transition conditions using Bit-Mask comparison against configured policies
State Change: When transition conditions are met, PowerStateMachine switches to the new state and executes entry/exit actions
Observer Notification: IStateMachineCallBack interface notifies registered observers of the state change for external coordination.

### Class Diagram
Classes Diagram of BitMask-Driven Power State Machine design
Figure 24. Class Diagram of PowerStateMachine design
Enums & Utilities Package:
EventMask: BitMask-based events for efficient operations of processing power mode events
StateId: Power state enumeration (NORMAL, SLEEP, LISTEN, etc.)
TransitionPolicy: containing transition conditions corresponding to each State.
ClearMask: Predefined masks for clearing multiple events
StateMachineUtil: Utility class for string conversions
Interfaces Package:
IStateMachineCallback: Observer pattern for state change notifications
IState: State pattern interface defining state behavior
Core Classes Package:
StateTemplate: Abstract base class implementing common state behavior.
StateMachineCreator: Factory pattern for creating states and policies.
TCUAPowerStateMachine: Main state machine context managing the entire state transition of system.
Design Patterns:
State Pattern: IState interface with StateTemplate implementation.
Observer Pattern: IStateMachineCallback for state change notifications.
Factory Pattern: StateMachineCreator for state creation.
Template Method: StateTemplate provides common implementation for states.
BitMask Pattern: EventMask for efficient event management.
Key Relationships:
Inheritance: StateTemplate implements IState
Composition: TCUAPowerStateMachine contains multiple IState instances
Dependency: Classes use enums and utility classes
Factory Creation: StateMachineCreator creates StateTemplate instances.
Detail Description

### Dynamic View
Sequence Diagram of  PowerStateMachine Initialization
Figure 25. Sequence Diagram of PowerMode Transition
Initialization Sequence:
Service Layer Startup - PowerManagerService → PowerExtService starts
PowerExtService will init PowerStateMachine on it’s start process
PowerStateMachine after that initiates states through Factory pattern – StateMachineCreator
StateMachineCreator use corresponding transition policies and StateTemplate to creates states
Power Mode Event Status Initialization - BitMask setup with default values
After the initialization completes, PowerExtService requests to entry the Normal state
The system now is in Normal state
Sequence Diagram of  processing power mode events
Figure 26. Sequence Diagram of processing Power Mode Events
Sequence flow:
Event Trigger: PowerExtService sends an event update to the PowerStateMachine with an event mask and activation status
Event Processing: The PowerStateMachine updates its internal bitmask state and then calls to the current state to check for state transitions.
Transition Logic: The current state iterates through predefined transition policies, using bitwise AND operations to match current state against condition patterns
Decision Making: First matching condition wins and determines the next state transition
Sequence Diagram of  power mode transition
Figure 27. Sequence Diagram of  Power Mode Transition
Common power mode transition sequences:
When the status of power-mode-events change, PowerStateMachine will process power mode events and then request the current state to check state transition conditions.
The current state iterators all the policies by order and check state transition conditions in policies against to the current status of power-mode-events.
In case all the conditions are satisfied, the current state will respond to PowerStateMachine the next transition state.
PowerStateMachine then change state to the next state
On state change, the new state will call back to PowerExtService to notify state change
PowerExtService then requests MCU to stop
Upon receiving ACK from MCU, PowerExtService calls to PowerManagerService to change the system power mode.
PowerManagerService changes the system power mode and notifies to other services/apps
After that, PowerManagerService calls to PowerControlDeivce to suspend CP, AP subsequently.

## Proposal 2: Design New Component Based-Data Structure Approach

### Power Manager Component Diagram
On the High Level Design, the proposal is about creating a new component that is dedicated for processing Power Mode Event and checking state transitions.
Figure 21. PowerManager static view
PowerModeEventHandler serves as a specialized component used by PowerExtService for event-driven state transition logic.
PowerExtService uses PowerModeEventHandler as a member.
PowerExtService delegates power mode event processing and state transition checking to PowerModeEventHandler.
PowerExtService coordinates between different power management interfaces and the core event handling logic.

### Class diagram of  Power Variant
Figure 22. Class Diagram of PowerModeEventHandler
The class diagram of PowerVariant part in this Propsal shows the relationship between the new class with others.
More detailed about the structure of PowerModeEventHandler will be in the next page ..

### Class diagram of  PowerModeEventHandler
Figure 23. Detailed Class Diagram of PowerModeEventHandler

### Dynamic View
Sequence Diagram of  PowerModeEventHandler on processing power mode events
Event Update Logic:
Receive Power mode event status from PowerExtInterface that is sent from other services or PowerManagerService.
Update the current status of power mode event to table: CurrentEventStatus
Transition evaluation logic:
Iterates through all policies in TransitionStrategy
Each policy have the next state and a set of power mode event + expected status
Compare the expected status of power mode event in the policy with the current status in the table CurrentEvent status.
If all the expected status of power mode events is met with current power mode event stutus, return the next state of the policy and stop further checking
Sequence Diagram of  Power Mode Transition
Transition sequence:
After validating the next state transition, PowerModeEventHandler will return the next state to PowerExtService
PowerExtService passses the next state to PowerStateMachine. If the next state is a valid state transiton with the current state, PowerStateMachine will transition to the next state and notifies back to PowerExtSerivive
PowerExtService will request MCU for state transition to the next power mode accordingly
Upon receiving the acknowledgement from MCU, PowerExtSerice calls to PowerManagerService to change Power Mode. PowerManagerService change power mode and control sub-systems.

# Comparison between the alternatives
Comparison between 2 alternatives based QA attributes
The final decision and supporting rationale are detailed in the subsequent chapter.

# Architectural decision and rationale

## Architectural decision
Both solutions can effectively address the current PowerManager challenges in TCUA as well as other projects that require complex Power Mode management, such as VCM and similar telematics systems. However, the architectural approach selection will be evaluated against quality attributes that meet project expectations, ensure functional requirements compliance, and provide reusability for future implementations.
Final architecture Decision:
Based on TCUA's power mode quality attribute expectations, QA-01 (performance) serves as the primary selection criterion for architectural evaluation.
The comparative analysis above indicates that the Bit-Mask-Driven Power State Machine architecture delivers superior performance and faster execution, making it the selected approach.
Bit-Mask-Driven Power State Machine

## Rationale for Selecting Bit-Mask Power State Machine
Rationale for Selecting BitMask Solution
High Performance:
O(1) constant time complexity of processing power mode events contributes to the Sleep Transition KPI assurance.
Cache efficient with single integer storage, reducing memory access overhead.
Contrasts with strategy pattern which uses Data Structure with O(n×m) time complexity that degrades with scale.
Memory Efficiency:
Single integer storage (4-8 bytes) vs. Data Structure's linear memory growth in Strategy design approach
Memory usage doesn't grow with additional events
Scalability Within PowerManger Context
Adding new events does not affect the state transition logic.
New states, conditions added only require configuration.
Specific advantages
Proven bit manipulation patterns are well-established in automotive embedded systems
Reusable design can be applied to VCM and other similar automotive projects
Lower implementation effort for similar future projects
This design is configuration-driven so easy to reuse for upcoming projects.
Significantly reduce redundant codes by factory pattern.
Risk Mitigation
• Provides deterministic power state transition behavior through priority-ordered policies, ensuring critical applications (emergency calls, stolent-vehicle-tracking, backup battery operations) maintain predictable power management responses..
Cost-Benefit Analysis: while Bit-Mask requires bit manipulation expertise, this one-time learning investment provides:
Long-term performance benefits.
Consistent behavior across different load conditions.
Lower maintenance overhead due to simpler state representation.
For TCUA's power management requirements, the Bit-Mask-Driven State Machine offers higher performance, memory efficiency, and deterministic behavior that aligns perfectly with performance KIP.
This architectural while the performance and memory advantages provide significant long-term benefits for the project and future automotive applications.
Specially, this architecture can be easily use for upcoming project with minor change (only configuration).
The Strategy Design with Data Structure solution's flexibility comes at the cost of unpredictable performance and higher resource consumption - trade-offs.

# How to verify final architecture design

## Implementation
BitMask design implementation
State transition Policies
These Policies are defined based on OEM Power Mode requirement. Policies specify the next state transition and conditions to transition.
State Class implementation
State Machine
This state machine is to process power mode events, manage states and state transitions
Factory Pattern for creating states
This is the mechanism to create state in the run-time with the pre-defined configuration. Through this factory pattern, we can reduce duplication codes and effort to implement state pattern.
State Machine creator uses pre-defined policies and state template to create states.
Core logic for processing power mode events and checking state transition conditions
Transition to the next state when state transitions are satisfied
Implement power mode requirements
Example: only configuration is required to implement power mode requirements.

## Design Verification
State Transition verification for QA-01 (Sleep transition performance):
1) State machine creator on initialization
2) Verify processing power mode events logic
Turn on bit of Operation mode: DEACTIVATED when receiving operation mode as DEACTIVATED
Turn on bit of CAN-bus-sleep bit mask when CAN bus becomes sleep
Turn on bit of bit mask when keep-Alive status is inactive
3) Verify checking state transition conditions against to the policy
When can bus changes to sleep, current power-mode-events status on bit-Set: 0x15 = bit 0, 2, 4
It matched with the state transition to Sleep Mode in policy
4) Transition to Sleep state
5) Request MCU to transition to Sleep and receives ACK from MCU
6) PowerExtService requests PowerManagerService (Power Core part) to change Power Mode
7) PowerManagerService changes TCUA power Mode and notifies to other services/applications
8) PowerMangerService then calls tp PowerCotrplExtVariant to control NAD (power off)
Trigger point: timestamp: 78.2973
End point: Call to Linux kernel to shutdown at timestamp: 78.5507
Average Transition time: ~337 ms tested on TCUA on Debug image
Result of verification for Functional requirements: [PASS]

# Conclusion

## Conclusion
In summary, the design of new component for commonazation of Power Mode Event handling delivers:
High-performance event handling and state transition checking for complex JLR power mode requirements.
Enhanced system stability through isolated PowerStateMachine design.
Simplified maintenance - new transitions require only policies.
Reusable patterns applicable to other projects by re-configure BitMask and Policies
Commonization of Power Mode Management in PowerManager
Significantly reduce the code complexity and redundancy
Configuration-driven implementation that allows OEM-specific customizations without core architecture modifications.
AS-IS
TO-BE

## Area for Improvement

## Future Plan
TCUA Project Integration: Implement the BitMask-Driven Powet State Machine design in the TCUA, project and thoughly validate performance improvements, stability and maintainability in real-system applicability. If the result is good, apply to TCUA/VCM.

**Table:**
Document Title | Commonization Design of Power Mode Management In PowerManager
Issuing Authority | Name: Thuong Nguyen
Configuration ID | T.B.D
Status of Document | In Progress / Approved / Released

**Table:**
Document Version | Date | Content of Change | Author | Reviewer | Approver
0.1 | 01-Aug-2025 | Initial Release | Thuong Nguyen | Seungchul Yi
Luan Pham | Seungchul Yi
0.2 | 07-Aug-2025 | Update the doc | Thuong Nguyen | Seungchul Yi
Luan Pham | Seungchul Yi
0.3 | 24-Aug-2025 | Update the doc and refine the solution | Thuong Nguyen | Seungchul Yi
Luan Pham | Seungchul Yi
0.4 | 07-Sep-2025 | Update the first review comments | Thuong Nguyen | Seungchul Yi
Luan Pham | Seungchul Yi
0.5 | 12-Sep-2025 | Refine document, | Thuong Nguyen | Seungchul Yi
Luan Pham | Seungchul Yi
1.0 | 22-Sep-2025 | Modify points that was reviewed by Mr Seungchul and finalize document | Thuong Nguyen | Seungchul Yi
Luan Pham | Seungchul Yi

**Table:**
Abbreviation | Description
TCUA | Telematics Control Unit Stand Alone
VCDP | JLR Back-end Server
VCM | Vehicle Connectivity module
NAD | Network Access Device
AP | Application Processor
CP | Connectivity Processor
VP | Vehicle Processor
BCMA | Body Control Module-A
MCU (VP) | Microcontroller Unit
eCall | Emergency Call
bCall | Breakdown Call
SVT | Stolen Vehicle Tracking

**Table:**
Functional Requirement ID | Description
FR-01 | While the TCUA is in LISTEN power state,
If the TCUA receives NAD WAKEUP OR IMU event,
Then the TCUA shall transition to NORMAL Power State,
FR-02 | While the TCUA is in NORMAL power state and ALL the following conditions are met, then the TCUA shall transition to SLEEP power state,
CAN SLEEP
KeepAlive is INACTIVE
Operation Mode is DEACTIVATED
FR-03 | While the TCUA is in NORMAL power state and ALL the following conditions are met, then the TCUA shall transition to LISTEN power state,
CAN SLEEP
KeepAlive is INACTIVE
Operation Mode is FEATURE ROVISIONED or ECALL-ONLY
FR-04 | While the TCUA is in NORMAL power state & PWMReceive Timer already started and ALL the following conditions are met, then the TCUA shall transition to POLLING power state,
CAN SLEEP
KeepAlive is INACTIVE
PWMReceive Timer expired
PwmInitialReceiveTimer expired
Operation Mode is FEATURE PROVISIONED or ECALL-ONLY
FR-05 | When the PwmFirstPeriodicCycle timer expired, then the TCUA shall check the current Power State,
If the TCUA's current power state is NORMAL state,
Then the TCUA shall not trigger power state transition until NORMAL mode exit condition are met
Else the TCUA shall transition to POLLING MODE EXIT power state immediately
FR-06 | TCUA shall move to Sleep :-
IF no Emergency Calls are ongoing and
No Callback timers are ongoing and
No SVT Live Tracking and
No CAN Activity
FR-07 | During bCall Callback sub-state, if no other processes require TCUA to be in Normal Power Mode, TCUA shall enter Listen Mode. This requirement shall override any Listen or Polling modes requests currently in progress.
FR-08 | TCUA eCall application shall be capable of automatically picking up an incoming call during the call back period, irrespective of PowerMode value ,Listen mode, Normal mode.
i.e if the TCUA is in sleep mode and the eCall button is pressed satisfactorily, the TCUA shall transition back to Normal mode and proceed to establishing the eCall.
FR-19 | SVT application shall control ECU Power Mode in accordance with the Figure in next comments
FR-10 | TCUA shall allow transition from Backup Battery Power State to Listen Power State or Sleep Power State depending on Application requirements.
… | 

**Table:**
QA-ID | QA | Description
QA-01 | Performance | TCUA shall follow the below Key Performance Indicators(KPI's)
Safe shutdown (transition to Sleep/Shutdown): 4 sec
QA-02 | Maintainability | A document describing the stages leading to the ECU going into Sleep and Waking up shall be provided to, and reviewed with, the JLR software capability team
QA-03 | Scalability | The Power Mode design shall support the addition of new power states, transition conditions without requiring modification to existing state implementations or core state transition logic.

**Table:**
Component | Description
PowerLock | This is the interface component that is used by other services/applications to request/release PowerManger keep-Alive the system.
PowerManager | This is the interface component that is used by other services/applications to set/get Power information from power manager, eg: power source, power mode, power state, …
CallBackInterface | This is the interface component that is used to notify other services/applications whenever power mode, power state, .. changes
Loadable State Machine | This component is to manage power mode, it changes power mode of the system based defined policy. It also specifies the power mode of sub-system corresponding system power modes.
PowerManagerService | This is the core component of the PowerManagerService. It serves as the central coordinator that manages interactions between all components within the PowerManagerService.
PowerControlExt | This component is for controlling sub-system based on Power Modes
PowerExtService | This is the core component of the variant architecture. It serves as the central coordinator that manages interactions between variant components and external services, core system parts, and other subsystems to deliver OEM-specific features and functionality..
Bit-Mask-Driven PowerStateMachine | This component is to process power mode events and manage state transition of the system according to OEM state machine.
PowerControlExtVariant | This component implements sub-system control which are called by PowerControlExt in core part.
PowerModeTimer | This component is to manage Power Mode timers corresponding to Power Modes
PowerExtInterface | This component is to communicate with other services to get/receive power mode events.

**Table:**
Class | Description
PowerStateMachine | This is core class, which manage all states, processing power mode events and coordinate with states for state transition
StateMachineCreator | Factory pattern implementation
Instantiates concrete State objects
Uses StateTransitionPolicy for configuration
Creates and configures the entire state machine
IStateMachineCallBack | Notifies observers of the state change for external coordination
Istate | Abstract Interface, defines common state operations
StateTemplate | Provide a template implementation

**Table:**
Name | Type | Description
PowerModeEventHandler | Class | Main class which is for processing and checking state transiton
EventType | Enum | Include Power Mode Events
Event Status | Enum | Include status of power mode events
StateTransitionCase | Enum | Include all the state transition scenarios
StateTransitionSet | Struct | A set of state transition case and power corresponding mode events

**Table:**
Category | Bit-Mask-Driven Power State machine | Strategy Pattern
Performance |  Better performance. Average transition time: ~260 ms
 Power Mode Event Status is stored in a single integer so it is faster to update and check conditions with time complexity (O1).
 Consume less space (single integer) to store power mode events status.

Score: 5/5 | Lower performance.  Average transition time to Sleep: ~390 ms
  Time complexity: O(nxm) where n is number of transition case, m is number of power mode event conditions.
 Memory usage grows linearly, need more space to store power mode events status and transition strategy.
Score: 3/5
Maintainability |  Proven bit manipulation patterns.
 The well-structure of policies for state transitions provide a good maintainability.
 Easy to add new event conditions and state transitions.
 States are created in the runtime so code is less complex.
 Requires bit manipulation expertise.
Score: 4/5 |  Self-documenting structure
 Source code is clear and easy to understand
 Easy unit testing
 Easy to add new event  conditions and state transitions.
 A small component so less effort to implement and maintain.

Score: 5/5
Scalability |  Easy to scale without changing code logic
 Provide high scalability though policies configuration
 Only configuration is needed when state transition conditions changes or added.


Score: 4/5 |  Easy to scale without changing code logic
 Provide high scalability though configuration only.
 Unlimited event capacity
 Complex relationships possible
 Multi-state event support

Score: 5/5

**Table:**
AS-IS | TO-BE
Code structure | Code structure

**Table:**
AS-IS | TO-BE
Sequential checking like this | Configuration driven

**Table:**
Requirement | Implementation
While the TCUA is in NORMAL power state and ALL the following conditions are met, then the TCUA shall transition to SLEEP power state.
+ CAN SLEEP
+ KeepAlive is INACTIVE
+ Operation Mode is DEACTIVATED | 
While the TCUA is in NORMAL power state & PWMReceive Timer already started and ALL the following conditions are met, then the TCUA shall transition to POLLING power state,
+ CAN SLEEP
+ KeepAlive is INACTIVE
+ PWMReceive Timer expired
+ PwmInitialReceiveTimer expired
+ Operation Mode is FEATURE PROVISIONED | 
