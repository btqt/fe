# Design of Secure Remote Diag on Container Separation Architecture

**Candidate:** Vu Hoang Phi (phi.vu)
**LGEDV – September 2026**

---

## Content

1. Context & Motivation
2. Problem
3. Quality Attribute
4. Design Alternative
5. Comparison & Decision
6. Implementation & Verification

---

## 1. Context & Motivation – Toyota 24DCM project

- **Toyota DCM:** a telematics platform providing connected vehicle services, including remote diagnostics from OEM server.
- Supports diagnostic communication and OTA updates through the Internal CAN bus.
- **Current architecture:** monolithic deployment model, with all components running in a single partition.

**System architecture migration:**

- Due to security, OEM wants to separate DCM into 2 partitions using LXC containers:
  - **EP container (Entry Point):** externally exposed network components.
  - **Internal container:** components which have access to CAN bus.

### Current system architecture (monolithic)

```plantuml
@startuml
skinparam componentStyle uml2
skinparam rectangle {
  BackgroundColor<<lge>> #CDECF7
  BackgroundColor<<oem>> #90EE90
  BackgroundColor<<ext>> #FFA500
}

cloud "OEM Server" as OEM #FFA500

rectangle "DCM\n<<Application Processor>> <<OTA slave>>" {
  rectangle "<<application>>" {
    rectangle "Various OEM Apps" <<oem>>
    rectangle "Regional Apps" <<lge>>
    rectangle "RemoteDiag" <<lge>>
  }
  rectangle "<<service>>" {
    rectangle "LGE Platform services" <<lge>>
    rectangle "MqttMgr\nHttpMgr" <<lge>>
    rectangle "OnBoardClient" <<lge>>
  }
}

rectangle "MM\n<<ECU>> <<OTA Master>>" as MM #FFA500
rectangle "Internal CAN bus" as CAN #FFA500

OEM -[#red]-> "MqttMgr\nHttpMgr" : diag (OEM flow)
"MqttMgr\nHttpMgr" -[#red]-> RemoteDiag
RemoteDiag <-[#magenta]-> MM : OTA diag (MM flow)
RemoteDiag --> OnBoardClient
OnBoardClient <--> CAN
@enduml
```

### New system architecture (containerized)

```plantuml
@startuml
skinparam componentStyle uml2
skinparam rectangle {
  BackgroundColor<<lge>> #CDECF7
  BackgroundColor<<oem>> #90EE90
  BackgroundColor<<ext>> #FFA500
}

cloud "OEM Server" as OEM #FFA500

rectangle "DCM\n<<Application Processor>> <<OTA slave>>" {
  rectangle "EP Container\n<<LXC container>>" {
    rectangle "<<application>>" {
      rectangle "Various OEM Apps" <<oem>>
      rectangle "LGE Regional Apps" <<lge>>
      rectangle "RemoteDiag" <<lge>>
    }
    rectangle "<<service>>" {
      rectangle "LGE Platform services" <<lge>>
      rectangle "MqttMgr\nHttpMgr" <<lge>>
    }
  }
  rectangle "IP Container\n<<LXC container>>" {
    rectangle "OnBoardClient" <<lge>>
  }
}

rectangle "MM\n<<ECU>> <<OTA Master>>" as MM #FFA500
rectangle "Internal CAN bus" as CAN #FFA500

OEM -[#red]-> "MqttMgr\nHttpMgr"
"MqttMgr\nHttpMgr" -[#red]-> RemoteDiag
RemoteDiag <-[#magenta]-> MM
RemoteDiag --> OnBoardClient
OnBoardClient <--> CAN
@enduml
```

> **Legend:** System/subsystem boundary; Multiple Applications/Services (stacked box); LXC Container (blue border); Scope of this FE project (red dashed).
> Arrows — black: have communication; red: OEM server ↔ Internal CAN bus flow; magenta: MM ecu ↔ Internal CAN bus flow.
> Color — Green: OEM scope; Light blue: LGE scope; Orange: External; Gray: 3rd party.

