

# About This Document
## Document Information

| Issuing authority  | LG VC Smart SW Platform Team      |
| ------------------ | --------------------------------- |
| Configuration ID   | LGE_SDD_onboardclientManagerService |
| Status of document | Approved / Released |

## Revision History
| Version | Date       | Comment         | Author                             | Approver |
| ------- | ---------- | --------------- | ---------------------------------- | -------- |
// @CGA_VARIANT_START{"__GLOBAL_SCOPE__:__VERSION_HISTORY__:variant"}
| 0.9     | 2018-07-20 | Initial Release | Charles.Lee <cheoljoo.lee@lge.com> |          |
// @CGA_VARIANT___END{"__GLOBAL_SCOPE__:__VERSION_HISTORY__:variant"}


## Overview
- This document specifies the Software Detailed Design (SDD) for the onboardclient service including the static design, dynamic design.
    - OnboardClient service manager

## Scope
- This document identifies the class consisting of each component from SAD, and describes the behaviors of those classes to accomplish the requirements upon them.

## Audience
- The target audience of this document is:
    - Software architect who will evaluate the design of the software
    - Component developer who will implement the design in actual code
    - Native application developers who need to use onboardclient service
    - Cheetah project participants who want to understand the low level design of the onboardclient service
    - Test engineers who verify this component

## Related Documents
- SAD (Software Architectural Design)

## Conventions
- This document remarks “NOTE” as follows

NOTE
> To describe information that helps audience’s conveniences.

## Acronyms
| Acronym | Description                   |
| ------- | ----------------------------- |
| SAD     | Software Architectural Design |
| SDD     | Software Detailed Design      |
// @CGA_VARIANT_START{"__GLOBAL_SCOPE__:__ACRONYMS__:variant"}
| etc     | add your acronyms             |
// @CGA_VARIANT___END{"__GLOBAL_SCOPE__:__ACRONYMS__:variant"}

## Glossary (Optional)
| Glossary        | Description      |
| --------------- | ---------------- |
| onboardclient | OnboardClient service manager |
// @CGA_VARIANT_START{"__GLOBAL_SCOPE__:__GLOSSARY__:variant"}
| etc             | add your glossary             |
// @CGA_VARIANT___END{"__GLOBAL_SCOPE__:__GLOSSARY__:variant"}

# onboardclient Component
## External Design

```puml external_design_onboardclient
@startuml external_design_onboardclient
skinparam componentStyle uml2

[Application] <<application>>
[SLDD app] <<integration test>>

package "onboardclient" {
    [onboardclient] <<serviceManager>>  as main_module
}

[Application] -(0- main_module
[SLDD app] -(0- main_module


[Binder] <<library>>
[Linux] <<OS>>
main_module -(0- [Binder]
[Binder] -(0- [Linux]

@enduml
```

// @CGA_VARIANT_START{"__GLOBAL_SCOPE__:__EXTERNAL_DESIGN__:variant"}
- add your comments for external design
// @CGA_VARIANT___END{"__GLOBAL_SCOPE__:__EXTERNAL_DESIGN__:variant"}

- Dynamic Design

```puml external_dynamic_design_onboardclient
@startuml external_dynamic_design_onboardclient
    box "Application (Proxy)"
    participant App
    participant BpOnboardclientManagerService
    end box
    participant Binder
    box "onboardclient Service ( Native )"
    participant BnOnboardclientManagerService
    participant ServiceStub
    end box
  
    App -> BpOnboardclientManagerService : API(arguments)
    BpOnboardclientManagerService -> Binder : OnboardclientData >> remote()->onTransact(OP_REGISTER_API, Parcel)
    Binder -> BnOnboardclientManagerService : onTransact(uint32_t code, const Parcel& data,reply)\n OP_REGISTER_API, Parcel >> arguments
    BnOnboardclientManagerService -> ServiceStub : API(arguments)
    activate ServiceStub
    BnOnboardclientManagerService <- ServiceStub : return E_OK
    deactivate ServiceStub
    Binder <- BnOnboardclientManagerService : reply->writeInt32
    BpOnboardclientManagerService <- Binder : reply.readInt32()
    App <- BpOnboardclientManagerService : return

@enduml
```

