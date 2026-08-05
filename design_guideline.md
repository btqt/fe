1. Sequence Diagram Guide
1.1 Returning Void Syncrhonous Function call
The return arrow is required for the synchronous function call to show the active time of each object during function execution time.
The CALL arrow should have SOLID body and bold head: #1, #2
The RETURN arrow should have DASHED body and bold head: #3, #4
```plantuml
@startuml
autonumber

skinparam sequence {
    sequenceArrowHead standard
}

participant Object1 #Application
participant Object2 #Application
participant Object3 #Application

== void synchronous return ==
Object1 -> Object2: voidReturningCall1
Object2 -> Object3: voidReturningCall2
Object2 <-- Object3
Object1 <-- Object2
@enduml
```

1.2 Returning Value Syncrhonous Function call
The return arrow is required for the synchronous function call to show the active time of each object during function execution time  and for further exception handling if any.
The CALL arrow should have SOLID body and bold head: #1, #2
The RETURN arrow should have DASHED body and bold head: #3, #4
```plantuml
@startuml
autonumber

skinparam sequence {
    sequenceArrowHead standard
}

participant Object1 #Application
participant Object2 #Application
participant Object3 #Application

== normal synchronous return ==
Object1 -> Object2: synchronousRequest1(param)
Object2 -> Object3: synchronousRequest2(param)
Object2 <-- Object3: E_OK
Object1 <-- Object2: E_OK
@enduml
```