---

## 2. Problem

- **RemoteDiag:** a module that processes diagnostics for each ECU and uploads diagnostic data to the OEM Server.
- RemoteDiag has 2 communication paths:
  - **with OEM Server:** performs direct diagnostic commands.
  - **with MM ecu:** OTA diagnostic update.
- **Risk:** the RemoteDiag ↔ MM path routes through the EP, which is insecure.
- OEM requests that unauthorized reprogramming requests should be filtered.

> RemoteDiag's internal architecture and communication should be redesigned to align with the new architecture.

```plantuml
@startuml
skinparam componentStyle uml2
skinparam rectangle {
  BackgroundColor<<lge>> #CDECF7
  BackgroundColor<<ext>> #FFA500
}

cloud "OEM Server" as OEM #FFA500

rectangle "EP Container\n<<LXC container>>" {
  rectangle "RemoteDiag" <<lge>> {
    rectangle "MgrAdapters" as ADP
    rectangle "RemoteDirectCommand" as RDC
    rectangle "OTA" as OTA
    rectangle "PriorityControl" as PRIO
  }
  rectangle "MqttMgr\nHttpMgr" as MQTT <<lge>>
}

rectangle "IP Container\n<<LXC container>>" {
  rectangle "OnBoardClient" as OBC <<lge>>
}

rectangle "DiagMgr / CalibMgr / PowerMgr / PPIMgr\nLocationMgr / RegionMgr / CommMgr / SomeIpMgr" as MGRS <<lge>>
rectangle "MM\n<<ECU>> <<OTA Master>>" as MM #FFA500
rectangle "Internal CAN bus" as CAN #FFA500

MGRS -- ADP : binder
ADP <-> RDC : event
RDC <-[#red]-> MQTT : diag commands
OEM -[#red]-> MQTT
RDC -[#red]-> PRIO : diag command
PRIO -> OBC : diag request
OTA -[#magenta]-> PRIO : OTA request
MM -[#magenta]-> OTA : <<TCP>>
OBC --> CAN : CAN data

note bottom of PRIO
  **Risk:** Insecure channel if EP Container is compromised.
  Must filter unauthorized reprogramming requests across the EP boundary.
end note
@enduml
```

### Functional requirements

| #     | Description                                                                                      |
| ----- | ------------------------------------------------------------------------------------------------ |
| FR-01 | The system shall block unauthorized diagnostic commands related to ECU reprogramming.            |
| FR-02 | The system shall route UDS diagnostic messages exchanged with the MM ECU entirely within the IP. |

---

## 3. Quality Attribute

| ID    | QA Scenario                                                                   | Measure                                                                                                                                                        | Type            | Priority |
| ----- | ----------------------------------------------------------------------------- | -------------------------------------------------------------------------------------------------------------------------------------------------------------- | --------------- | -------- |
| QA-01 | A compromised app/service sends reprogramming diagnostic to CAN               | Zero Reprogramming SIDs reaches the internal CAN.                                                                                                              | Security        | High     |
| QA-02 | MM ECU sends a diagnostic request via TCP to RemoteDiag                       | Zero diagnostic communication is found in EP. 100% of requests are handled within IP.                                                                          | Security        | High     |
| QA-03 | The new design preserves the original core responsibility of each component.  | No component is required to take responsibilities outside its originally defined role. No additional specification updates caused by responsibility expansion. | Modifiability   | High     |
| QA-04 | Existing software components (other than RemoteDiag) require minimal changes. | Number of components which need to be modified to align with the new design.                                                                                   | Maintainability | Medium   |
| QA-05 | Failure in OEM Center flow or MM ECU flow                                     | The other flow remains operational.                                                                                                                            | Availability    | Low      |

---

## 4. Design Alternative

### Alt-A: Split RemoteDiag

