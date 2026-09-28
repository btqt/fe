# Component-and-Connector View - module `remotediag` (Runtime, theo chuẩn SEI)

Sơ đồ dưới đây tuân theo cách trình bày Component-and-Connector (C&C) view của SEI
(trong tài liệu *"Documenting Software Architectures: Views and Beyond"* - Clements et al.),
được **thu gọn theo yêu cầu** để chỉ thể hiện đúng 3 đường giao tiếp quan trọng nhất của
`remotediag` với thế giới bên ngoài, và làm rõ rằng cả 2 nguồn yêu cầu chẩn đoán (OTA và OEM
Server) đều **đi qua `PriorityControl`** - điểm arbitration ở tầng logic/nghiệp vụ:

1. `RemoteOTA` &harr; **OTA Master ECU** (giao thức OTA/"FA" qua TCP).
2. **OEM Server** &harr; `remotediag` — tập trung vào: upload diagnostic data (`UploadManager`)
   và nhận diagnostic direct command từ server (`RemoteDirectCommand`).
3. **Binder IPC** giữa `OnboardclientAdapter` và `OnboardclientManagerService` để thực thi UDS
   trên Vehicle ECU (chỉ thực hiện **sau khi** `PriorityControl` đã cấp quyền `PROCESSING`).

`PriorityControl` là **connector arbitration duy nhất được nhấn mạnh trong sơ đồ này**: cả
`RemoteOTA` lẫn `RemoteDirectCommand` đều gọi `requestTriggerProcess()` vào đây trước khi được
phép gửi UDS xuống ECU; `PriorityControl` quyết định ai được `PROCESSING`, ai bị `SUSPENDED`
hay `DISCARDED` dựa trên chính sách ưu tiên cố định (`PRIO_OTA_HIGH=10 < PRIO_WARNING_TRIGGER=20
< ... < PRIO_OTA_LOW=200 < PRIO_MAX=255`).

## Chú giải (Diagram Key)

| Ký hiệu (shape) | Stereotype | Ý nghĩa |
|---|---|---|
| Hình chữ nhật `[ ]` | `<<component>>` | Đơn vị phần mềm chạy runtime (class/module cụ thể) |
| Hình con nhộng `([ ])` | `<<external system>>` | Hệ thống/tiến trình bên ngoài `remotediag` |
| Hình lục giác `{{ }}` | `<<connector>>` | Cơ chế kết nối dùng chung, được reify thành 1 phần tử kiến trúc — ở đây là **`PriorityControl`, connector arbitration (scheduling) dùng chung giữa `RemoteOTA` và `RemoteDirectCommand`** |
| Khung subgraph | `<<process>>` | Ranh giới tiến trình `remotediag` |

| Tag trên connector | Loại tương tác |
|---|---|
| `[STREAM]` | Luồng byte qua raw TCP socket (giao thức OTA/"FA") |
| `[NET]` | Giao thức mạng ra Cloud/OEM Server (gRPC/HTTPS, MQTT) |
| `[CALL]` | Gọi hàm đồng bộ: xin xử lý (`requestTriggerProcess`), nhận trạng thái (`notifyStatus`), hoặc gửi UDS (`sendUdsData`) |
| `[IPC]` | Trao đổi Binder IPC cross-process với `OnboardclientManagerService` |