1.3 Void Asyncrhonous Function call
The return arrow is not needed,
The CALL arrow should have SOLID body and light head: #1
The RETURN arrow is no need for this: N/A
```plantuml
@startuml
autonumber

skinparam sequence {
    sequenceArrowHead standard
}

participant Object1 #Application
participant Object2 #Application
participant Object3 #Application

== void asynchronous ==
Object1 ->> Object2: asynchronousRequest(param, callback)
Object2 -> Object3: synchronousRequest1(param)
Object2 <-- Object3: E_OK
Object1 <- Object2: callback()

@enduml
```
1.4 Returning Value Asyncrhonous Function call
This is treated similar to the synchronous function call, the Return arrow is needed for showing the exception handling logic if any.
The CALL arrow should have SOLID body and bold head: #1
The RETURN arrow should have DASHED body and bold head: #2
```plantuml
@startuml
autonumber

skinparam sequence {
    sequenceArrowHead standard
}

participant Object1 #Application
participant Object2 #Application
participant Object3 #Application

== non void asynchronous ==
Object1 -> Object2: asynchronousRequest(param, callback)
Object1 <-- Object2: E_REQUEST_SENT_SUCESS
Object2 -> Object3: synchronousRequest1(param)
Object2 <-- Object3: E_OK
Object1 <- Object2: callback()
@enduml
```
2. Sample
2.1. HLD Sequence Diagram
```info
Sequence Diagram Object는 SW Block Diagram에 표기되어 있는 모듈명과 일치해야 함
동일한 모듈이 없을 경우 Block Diagram를 업데이트 할지 아키텍트 협의 필요
Sequence Diagram Object는 Black box로 봄, 내부 설계는 SDD에서 표현
Sequence 간 call에대한 text는 External API name이 되어야 함. 단 API로 기술될 수 없는 경우(동작 등) Description으로 대체할 수 있음. (예, Click SOS Button)
Trigger 대상이 명확해야함 (DCM Core/User or button/other ECU)
```
```plantuml
@startuml HLD sequence diagram color palette
title HLD sequence diagram color palette

skinparam MinClassWidth 200
skinparam nodesep 10
skinparam Ranksep 10

rectangle "DCM Box\n~#LightYellow" #LightYellow
rectangle "Services/Participants internal\n~#Application" #Application
rectangle "Toyota\n~#Green" #Green
rectangle "SWCs\n~#Application" #Application
rectangle "Services/Participants external\n~#LightGray" #LightGray
rectangle "Linux Libraries & Utility\n~#LightGray" #LightGray
rectangle "Linux Kernel/Drivers\n~#LightGray" #LightGray
rectangle "Highlight/Important parts\n~#Pink" #Pink
@enduml
```
2.1.1 Usecase - Update Time Based on Time Source Priority and how time is restored after reboot
```plantuml
@startuml
 
title Update Time based on Time Source Priority\nand how time is restored after reboot(26BEV)
autonumber
'!pragma teoz true
skinparam sequence {
    ParticipantPadding 10
    ArrowColor Black
    ActorBorderColor Black
    LifeLineBorderColor Black
    ParticipantBackgroundColor Black
    ParticipantFontSize 13
}
skinparam sequenceReferenceBackgroundColor white
 
box DCM #LightYellow
    participant "<b>DCE" as dce #LightGreen
    participant "<b>Other\n<b>Services" as others #APPLICATION
    participant "<b>TimeMgr" as time #APPLICATION
    participant "<b>LocationMgr" as location #APPLICATION
    participant "<b>TelephonyMgr" as telephony #APPLICATION
    participant "<b>PowerMgr" as power #APPLICATION
    participant "<b>System\n<b>Property" as property #APPLICATION
    participant "<b>Kernel" as kernel #LightGray
end box

participant "<b>GNSS" as gnss #LightGray
participant "<b>Network" as network #LightGray
participant "<b>NTP\n<b>Server" as ntp #LightGray

==BOOT COMPLETE==

time -> time : last_set_time_source = INIT_TIME
time -> property : read power mode stored in system property
alt #pink if last power mode is NOT STOP mode
    time -> property : read current time stored in system property
    property --> time : return stored time
    time -> kernel : set system time(stored time)
end alt

note over time
    <b>Time Source Priority:
    <b>GPS_TIME > NETWORK_TIME > NTP_TIME
end note

par time sync by GNSS
    loop
        gnss -> location : send GNSS information
        alt on boot or when |system time - gps time| >= 3sec
            location -> time : setCurrentTime()
            time -> time : set last_set_time_source = GPS_TIME
            time -> kernel : set_sys_time()
            time -> others : onTimeReceived()
        end alt
    end loop
else time sync by Network
    loop whenever NITZ request is received
        network -> telephony : send NITZ information
        telephony -> time : setCurrentTime()
        alt if last_set_time_source == NETWORK_TIME or NTP_TIME or INIT_TIME
            time -> time : set last_set_time_source = NETWORK_TIME
            time -> kernel : set_sys_time()
            time -> others : onTimeReceived()
        end alt
    end loop
else time sync by NTP
    dce -> time : dcepf_net_ntp_apply_url_create\n(ntp_server(url), dcepf_operation_result_cb_t callback)
    time -> time : convert url to ip address
    loop every 5 mins or when |system time - ntp time| >= 3sec
        time -> ntp : request time to NTP Server
        ntp --> time : return ntp time
        time -> time : setCurrentTime()
        alt if last_set_time_source == NTP_TIME or INIT_TIME
            time -> time : set last_set_time_source = NTP_TIME
            time -> kernel : set_sys_time()
            time -> others : onTimeReceived()
        end alt
    end loop
else

power -> time : onExtValueChanged(ECO/Standby/Stop)
time -> property : store power mode to system property
end par

==REBOOT OCCURS or ENTERS STOP MODE==
alt #pink REBOOT OCCURS
    power -> time : notify reboot event
else ENTERS STOP MODE
    power -> time : notify STOP mode
    time -> property : store power mode to system property
end alt

alt #pink last_set_time_source == GPS or Network or NTP
    time -> property : store current time to system property
end alt
power -> kernel : reboot() or poweroff()
note over others, kernel #Yellow
    <b>"DCM Reboot" or "enter STOP mode and boot up"
end note
note over time
    <b>go to beginning
end note

@enduml
```