- Split RemoteDiag responsibilities into 2 apps:
  - **RemoteDiag (EP):** handles diag messages from OEM server.
  - **RemoteOTA (IP):** handles OTA diag message flows from MM ECU.
- **OnBoardClient** is responsible for reprogramming message filtering and diagnostic priority control.

```plantuml
@startuml
skinparam componentStyle uml2
skinparam rectangle {
  BackgroundColor<<lge>> #CDECF7
  BackgroundColor<<new>> #FFFFCC
  BackgroundColor<<ext>> #FFA500
}

cloud "OEM Server" as OEM #FFA500

rectangle "DiagMgr / CalibMgr / PowerMgr / PPIMgr\nLocationMgr / RegionMgr / CommMgr / SomeIpMgr" as MGRS <<lge>>

rectangle "EP Container\n<<LXC container>>" {
  rectangle "RemoteDiag" <<lge>> {
    rectangle "MgrAdapters" as ADP
    rectangle "RemoteDirectCommand" as RDC
    rectangle "UnixSocketClient" as USC <<new>>
  }
  rectangle "MqttMgr\nHttpMgr" as MQTT <<lge>>
}

rectangle "IP Container\n<<LXC container>>" {
  rectangle "RemoteOTA" <<lge>> {
    rectangle "OTA" as OTA
  }
  rectangle "OnBoardClient" <<lge>> {
    rectangle "UnixSocketServer" as USS <<new>>
    rectangle "OBCMgrService" {
      rectangle "SID Filter" as SID <<new>>
    }
    rectangle "PriorityControl" as PRIO <<new>>
    rectangle "CANHandler" as CANH
  }
}

rectangle "MM\n<<ECU>> <<OTA Master>>" as MM #FFA500
rectangle "Internal CAN bus" as CAN #FFA500

MGRS -- ADP : binder
ADP <-> RDC : event
RDC -[#red]-> MQTT : diag command
OEM -[#red]-> MQTT
RDC -[#red]-> USC : diag command
USC -[#red]-> USS : UDS socket
USS -[#red]-> SID : diag command / filter
SID -[#red]-> PRIO : filtered diag
OTA -[#magenta]-> PRIO : OTA diag
PRIO -> CANH : filtered diag/OTA
MM -[#magenta]-> OTA : <<TCP>>
CANH --> CAN : CAN data
@enduml
```

### Alt-B: Move RemoteDiag

- Move RemoteDiag into the IP container.
- To maintain communication with the OEM server and other EP-side components, establish UDS socket connections to each module.

> **Trade-off — Maintainability:** All EP modules require cross-container IPC support, increasing implementation effort and code-change scope.

```plantuml
@startuml
skinparam componentStyle uml2
skinparam rectangle {
  BackgroundColor<<lge>> #CDECF7
  BackgroundColor<<new>> #FFFFCC
  BackgroundColor<<ext>> #FFA500
}

cloud "OEM Server" as OEM #FFA500

rectangle "EP Container\n<<LXC container>>" {
  rectangle "DiagMgr / CalibMgr / PowerMgr / PPIMgr\nLocationMgr / RegionMgr / CommMgr / SomeIpMgr" as MGRS <<lge>>
  rectangle "MqttMgr\nHttpMgr" as MQTT <<lge>>
}

rectangle "IP Container\n<<LXC container>>" {
  rectangle "RemoteDiag" <<lge>> {
    rectangle "UnixSocketClient" as USC <<new>>
    rectangle "RemoteDirectCommand" as RDC {
      rectangle "SID Filter" as SID <<new>>
    }
    rectangle "OTA" as OTA
    rectangle "PriorityControl" as PRIO <<new>>
  }
  rectangle "OnBoardClient" as OBC <<lge>>
}

rectangle "MM\n<<ECU>> <<OTA Master>>" as MM #FFA500
rectangle "Internal CAN bus" as CAN #FFA500

MGRS -> USC : <<UDS socket>>
MQTT -[#red]-> USC : <<UDS socket>>
USC -[#red]-> SID : diag command / filter
SID -[#red]-> PRIO : filtered diag
OTA -[#magenta]-> PRIO : OTA diag
PRIO -> OBC : filtered diag/OTA
OEM -[#magenta]-> MM
MM -[#magenta]-> OTA : <<TCP>>
OBC --> CAN : CAN data
@enduml
```

