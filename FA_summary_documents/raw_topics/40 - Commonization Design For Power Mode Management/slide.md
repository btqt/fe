# Slide Content

**Source:** FA_Thuong4_nguyen_Commonization_Design_For_Power_Mode_Management_v1.0_Final.pptx


## Slide 1

Function Architect Certification Review

Author: Nguyen Xuan Thuong Mentor: Seungchul Yi

Commonization Design For Power Mode Management



## Slide 2

TABLE CONTENT

4

Architecture Design Implementation

5

Background

5

6

2

Q&A

6

1

Functional Requirements and Quality Attributes

Architecture Design Proposals

3

Problem Indentification



## Slide 3

1. Background

PowerManager Static View

TCUA has 3 main sub-systems: + AP (application processor) + CP (connectivity processor) + MCU PowerManager on AP is the master coordinating with sub-systems to control the power mode of TCUA PowerManage process power-mode-events and check state transition conditions based on OEM power mode requirements



## Slide 4

Power State Machine

1. Background

Power state machine specifies states and conditions to transition between states PowerManager controls the power mode of TCUA based on this state machine requirements. State transitions to other states have different priority



## Slide 5

Power Mode Events

1. Background

4 Power Modes 18 power mode events 40 State transition scenarios A transition condition is a set of power mode events status Overlap power mode events in state transition conditions



## Slide 6

Current challenges

2. Problem Indentification

No design to evaluate complex power mode transition conditions

No specific design to evaluate complex state transition conditions Sequential checking approach leads to code duplicated, complex and hard to maintain Priority of state transitions is hard-code by if-else statement New state transitions requires much effort to implement and prone to side effects. Limit of scalability and reusability

Single condition checking



## Slide 7

Functional requirements

3. Functional Requirements and Quality Attributes



## Slide 8

Quality Attributes

3. Functional Requirements and Quality Attributes



## Slide 9

4.1 Proposal 1: Bit-Mask-Driven power state machine

4. Design Proposals

Create a new state machine design to manage power mode of the system with objectives: Functionality: Provide a common design for power mode management via power state machine that can hande multiple events for state transions. Scalability: Easy to add new state transition scenarios or state transition conditions Reusability: The design can be applied across different projects Maintainability: Clear separation of concerns and configuration-driven approach

Overall approach



## Slide 10

4.1 Proposal 1: Bit-Mask-Driven power state machine

4. Design Proposals

StateListen

CurenPowerModeEventStatus

StateNormal

0

0

0

0

0

0

0

TransitionPolicy

0

0

0

0

0

0

0

0

1

0

- Next state: Listen - Conditions: MaskEvent1: [Bit 0] MaskEvent2: [Bit 1] MaskEvent3: [Bit 4]

Check bitmask at bit 0 : 

BitMask to transition satisfied Transition to Listen

Check state transition conditions

Create a new state machine design which manages state transitions though policies. Each state has polices that specify the next state transition and conditions of power mode events presenting by bitMask. Use bit-mask algorithm to process power mode events and check for state transitions. Configuration-driven though transition polices to implement power mode requirements.

Detailed approach

1

1

Check bitmask at bit 1 : 

Check bitmask at bit 4 : 

CondtionMask to Listen

0

0

0

0

0

1

1

1

1

1



## Slide 11

4.1 Proposal 1: Bit-Mask-Driven power state machine

Component Diagram

4. Design Proposals



## Slide 12

4.1 Proposal 1: Bit-Mask-Driven power state machine

4. Design Proposals

Factory pattern for creating states

Factor pattern is to create states, reduce implementation effort and code duplications. StateMachineCreator uses policies, StateTemplate and Observer to creates states and the initialization process. PowerStateMachine will manage these states and transitions between them



## Slide 13

4.1 Proposal 1: Bit-Mask-Driven power state machine

4. Design Proposals

Flow of checking state transition

Update power-mode-event status to the Bit-Set Call to the current state to check next state transition Current state checks current power-mode-event status against to their policies in priority order PowerStateMachine will transition to the next state if state changes.



## Slide 14

4. Design Proposals

4.1 Proposal 1

Sequence diagram

Sequence: Process power mode event and check state transitions



## Slide 15

4.2 Proposal 2: Strategy pattern for checking state transition conditions

4. Design Proposals

Overall approach

Create a dedicated component for checking state transition conditions with objectives: Functionality: Provide a common design for checking state transitions through configuration-driven in Power Management Scalability: Easy to add new state transition scenarios or state transition conditions Reusability: The design can be applied across different projects Maintainability: Clear separation of concerns and configuration-driven approach



## Slide 16

4.2 Proposal 2: Strategy pattern for checking state transition conditions

4. Design Proposals

Detailed Concept

Define a transition strategy based on the OEM state machine and requirements Strategy specifies the next state transition and conditions of power-mode-event status Use a table to store the current status of power mode events. When the status of power mode events change, update to the table and then check the strategy against to the current event status on the table. Return the next state to OEM state machine



## Slide 17

4.2 Proposal 2: Strategy pattern for checking state transition conditions

4. Design Proposals

Class Diagram



## Slide 18

4.2 Proposal 2

4. Design Proposals

Sequence: Process power mode event and check state transitions

Sequence diagram

Update power events status Check expected event status on Policy against to the current Event status Return the next state if conditions in policy matched the current status of events



## Slide 19

4.3 Comparision between proposals

4. Design Proposals

Score: 3/3

Score: 0/3

Score: 5/5

Score: 4/5

Score: 4/4

Score: 3/4

Based on the QA-01 (Performance), Select the proposal 1: Bit-Mask-Driven Power State machine



## Slide 20

5. Architecture Design Implementation



## Slide 21

5. Architecture Design Implementation



## Slide 22

5. Architecture Design Implementation

Implementation

BitMask declaration

Policy imlementation



## Slide 23

5. Architecture Design Implementation

Implementation Factory pattern and state-transition-evaluation logic

Factory pattern to create states

Core logic to check and perform state state transition



## Slide 24

Average transition time Current design: 413 ms New design : 337 ms => reduce 18 % transition time

5. Architecture Design Implementation

Verify QA-01 (performance)

Current Design Test Result

New Design Test Result



## Slide 25

5. Architecture Design Implementation

Verify QA-01 (performance)

Verification logs of new design

334,6 ms



## Slide 26

5. Architecture Design Implementation

Verify Functional requirements, result [PASS]



## Slide 27

6. Q & A

Thank you for listening



## Slide 28

Bit-Mask-Driven power state machine

APENDIX

Use a bitSet (single integer) to store the current state of all power mode events. When power mode events change state, the corresponding bits are updated accordingly: Set bit to 1 when the power mode event becomes ACTIVE Clear bit to 0 when the power mode event becomes INACTIVE

Bit-Mask Pattern for processing power mode events



## Slide 29

Checking state transition algorithm

APENDIX



## Slide 30

APPENDIX

Final Architecture Design

Dynamic view

Initialization process Init PowerStateMachine Init StateMachineCreator Create states Init default power mode event status



## Slide 31

State transition sequence

APPENDIX

Final Architecture Design

Dynamic view



## Slide 32

APPENDIX

Power state machine of VCM JLR



## Slide 33

APPENDIX

Power state machine of GEN12 project



## Slide 34

APPENDIX

Power state machine of ICON