2.1.2 Usecase - dcepf_net_ip_apply_dns_gw_v6_delete
```plantuml
@startuml

'!pragma teoz true
autonumber
skinparam sequence {
  ParticipantPadding 20
  ArrowColor Black
  ActorBorderColor Black
  LifeLineBorderColor Black
  ParticipantBorderColor Black
  ParticipantBackgroundColor White
  ParticipantFontName "Arial"
  ParticipantFontSize 13
  ParticipantFontColor Black
}

box DCM #lightYellow
  participant "DCE" #Green
  participant "PF API" as PFAPI #Application
  participant "RouteMgr" #Application
  participant "KERNEL" #Lightgray
end box

== dcepf_net_ip_apply_dns_gw_v6_delete() ==

"DCE" -> PFAPI: dcepf_net_ip_apply_dns_gw_v6_delete\n(handle, callback)

alt if(handle == null or invalid)
    PFAPI --> "DCE": return DCEPF_API_STATUS_INVALID_PARAM
else
    PFAPI -> PFAPI: Save callback function pointer
    PFAPI --> "DCE": return DCEPF_API_STATUS_SUCCESS

        PFAPI -> RouteMgr: rmnetDisconnected()
            RouteMgr -> KERNEL: delete Routing Rule \n (/sbin/ip -6 route del default via <gateway>\n dev <interface> table <table_id>) 
            RouteMgr -> KERNEL: delete default Gw \n (/sbin/route -A -inet6 delete default <interface>)
            RouteMgr -> KERNEL: delete NAT \n (/usr/sbin/iptables -t nat -D POSTROUTING -s <srcip> \n -o <interface> -j MASQUERADE)
            RouteMgr --> PFAPI: return result
            alt result == OK 
            
            PFAPI -> "DCE": callback(DCEPF_RESULT_SUCCESS)
            else 
            PFAPI -> "DCE": callback(DCEPF_RESULT_FAILURE)
            end


end



@enduml
```
2.1.3 Usecase - Overall Initial bootup sequence
```plantuml
@startuml 

title __Boot Sequence__

'!pragma teoz true
autonumber

box DCM #Lightyellow
    box AP #Lightyellow 
        collections "<b>Regional Apps" as RAPP #application
        collections "<b>TMC SW" as TMC #lightGreen
        collections "<b>DCM/DCE PF" as PFM #application
        participant "<b>systemd" as SD #lightgray
        participant "<b>Kernel" as LK #lightgray
        participant "<b>MODEM" as MODEM #lightgray
    end box
    box MCU #Lightyellow
        collections "<b>SWCs" as swc #application
        participant "<b>OSEK OS" as os #lightgray
    end box
end box

participant "<b>BATT" as batt #lightgray

batt -> os : B+ ON
os --> swc : start SWC Tasks
swc --> LK : AP power ON\n(set GPIO(NAD_3V9_ON) to HIGH)
SD <-- LK : launch
SD --> MODEM : [Qualcomm Scripts] \nUpload MODEM image\n from fileSystem to MODEM Memory
SD --> MODEM : [Qualcomm Scripts] \nRequest PMIC to MODEM power on
PFM <-- SD : launch appMgr
PFM -> SD : [AppMgr] SD_Notify (READY=1)
PFM <-- SD : launch other DCM/DCE PF Services
PFM --> PFM : DCM/DCE PF Services send\n notifyReady to AppMgr

== BOOT_COMPLETED_PRE ==

PFM -> PFM : [AppMgr] broadcastSystemPost(\nSYS_POST_BOOT_COMPLETED_PRE) 

TMC <-- SD : launch
RAPP <-- PFM : [AppMgr] launch
RAPP -> PFM : Regional Apps send\nnotifyReady to AppMgr


== BOOT_COMPLETED ==

PFM -> PFM : [AppMgr] broadcastSystemPost(\nSYS_POST_BOOT_COMPLETED) 
RAPP <- PFM : [AppMgr] broadcastSystemPost(\nSYS_POST_BOOT_COMPLETED) 
PFM -> swc : [CommMgr] boot complete
swc -> swc : [IPC_SWC] send boot\n complete to other SWCs

@enduml
```