### Alt-C: Move RemoteDiag + Proxy

- Move RemoteDiag into the IP container.
- Add a new **RemoteDiagProxy (RDP)** component in the EP that acts as a transparent proxy.
  - EP modules continue to interact with RDP using their existing interface.
  - RDP forwards requests to RemoteDiag via a UDS socket channel.

```plantuml
@startuml
skinparam componentStyle uml2
skinparam rectangle {
  BackgroundColor<<lge>> #CDECF7
  BackgroundColor<<new>> #FFFFCC
  BackgroundColor<<ext>> #FFA500
}

cloud "OEM Server" as OEM #FFA500

rectangle "DiagMgr / CalibMgr / PowerMgr / PPIMgr\nLocationMgr / RegionMgr / CommMgr / SomeIpMgr" as MGRS <<lge>>

rectangle "EP Container\n<<LXC container>>" {
  rectangle "RemoteDiagProxy" <<lge>> {
    rectangle "Service Adapters" as ADP
    rectangle "RemoteDiagProxy" as RDP <<new>>
    rectangle "UnixSocketClient" as USC <<new>>
  }
  rectangle "MqttMgr\nHttpMgr" as MQTT <<lge>>
}

rectangle "IP Container\n<<LXC container>>" {
  rectangle "RemoteDiag" <<lge>> {
    rectangle "UnixSocketServer" as USS <<new>>
    rectangle "RemoteDiag(core)" as RDGC {
      rectangle "SID Filter" as SID <<new>>
    }
    rectangle "RemoteOTA" as OTA
    rectangle "PriorityControl" as PRIO <<new>>
  }
  rectangle "OnBoardClient" as OBC <<lge>>
}

rectangle "MM\n<<ECU>> <<OTA Master>>" as MM #FFA500
rectangle "Internal CAN bus" as CAN #FFA500

MGRS -- ADP : binder
ADP <-> RDP
RDP -[#red]-> MQTT : Diag command
RDP -[#red]-> USC : Diag command
OEM -[#red]-> MQTT
USC -[#red]-> USS : UDS socket
USS -[#red]-> SID : Diag command / filter
SID -[#red]-> PRIO : Filtered diag
OTA -[#magenta]-> PRIO : OTA diagnostic
PRIO -> OBC : Filtered diag/OTA
MM -[#magenta]-> OTA : <<TCP>>
OBC --> CAN : CAN data
@enduml
```

---

## 5. Comparison & Decision

|                           | Alt-A – Split RemoteDiag                                                                                                                                   | Alt-B – Move RemoteDiag                                                                                                           | Alt-C – Move RemoteDiag + Proxy                                                                                       |
| ------------------------- | ---------------------------------------------------------------------------------------------------------------------------------------------------------- | --------------------------------------------------------------------------------------------------------------------------------- | --------------------------------------------------------------------------------------------------------------------- |
| **QA-01 Security**        | **High** — Unauthorized diagnostic commands are filtered.                                                                                                  | **High** — Unauthorized diagnostic commands are filtered.                                                                         | **High** — Unauthorized diagnostic commands are filtered.                                                             |
| **QA-02 Security**        | **High** — MM ECU diagnostic traffic remains within the EP container. (CyberSecurity team confirmed)                                                       | **High** — MM ECU diagnostic traffic remains within the EP container. (CyberSecurity team confirmed)                              | **High** — MM ECU diagnostic traffic remains within the EP container. (CyberSecurity team confirmed)                  |
| **QA-03 Modifiability**   | **Low** — RemoteDiag and OnBoardClient's responsibility is changed (diagnostic priority control is moved to OnBoardClient), requiring requirement updates. | **High** — Core responsibilities of existing components remain unchanged after migration.                                         | **High** — Core responsibilities of existing components remain unchanged after migration.                             |
| **QA-04 Maintainability** | **Medium** — 01 additional component is modified (OnBoardClient — implements UDS socket server and priority control).                                      | **Low** — 10 EP modules (PowerMgr, HttpMgr, …) have to implement cross-container UDS socket communication and fail-safe handling. | **High** — No changes required in existing modules. RemoteDiagProxy preserves the current binder interface.           |
| **QA-05 Availability**    | **High** — OEM center and MM ECU communication flows run in separate processes. Failure of one flow does not impact the other.                             | **Low** — OEM center and MM ECU communication flows share the same process. Failure in one flow may impact the other.             | **Low** — OEM center and MM ECU communication flows share the same process. Failure in one flow may impact the other. |
| **Decision**              | Rejected                                                                                                                                                   | Rejected                                                                                                                          | **Selected**                                                                                                          |