// @CGA_VARIANT_START{"__GLOBAL_SCOPE__:__DYNAMIC1_DESIGN__:variant"}
- Step : add your comments for dynamic design
    - use markdown format
// @CGA_VARIANT___END{"__GLOBAL_SCOPE__:__DYNAMIC1_DESIGN__:variant"}

- **sendUdsData** : send UDS request

## Internal Design
- ![img](./outplantuml/CLASSStatic.png)


// @CGA_VARIANT_START{"__GLOBAL_SCOPE__:__STATIC_DESIGN__:variant"}
- add your comments for static design of internal design
    - use markdown format
// @CGA_VARIANT___END{"__GLOBAL_SCOPE__:__STATIC_DESIGN__:variant"}

- The class diagram for the onboardclient is shown below:
    - ![img](./outplantuml/CLASSGroup.png)

// @CGA_VARIANT_START{"__GLOBAL_SCOPE__:__CLASS_DESIGN__:variant"}
- add your comments for class design of internal design
    - use markdown format
// @CGA_VARIANT___END{"__GLOBAL_SCOPE__:__CLASS_DESIGN__:variant"}

- Dynmaic Design
    - this is sequence diagram how to do operations through binder.

```puml internal_dynamic_design_onboardclient
@startuml internal_dynamic_design_onboardclient
  box "Application"
  participant OnboardclientSampleApplication
  participant BnOnboardclientManagerReceiver
  participant BpOnboardclientManagerService
  end box
  participant Binder
  box "OnboardclientService"
  participant BpOnboardclientManagerReceiver
  participant ServiceStub
  participant BnOnboardclientManagerService
  participant OnboardclientInputManager
  participant OnboardclientHandler
  end box
  box "OtherService"
  end box
  == OnboardclientSampleApplication register to Onboardclient and VIF sends msg to OnboardclientSampleApplication ==
    OnboardclientSampleApplication -> BpOnboardclientManagerService : registerReceiver(id,receiver)
    BpOnboardclientManagerService -> Binder : receiver >> remote()->onTransact(OP_REGISTER_RECEIVER, Parcel)
    Binder -> BnOnboardclientManagerService : onTransact(uint32_t code, const Parcel& data,reply)\n OP_REGISTER_RECEIVER, Parcel >> receiver
    BnOnboardclientManagerService -> ServiceStub : registerReceiver(id,receiver)
    note right ServiceStub : mReceivers[id].push_back(receiver)\nlinkToDeath() : onReceiverBinderDied(delete it from mReceivers when app dies.)
    activate ServiceStub
    BnOnboardclientManagerService <- ServiceStub : return E_OK
    deactivate ServiceStub
    Binder <- BnOnboardclientManagerService : reply->writeInt32
    BpOnboardclientManagerService <- Binder : reply.readInt32()
    OnboardclientSampleApplication <- BpOnboardclientManagerService : return E_OK

    OnboardclientInputManager -> OnboardclientHandler : handleMessage(msg)

    OnboardclientHandler -> OnboardclientInputManager : transferDatabyVIF(Data)
    OnboardclientInputManager -> ServiceStub : queryReceiverByID()
    note right ServiceStub : mReceivers[id].onReceive(data);
    ServiceStub -> BpOnboardclientManagerReceiver : onReceive(data)
    BpOnboardclientManagerReceiver -> Binder : (void)remote()->transact(TRANSACT_ONRECEIVE, data, &reply);
    Binder -> BnOnboardclientManagerReceiver : onTransact(TRANSACT_ONRECEIVE)
    BnOnboardclientManagerReceiver -> OnboardclientSampleApplication : OnboardclientSampleReceiver::onReceive()
    note right OnboardclientSampleApplication : message->sendToTarget(obtainMessage());
    OnboardclientSampleApplication -> OnboardclientSampleApplication : OnboardclientSampleApplication::OnboardclientHandler::handleMessage(msg)

@enduml
```

// @CGA_VARIANT_START{"__GLOBAL_SCOPE__:__DYNAMIC2_DESIGN__:variant"}
- add your comments for dynamic design of internal design
    - use markdown format
// @CGA_VARIANT___END{"__GLOBAL_SCOPE__:__DYNAMIC2_DESIGN__:variant"}