```plantuml
@startuml
'!theme plain
skinparam backgroundColor #FEFEFE
skinparam defaultFontName "Segoe UI"
skinparam defaultFontSize 11
skinparam componentStyle rectangle
skinparam padding 8
skinparam roundCorner 6

' ── Color Palette ──
skinparam component {
    BackgroundColor #E8F0FE
    BorderColor #4285F4
    FontColor #1A1A2E
}
skinparam node {
    BackgroundColor #FFF3E0
    BorderColor #FB8C00
    FontColor #1A1A2E
}
skinparam rectangle {
    BackgroundColor #F1F8E9
    BorderColor #7CB342
}
skinparam hexagon {
    BackgroundColor #FCE4EC
    BorderColor #E53935
}

' ══════════════════════════════════════════════
'  LEGEND / Diagram Key (SEI C&C notation)
' ══════════════════════════════════════════════
legend right
  |= Ký hiệu |= Stereotype |= Ý nghĩa |
  | Hình chữ nhật | <<component>> | Đơn vị phần mềm runtime |
  | Hình con nhộng | <<external system>> | Hệ thống bên ngoài remotediag |
  | Hình lục giác | <<connector>> | Arbitration dùng chung |
  | Khung bao | <<process>> | Ranh giới tiến trình |
  |= Tag |= Loại tương tác |
  | [STREAM] | Raw TCP socket (OTA/"FA") |
  | [NET]    | gRPC/HTTPS, MQTT |
  | [CALL]   | Gọi hàm đồng bộ |
  | [IPC]    | Binder IPC cross-process |
end legend

' ══════════════════════════════════════════════
'  EXTERNAL SYSTEMS
' ══════════════════════════════════════════════
node "<<external system>>\nOTA Master ECU" as OTAECU #FFF3E0
node "<<external system>>\nOEM Server\n(Cloud / DCM)" as OEMSRV #FFF3E0
node "<<external system>>\nVehicle ECU\n(qua OnboardclientManagerService)" as TARGETECU #FFF3E0

' ══════════════════════════════════════════════
'  PROCESS BOUNDARY: remotediag
' ══════════════════════════════════════════════
rectangle "<<process>> remotediag" as RDG #F1F8E9 {

    component "<<component>>\nRemoteOTA" as OTA #E8F0FE
    component "<<component>>\nRemoteDirectCommand" as DIRCMD #E8F0FE
    component "<<component>>\nUploadManager" as UPLOAD #E8F0FE
    component "<<component>>\nOnboardclientAdapter" as OBC #E8F0FE

    ' PriorityControl — connector arbitration (hexagon workaround)
    rectangle "<<connector>>\nPriorityControl\n——\nDiagnostic Trigger Arbitration\n(fixed-priority preemptive scheduling)" as PRIO #FCE4EC
}

' ══════════════════════════════════════════════
'  CONNECTIONS
' ══════════════════════════════════════════════

' --- 1. OTA Master ECU  <-->  RemoteOTA ---
OTAECU <--> OTA : [STREAM]\ngiao thức OTA/FA (TCP)

' --- 2. OEM Server  -->  RemoteDirectCommand ---
OEMSRV --> DIRCMD : [NET]\ndiagnostic direct command

' --- 2b. UploadManager  -->  OEM Server ---
UPLOAD --> OEMSRV : [NET]\nupload diagnostic data

' --- 3. requestTriggerProcess  -->  PriorityControl ---
OTA --> PRIO : [CALL]\nrequestTriggerProcess\n(yêu cầu chẩn đoán từ OTA)
DIRCMD --> PRIO : [CALL]\nrequestTriggerProcess\n(yêu cầu chẩn đoán từ OEM Server)

' --- 4. PriorityControl  -->  notifyStatus ---
PRIO --> OTA : [CALL]\nnotifyStatus:\nPROCESSING / SUSPENDED / DISCARDED
PRIO --> DIRCMD : [CALL]\nnotifyStatus:\nPROCESSING / SUSPENDED / DISCARDED

' --- 5. sendUdsData (sau khi được cấp quyền) ---
OTA --> OBC : [CALL]\nsendUdsData\n(khi được cấp quyền)
DIRCMD --> OBC : [CALL]\nsendUdsData\n(khi được cấp quyền)

' --- 6. Binder IPC ---
OBC <--> TARGETECU : [IPC]\nBinder IPC:\nUDS request/response

@enduml
```

## Ghi chú kiến trúc

- **`PriorityControl` là điểm arbitration ở tầng logic/nghiệp vụ**: nó không biết gì về UDS hay
  `OnboardclientAdapter`, chỉ quản lý trạng thái của từng `DiagTrigger` (`PROCESSING`/`SUSPENDED`/
  `DISCARDED`/`PENDING`) theo chính sách ưu tiên cố định. Cả yêu cầu từ `RemoteOTA` (khởi phát bởi
  OTA Master ECU) và từ `RemoteDirectCommand` (khởi phát bởi OEM Server) đều phải qua
  `requestTriggerProcess()` tại đây trước khi được phép chạy.
- Nếu OTA đang `PROCESSING` với `PRIO_OTA_HIGH` → cờ `m_ota_non_interuptible = true` → mọi yêu cầu
  khác (kể cả từ OEM Server) đều không thể ngắt, chỉ xếp `PENDING`. Ngược lại nếu OTA đang chạy ở
  `PRIO_OTA_LOW` mà có yêu cầu từ OEM Server ưu tiên cao hơn → OTA bị `DISCARDED` ngay lập tức.
- `OnboardclientAdapter` vẫn còn tồn tại như 1 component IPC gateway phía sau (giữ khoá phiên UDS
  vật lý — xem thảo luận trước), nhưng trong sơ đồ này **không được nhấn mạnh là arbitration**
  theo đúng phạm vi bạn yêu cầu; nó chỉ đóng vai trò component thực thi việc gửi UDS sau khi đã
  được `PriorityControl` cấp quyền.
- Sơ đồ này lược bỏ các luồng khác (DTC/SSR/RoB, IG, Location, Power...) so với bản C&C runtime
  đầy đủ để tập trung đúng 3 đường giao tiếp được yêu cầu.