> **Rating:** High = Fully satisfy · Medium = Partially satisfy · Low = Not satisfy
> **Final decision: Alt-C**

---

## Detail design: Forward API call from proxy to remotediag

```plantuml
@startuml
title __#13-02 IPC callback transmission (proxy to remotediag)__
!pragma teoz true

autonumber

box EP #lightYellow
    participant "<b>XXXMgr" as XXX #Application
    participant "<b>RemotediagProxy" as PROXY #Application
    participant "<b>ProxyIpcClient" as CLI #Application
    participant "<b>UnixSocketClient" as USC #Application
endbox

box IP #lightBlue
    participant "<b>UnixSocketServer" as USS #Application
    participant "<b>ProxyIpcServer" as SRV #Application
    participant "<b>ServerCallback\n<b>Dispatcher" as SRV_DISP #Application
    participant "<b>Callback\n<b>Handler(Adapter)" as SRV_CB_HDL #Application
    participant "<b>RemoteDiag" as RDG #Application
endbox

== Callback trigger points from XXXMgr ==
XXX->PROXY: onEventChanged(eventData)
XXX->PROXY: onStateChanged(stateData)
XXX->PROXY: onNotification(notifyData)

== Callback publish from proxy ==
PROXY->CLI: sendCallback(callbackId, payload)
CLI->CLI: enqueue mPendingCallbacks
CLI->CLI: flushCallbacks()
CLI->USC: sendFrame(Callback, callbackId, corr=0, payload)

== Transport delivery ==
USC->USS: Frame{type=Callback, id, corr=0, payload}
USS->SRV: onFrameReceived(frame)

== Callback dispatch on remotediag side ==
SRV->SRV: handleCallback(frame)\nenqueue mCallbackQueue
SRV->SRV_DISP: runCallbackDispatchLoop()
SRV_DISP->SRV_DISP: dispatchCallback(callbackId, payload)
SRV_DISP->SRV_CB_HDL: handle(callbackId, payload)
SRV_CB_HDL->RDG: invoke adapter callback logic

note over CLI,SRV #White
Callback is one-way (no response frame required).
end note
@enduml
```

> **Related flow — IPC API request-response (remotediag to proxy):**

```plantuml
@startuml
title __#13-03 IPC API request-response (remotediag to proxy)__
!pragma teoz true

autonumber

box IP #lightBlue
    participant "<b>RemoteDiag" as RDG #Application
    participant "<b>ProxyIpcServer" as SRV #Application
    participant "<b>UnixSocketServer" as USS #Application
endbox

box EP #lightYellow
    participant "<b>UnixSocketClient" as USC #Application
    participant "<b>ProxyIpcClient" as CLI #Application
    participant "<b>ClientCommand\n<b>Dispatcher" as CLI_DISP #Application
    participant "<b>Command\n<b>Handler(Adapter)" as CLI_CMD_HDL #Application
    participant "<b>RemotediagProxy" as PROXY #Application
endbox

== Request from remotediag to proxy ==
RDG->SRV: requestAPICall(commandId, payload, timeout)
SRV->USS: sendFrame(Request, commandId, correlationId, payload)
USS->USC: Frame{type=Request, id=commandId, corr, payload}
USC->CLI: onFrameReceived(frame)

== Command dispatch on proxy side ==
CLI->CLI: enqueue mRequestQueue
CLI->CLI: runRequestDispatchLoop()\nhandleRequest(frame)
CLI->CLI_DISP: dispatchCommand(commandId, payload)
CLI_DISP->CLI_CMD_HDL: handle(commandId, payload)
CLI_CMD_HDL->PROXY: execute adapter/service logic
PROXY-->CLI_CMD_HDL: result
CLI_CMD_HDL-->CLI_DISP: CommandResponse{success,payload}
CLI_DISP-->CLI: response

== Response back to remotediag ==
CLI->USC: sendFrame(Response, commandId, correlationId, [status|payload])
USC->USS: Frame{type=Response, id=commandId, corr, payload}
USS->SRV: onFrameReceived(frame)
SRV->SRV: handleResponse(frame)\nmatch id + corr\nnotify mResponseCv
SRV-->RDG: return success/failure + response payload

note over CLI,SRV #White
Response payload format: [status:1byte][payload:N bytes]
status=0x01 (OK), status=0x00 (ERROR)
end note
@enduml
```

---

## Detail design: Filter scenario

```plantuml
@startuml
title 01 Diagnostic filter scenario
!pragma teoz true

autonumber

box "<<IP Container>>\n  RemoteDiag" #application
    participant "UnixSocketServer" as USS
    participant "ProxyIpcServer" as PIS
    participant "MessageDispatcher" as MDR
    participant "RemotediagHandler" as RDG
    participant "CollectionCondition" as COCO
    participant "RemoteDirectCommand" as RDC
    participant "SidFilter" as SID
    participant "PriorityControl" as PRIO
end box

participant "OnBoardClient" as OBC #application

participant "Internal\nCAN bus" as CAN

USS -> PIS : onFrameReceived\n(frame)
PIS -> MDR : dispatchCallback\n(HttpGrpcResponse)
MDR -> RDG : MSG_RPC_MESSAGE_RECEIVED
RDG -> COCO : parse DirectCommand collection
RDG -> RDC : onCenterCommandForward(MSG_ID_CENTERREQUESTDIRECTCOMMAND)

' == Priority control + SID filter ==
RDC -> PRIO : requestTriggerProcess\n(DiagTrigger,DIRECT_COMMAND, prio)
PRIO -> PRIO : getHighestPriorityTask()
PRIO --> RDC : notifyTrigger(TRIGGER_START)
RDC -> RDC : sendDirectCommand()
loop for each DirectCommand in collection
    RDC -> RDC : validateDirectCommand(ECU addr/phase)
    RDC -> SID : isAllowed(SID)
    alt #pink SID is in blocked list (0x10-02, 0x11, 0x28, 0x34, 0x85)
        SID --> RDC : false
        RDC -> RDC : set SC_COMMAND_ERROR
    else #lightgreen SID is valid
        SID --> RDC : true
        RDC -> RDC : push DirectCommandTransmission
        ' == Transmit UDS to CAN ==
        RDC -> OBC : sendUdsData(connectId, udsReq)
        OBC -> CAN : UDS request (CAN/CAN-FD)
    end
end
@enduml
```

---

## 6. Implementation and Verification

### SidFilter blocks reprogramming diagnostic requests

Runtime log evidence — `SidFilter` rejects reprogramming-related SIDs before they reach the internal CAN bus:

```text
3555985  66.2025  24LM  RDG  info  [ SidFilter.cpp : isAllowed : 45 ] SidFilter: blocked reprogramming-related request SID 11
3555986  66.2026  24LM  RDG  info  [ RemoteDirectCommand.cpp : validateDirectCommand : 344 ] Ecu list size = 1
3555987  66.2026  24LM  RDG  info  [ SidFilter.cpp : isAllowed : 45 ] SidFilter: blocked reprogramming-related request SID 28
3555989  66.2026  24LM  RDG  info  [ SidFilter.cpp : isAllowed : 45 ] SidFilter: blocked reprogramming-related request SID 85
3555991  66.2027  24LM  RDG  info  [ SidFilter.cpp : isAllowed : 45 ] SidFilter: blocked reprogramming-related request SID 10
3555993  66.2027  24LM  RDG  info  [ SidFilter.cpp : isAllowed : 45 ] SidFilter: blocked reprogramming-related request SID 34
```

### RemoteDiagProxy and RemoteDiag communication

Runtime log evidence — proxy IPC request/response across the UDS socket:

```text
[ VehicleManagerAdapter.cpp : registerServiceLocked : 99 ] Registed Vehicle Manager Service with PID: 1333
[ VehicleManagerAdapter.cpp : registerServiceLocked : 101 ] Registed MsgInd_CanRx_MET1S02
...
[ ProxyIpcServer.cpp : requestAPICall : 219 ] ProxyIpcServer: request start request=VehicleGetTripCounter path=/dev/socket/remotediag/remotediag_proxy.sock timeoutMs=2000 maxRetries=10
[ ProxyIpcServer.cpp : requestAPICall : 247 ] ProxyIpcServer: sending request=VehicleGetTripCounter payloadLen=0 frameSize=8
[ ProxyIpcClient.cpp : dispatchRequest : 381 ] ProxyIpcClient dispatchRequest: command=VehicleGetTripCounter(9002) payloadLen=0
[ ProxyIpcServer.cpp : requestAPICall : 300 ] ProxyIpcServer: request complete request=VehicleGetTripCounter status=OK payloadLen=5
```

---

## Q&A

_Thank you for listening._

---

# Appendix

## Diagnostic filtering requirements

### 5. Diagnostic Filtering Requirements

#### 5.1. Diagnostic Filtering Targets — 【MFGREQ_00005】

The ECU shall target the diagnostic request message excluding the legally applicable diagnostic communication addresses for diagnostic filtering. (For diagnostic communication addresses, refer to "Related Documents [2][10]".)

_(Supplement)_ "Reserve" in service tools and remote diagnostic response messages may be set in diagnostic request messages. Therefore, refer to the latest diagnostic address, and be careful not to exclude the diagnostic request message from the filtering target.

#### 5.2. Diagnostic Filtering Implementation Details — 【MFGREQ_00006】

The ECU shall discard diagnostic request messages in Table 5-1 by diagnostic filtering.

**Table 5-1 Diagnostic requests to be filtered (Phase 5, 6)**

| SID  | Description                                           |
| ---- | ----------------------------------------------------- |
| 0x10 | DiagnosticSessionControl — Programming session (0x02) |
| 0x11 | ECUReset                                              |
| 0x28 | CommunicationControl                                  |
| 0x34 | RequestDownload                                       |
| 0x85 | ControlDTCSetting                                     |

#### 5.3. Diagnostic Filtering Deactivate Conditions — 【MFGREQ_00007】

The ECU shall deactivate the diagnostic filtering of MFGREQ_00006 if the center connection device authentication completed successfully. For center connection device authentication, refer to "Related Documents [3]".

#### 5.4. Reactivating after Diagnostic Filtering deactivation — 【MFGREQ_00016】

After the deactivation of the diagnostic filtering in 【MFGREQ_00007】, the ECU shall reactivate the diagnostic filtering before the authentication state of the center connection device authentication transitions to "Unauthenticated" state. For the authentication state of the center connection device authentication, refer to "Related Documents [3]".

_TOYOTA MOTOR CORPORATION_
