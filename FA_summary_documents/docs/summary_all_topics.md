# Tổng Hợp 40 Topic Kiến Trúc Phần Mềm — FA Projects

> **Ngôn ngữ:** Tiếng Việt, thuật ngữ kỹ thuật giữ nguyên tiếng Anh  
> **Lĩnh vực:** Automotive Embedded Systems / Android IVI (AAOS) / Telematics  
> **Tổng số topic:** 40 + 1 tổng hợp  
> **Cập nhật:** 2026-05-15

---

## Mục Lục Nhanh

| #   | Tên Topic                              | Dự án                 | Kết quả nổi bật                 |
| --- | -------------------------------------- | --------------------- | ------------------------------- |
| 01  | Power Manager Event Notification       | Toyota 24DCM          | 750ms → 22ms (34×)              |
| 02  | Function Architect Task — VMS Service  | Renault A-IVI2        | -73 MB RAM                      |
| 03  | Concurrent Processing Factory Manager  | Honda TSU             | 23.3s → 2.85s (8.2×)            |
| 04  | TimeManager for BAM                    | BMW ICON              | Proxy+Façade static/dynamic     |
| 05  | Projection Player Improvement          | VW Cockpit            | 2000 → 1200 LOC                 |
| 06  | Map Download App Design                | Honda TSU             | Remote Interface Manager        |
| 07  | Hierarchical State Machine             | GM GEN12              | HSM vs FSM                      |
| 08  | Cinemo Events in Media Service         | VW ICAS3.1            | 12 → 11 threads                 |
| 09  | Provisioning App Internal Design       | BMW WAVE              | Communicator+Command            |
| 10  | LPA Design Improvement                 | BMW WAVE/ICON         | LPA Modularization              |
| 11  | IODiagnostic Architecture              | BMW ICONICC           | 0.018% → 0.005% CPU             |
| 12  | Handwriting Recognition Design         | VW ICAS3CHN           | HWRManager in InputService      |
| 13  | Communication Monitoring Commonization | MB BR167M2            | 0.018% → 0.005% CPU avg         |
| 14  | Carplay Reconnection Improvement       | Nissan DA2            | 61s → 9s (6.8×)                 |
| 15  | CAN Dispatching Architecture           | GM Info3.5            | Observer Pattern                |
| 16  | NGeCall Architecture Design            | BMW WAVE              | Interface decomposition         |
| 17  | Carplay Ultra Storage Management       | ConnectWide           | Singleton Engine layer          |
| 18  | Network Management Handler             | VW Cockpit            | Factory Pattern, SRP            |
| 19  | Wifi Manager Commonization             | VW Cockpit / BMW ICON | CoR + Abstract Factory          |
| 20  | BT Manager Design                      | BMW ICONICC           | SOME/IP + BTManager             |
| 21  | Change Mode Sequence Improvement       | HMI IVI               | Common Change mode Architecture |
| 22  | Common Factory Service for AVN Virt.   | GM/HKMC/BMW RSE27     | SOA, 40–60% reuse               |
| 23  | V2X Manager Commonization              | JLR VCM               | 15.93 → 92 MB/s                 |
| 24  | Factory Service Commonization          | Renault/Nissan        | 80s → 45s boot                  |
| 25  | Sport Chrono Module Improvement        | Porsche E3PA          | State Pattern + Observers       |
| 26  | BT Remote Device Sync                  | BMW RSE27             | 15.88ms → 6.22ms                |
| 27  | ECall Application Skeleton             | BMW/Honda/Toyota      | 5 → 2 process, -73% mem         |
| 28  | Data Sharing Service Optimization      | FPK Cheetah           | 3.706s → 0.169s (21.9×)         |
| 29  | KIPC Architecture Improvement          | VW ICAS3              | 10 msgs: 10h → 1h               |
| 30  | Sync Partition SW Update               | Toyota 26BEV          | -20.7% time, data robust        |
| 31  | BLE Commonization Architecture         | BMW ICONICC           | Strategy per BLE service        |
| 32  | VR Common Service                      | JLR P-IVI             | -41% memory                     |
| 33  | Home App Performance                   | JLR P-IVI             | -0.724s boot, -18MB             |
| 34  | Common 3D in P-IVI                     | JLR P-IVI             | -4,092ms load, -22% CPU         |
| 35  | Alliance Update Manager Architecture   | Renault A-IVI2        | State Machine + Event-Driven    |
| 36  | Navigation Adaptation Layer            | VW ICAS3              | Thread pool, 4 managers         |
| 37  | SXM Broadcast Performance              | LG VS DCV             | 0.425s → 0.001s (425×)          |
| 38  | Call Manager Commonization             | JLR P-IVI             | 81ms → 41ms, 7% → 0.5% CPU      |
| 39  | Process Communication Manager          | VDC (JLR Telematics)  | -120ms startup per process      |
| 40  | Power Mode Management Commonization    | TCUA (JLR EVA3)       | BitMask O(1), ~337ms KPI        |

---

## Topic 01 — Improving performance of event notification in Power Manager

**Tác giả:** Hoang Quang Trung | **Dự án:** Toyota 24DCM | **Năm:** 2025

---

### Problem

Power Manager service trong hệ thống Toyota 24DCM có cơ chế **On-Demand Thread**: mỗi khi cần gửi notification, service tạo mới 38 thread (tương ứng 38 receiver), gửi thông báo, rồi hủy tất cả. Chi phí tạo/hủy thread ~20ms × 38 = **760ms/event**. Khi sự kiện IG OFF đến sau IG ON, nó phải chờ ~5 event xử lý xong trước → tổng delay **~4 giây** → Remote AC không tắt đúng thời hạn 1.5s yêu cầu (bug DCM24MON-4575).

### Solution

Sau khi so sánh 3 phương án (Thread Cache Pool, Fixed Thread Pool, Dynamic Thread Pool), giải pháp được chọn là **Dynamic Thread Pool (Proposal 3)** kết hợp với **Generic Interface Integration**:

- **Core Pool Size:** 2 thread thường trực (baseline)
- **Scale-Up:** Khi một thread xử lý vượt ngưỡng **500ms** → tự tạo thêm thread mới (hang detection)
- **Scale-Down:** Sau **30 giây** idle → giảm về baseline để tiết kiệm tài nguyên
- **ITask Interface:** `DynamicThreadPool` hoạt động với bất kỳ task nào implement `ITask`, tách biệt logic nghiệp vụ khỏi thread management → tái sử dụng được cho toàn hệ thống

### Outcome

| Tiêu chí               | On-Demand Thread | Dynamic Thread Pool        |
| ---------------------- | ---------------- | -------------------------- |
| Thời gian notify/event | ~750ms           | ~22ms (**giảm 97%**)       |
| CPU khi notify         | 14%              | 3.7% (giảm 74%)            |
| Virtual Memory         | 335 MB           | 142 MB (giảm 57%)          |
| Bug DCM24MON-4575      | 4.2s delay       | **140ms** — đạt yêu cầu ✅ |

Tất cả quality attribute ưu tiên cao (Performance, Reliability) đều đạt. Kế hoạch mở rộng sang các dự án khác (26BEV, Tiger Framework) vào 2026.

---

## Topic 02 — Function Architect Task: Alliance Car VMS Service

**Tác giả:** Tran The Dan | **Dự án:** Renault A-IVI2 (Android AAOS) | **Năm:** 2024

### Problem

`AllianceCarVMSService` chạy cùng process với `AllianceCarService` (>60 plugin services). Các VMS object của Google VMS SDK chiếm ~70% RAM của process → Java **Garbage Collection** (GC) bị triggered, block toàn bộ process ~3 giây → `AllianceCarPowerService` không xử lý kịp tín hiệu `WelcomeSequenceStatus` từ Cluster → animation Welcome trên IVI bị chậm so với Cluster (yêu cầu <100ms, thực tế >3s).

### Solution

Hai phương án được so sánh:

- **Lazy Initialization:** Trì hoãn khởi tạo VMS object đến sau khi Welcome Sequence hoàn thành qua AIDL callback `IPowerVMSListener` → giảm tải startup nhưng vẫn tiềm ẩn GC block ở thời điểm khác.
- **Service Separation (được chọn):** Tách `AllianceCarVMSService` ra **process riêng** (External Service) → GC của VMS không ảnh hưởng tới process chính. `AllianceCarExternalVmsProviderService` xử lý toàn bộ VMS SDK (Path API, Route API), expose data qua `AllianceCarVmsHorizonManager` AIDL interface.

### Outcome

| Tiêu chí                      | Trước    | Sau                    |
| ----------------------------- | -------- | ---------------------- |
| Memory (Alliance Car Service) | 109 MB   | 36 MB (**-73 MB**)     |
| GC execution time             | ~2984 ms | ~174 ms (**-2809 ms**) |
| Total RAM tăng thêm           | —        | +2 MB (không đáng kể)  |

Welcome Sequence không còn bị delay. Codebase tách biệt rõ ràng, dễ maintain. Kế hoạch tiếp tục monitor memory và bổ sung lazy init nếu cần.

---

## Topic 03 — Concurrent Processing for Factory Manager

**Tác giả:** Pham Thanh Bao | **Dự án:** Honda TSU MY23 | **Năm:** 2023

### Problem

Factory Manager Service (FMS) chỉ xử lý **tuần tự từng lệnh** (single-command-processing). Test tool gửi đồng thời nhiều lệnh → FMS phải xử lý lần lượt → tăng thời gian kiểm tra (tact time) → giảm UPH (Unit Per Hour) trên FA line, tăng chi phí nhân công và thiết bị.

### Solution

So sánh 2 phương án:

- **Handler-Looper Approach:** Mỗi service có handler riêng, xử lý song song qua các service, nhưng không hỗ trợ các lệnh FMS tự xử lý (non-target-service commands) và không tận dụng được khi external service hỗ trợ concurrent.
- **Thread Pool Approach (được chọn):** Pre-initialized worker threads, tất cả command được enqueue vào hàng đợi, worker threads dequeue và xử lý song song. Dependency giữa các command được quản lý bằng **Dependence ID** do test tool quy định — FMS không cần biết thứ tự nghiệp vụ, chỉ cần chờ command có Dependence ID tương ứng hoàn thành. Toàn bộ thay đổi chỉ nằm trong **Common Part** → các project mới kế thừa tự động, Variant Part = 0 dòng thay đổi.

### Outcome

| Tiêu chí                | FMS cũ                       | Thread Pool FMS                  |
| ----------------------- | ---------------------------- | -------------------------------- |
| Thời gian xử lý 10 lệnh | 23.315s                      | **2.85s** (giảm ~88%)            |
| Command bị mất          | 9/10                         | **0/10** ✅                      |
| Reusability             | Cần re-implement mỗi project | Common part → tự động kế thừa ✅ |

---

## Topic 04 — TimeManager for BAM in BMW ICON

**Tác giả:** Pham Thanh Binh | **Dự án:** BMW ICONICC (BAM chipset) | **Năm:** 2024

### Problem

TimeManager trong ICONICC cần thu thập thời gian từ nhiều nguồn (BMW Backend, GNSS, WUC, IPB, Persistent) để tính toán SystemTime chính xác và bảo mật. Yêu cầu đặt ra là: thời gian phải được đồng bộ với Backend để đảm bảo certificate key hoạt động, đồng thời dễ mở rộng khi có nguồn thời gian mới, không truy cập Backend quá thường xuyên (hạn chế bảo mật).

### Solution

**Static View:** So sánh 3 design pattern → chọn **Hybrid (Proxy + Façade)**:

- Mỗi nguồn thời gian có một `TimeProxy` riêng (cache dữ liệu, tăng performance).
- `EventManager` đóng vai trò Façade — `TimeManagerService` chỉ cần gọi theo loại thời gian, không cần biết proxy cụ thể → dễ mở rộng.

**Dynamic View:** So sánh 3 phương án → chọn **Proposal 3 (Proxy maintain time)**:

- Mỗi Proxy tự request nguồn theo chu kỳ riêng, **duy trì (maintain) thời gian** bằng cách tự đếm liên tục.
- `TimeManagerService` collect từ tất cả Proxy theo chu kỳ 60s, dùng bảng ưu tiên để chọn nguồn hợp lệ → set vào Linux System Time.
- Hang detection qua DTC nếu không có nguồn thời gian nào available.

### Outcome

| Tiêu chí                  | Proposal 1 (tuần tự) | Proposal 3 (Proxy maintain) |
| ------------------------- | -------------------- | --------------------------- |
| Latency cập nhật time     | ~60s                 | <proxy period (~100ms)      |
| Time jump/phút            | ~1 lần               | **~1 lần** (ổn định)        |
| Bảo mật với Backend       | Không                | **Có** ✅                   |
| Tần suất truy cập Backend | 1 lần/phút (cố định) | 1 lần/phút (chỉ khi cần)    |

---

## Topic 05 — Improve Design of Projection Player in Video Projection Project

**Tác giả:** Dang Thanh Cong | **Dự án:** VW Cockpit 2022+ (ICAS3CHN) | **Năm:** 2025

### Problem

`ProjectionPlayer` — component điều khiển toàn bộ quá trình phát video từ điện thoại lên HU — có 2 vấn đề:

1. **Quản lý trạng thái bằng multiple boolean flags** → logic xử lý tin nhắn phức tạp, dễ sai, khó mở rộng. Cyclomatic complexity cao (ví dụ: `onCinemoPlaySpeed` = 12).
2. **Gọi thẳng Cinemo API** → vi phạm Dependency Inversion Principle, khó thay thư viện, khó test.

### Solution

**Improve 1 — State Pattern (được chọn vs State-Event Mapping + Command):**

- Mỗi trạng thái (PlayingState, PauseState, FastForwardState, HighPriorityState…) là một class riêng implement `IState`.
- `ProjectionPlayer` chỉ dispatch message đến `currentState`, không còn if/else phức tạp.

**Improve 2 — Strategy Pattern (được chọn vs Facade):**

- `IPlayerStrategy` là interface chung; `CinemoPlayer` implement nó, bọc toàn bộ Cinemo API.
- `ProjectionPlayer` chỉ giao tiếp qua interface → dễ thêm `VisualOnPlayer` hoặc switch library tại runtime.

### Outcome

| Tiêu chí                           | Trước    | Sau                     |
| ---------------------------------- | -------- | ----------------------- |
| `ProjectionPlayer` LOC             | 2000     | 1200                    |
| Cyclomatic complexity (avg method) | ~9-12    | **4-5**                 |
| Thêm state mới (Speed Over)        | 3 ngày   | **2 ngày**              |
| Thêm library mới                   | 5 ngày   | **3 ngày**              |
| 5 bug liên quan flags              | Tái hiện | **Đã fix hoàn toàn** ✅ |

---

## Topic 06 — New App Design for Map Download Feature in Honda TSU

**Tác giả:** Dang Cao | **Dự án:** Honda TSU 26MY | **Năm:** 2023

### Problem

MPU và ADAS cần dữ liệu bản đồ HD/SD từ server bên ngoài phương tiện để hỗ trợ lái xe tự động, nhưng vì lý do an ninh mạng (cybersecurity), chúng không được kết nối trực tiếp ra ngoài → TSU phải đóng vai trò proxy trung gian. Cần thiết kế **Map Download App** mới trên tầng Application, xử lý đồng thời tối đa 5 kết nối download, đạt tốc độ cao (lên đến 12.6 lần/giây ở tốc độ 180km/h).

### Solution

So sánh 2 phương án:

- **Proposal 2 (Direct):** App tự dùng `libcurl` giao tiếp thẳng với server, quản lý 5 worker threads nội bộ → performance cao, nhưng duplicate logic và không tái sử dụng được.
- **Proposal 1 (Reuse RIM — được chọn):** Tái sử dụng `Remote Interface Manager` (service đã có) để xác thực và communicate với server. Map Download App chỉ đảm nhận vai trò HTTP server (nhận request từ MPU), queue management, và chuyển đổi request → gửi qua RIM qua Binder. Thiết kế modular: `HttpServer`, `MapDownloadQueue`, `MapDownloadOperator`, `MapDownloadStorage`.

### Outcome

- Cả 2 proposal đều đáp ứng **target time download** (test với server giả lập).
- Proposal 1 được chọn vì **Reusability cao** (RIM dùng chung cho nhiều app), dễ maintain, logic xác thực TLS không bị duplicate.
- Kế hoạch triển khai trong Honda MY26 project.

---

## Topic 07 — Hierarchical State Machine Pattern for Power Manager in GEN12

**Tác giả:** Phuong DAO | **Dự án:** GM GEN12 | **Năm:** 2022

### Problem
Power Manager của GM GEN12 cần cài đặt GMSM (GM State Machine) với 4 trạng thái chính (off, powering_up, powering_down, powered). Sau một Change Request (CR): thêm suspend-to-RAM, thêm sub-states (running, suspending). Vấn đề: cách nào cài đặt State Machine để **dễ thay đổi khi có CR** và tránh code duplicate giữa super-state và sub-state?

### Solution
So sánh 2 phương án:
- **Proposal 1 — FSM (Finite State Machine):** Dùng function pointer table — đơn giản nhưng duplicate code khi xử lý event ở sub-state và super-state, khó maintain khi có CR.
- **Proposal 2 — HSM (Hierarchical State Machine — được chọn):** Chia GMSM thành các module: state machine và các class State riêng biệt. Super-state xử lý event chung, sub-state xử lý event đặc thù → nếu sub-state không handle được, event bubble lên super-state. Dùng Interface Injection để giảm coupling. Hỗ trợ parallel state machines.

### Outcome
| Tiêu chí | FSM | HSM |
|---|---|---|
| Maintainability | Medium | **High** |
| Modifiability (khi CR) | Medium | **High** |
| Reusability | Low | **Medium** |

- CR power state đã được apply thành công trong GEN12 — **giảm đáng kể effort**.
- HSM được **tái sử dụng** cho Power Manager trong Toyota 24DCM project.

---  

---

## Topic 08 — Handling Cinemo Events in Media Service

**Tác giả:** Bang Dinh | **Dự án:** VW ICAS3.1 CHN | **Năm:** 2023

### Problem
Media Native có 3 Player class (JukeboxPlayer, ApplePlayer, BTApplePlayer), mỗi class **tự xử lý Cinemo events nội bộ** và **tự quản lý 1 thread** riêng để đọc event queue → vi phạm Single Responsibility Principle và Open/Closed Principle. Logic xử lý event gần như giống nhau nhưng được implement lặp lại độc lập trong từng class → khó maintain, không tái sử dụng, lãng phí thread (12 threads tổng).

### Solution
So sánh 2 phương án:
- **Design 2 — Observer Pattern:** `EventPublisher` (singleton) dispatch event đến tất cả handler đã subscribe → Reliability thấp (event bị notify không chọn lọc).
- **Design 1 — Chain of Responsibility (được chọn):** `CinemoEventQueue` (singleton) dùng 1 thread đọc event, dispatch theo chain — event dừng tại handler phù hợp. `NowEventHandler` xử lý Now Player events; `TBTEventHandler` xử lý TBT Player events. Handler callback về Player qua `PlayerInterface`.

### Outcome
| Tiêu chí | Thiết kế cũ | Design 1 (Chain of Resp.) |
|---|---|---|
| Số thread xử lý Cinemo event | 2 threads | **1 thread** |
| Tổng threads Media Native | 12 | **11** |
| Reliability | High | Medium (đủ yêu cầu) |
| Reusability | Low | **High** ✅ |
| Maintainability | Low | **High** ✅ |

Tất cả 32 functional requirements đều được trace đến các handler tương ứng và verified.

---

## Topic 09 — Internal Design of Provisioning App supports expanding features

**Tác giả:** Tuyen Dang | **Dự án:** BMW WAVE | **Năm:** 2023

### Problem
`ProvisioningApp` nhồi nhét toàn bộ: proxy của các service, Receiver class, và handler logic trong 1 class duy nhất → vi phạm Single Responsibility Principle. `ProvAppHandler` dùng switch-case + Enum để dispatch request → mỗi khi thêm service mới hoặc request mới đều phải sửa nhiều nơi → khó expand, khó reuse sang ICON/TMP4G.

### Solution
Hai cải tiến độc lập được thiết kế:

**Cải tiến 1 — Decouple dependencies: Communicator Pattern (chọn vs Façade)**
- Mỗi Service được đóng gói trong Manager class riêng (proxy + subscribe/unsubscribe).
- `Communicator` (singleton) quản lý tất cả Manager, hỗ trợ lazy initialization (Reflection pattern).
- `ProvisioningApp` chỉ cần thêm dependency vào Communicator khi có service mới → không sửa logic cũ.
- Chọn hơn Façade vì Reusability cao hơn (áp dụng được nhiều project khác nhau).

**Cải tiến 2 — Handler design: Command Pattern (chọn vs Chain of Responsibility)**
- Mỗi loại request/notification được xử lý bởi một Handler class riêng implement interface `Command`.
- Thêm request mới → thêm Command class mới, không sửa class cũ.
- Chọn hơn Chain of Responsibility vì hầu hết request chỉ cần 1 handler → chain là không cần thiết, gây deep stack trace.

### Outcome
- Provisioning App hoạt động ổn định sau khi áp dụng.
- **Lazy initialization** giảm thời gian `onCreate` xuống **0.1s**.
- Thêm `AudioService` mới → chỉ thêm 1 Manager class, không sửa class cũ ✅.
- Thiết kế có thể tái sử dụng cho bất kỳ app nào theo mô hình proxy-client và pub-sub.

---   

---

## Topic 10 — Improve LPA Design for Easy Application on New Projects

**Tác giả:** Minh Nguyen | **Dự án:** BMW WAVE / ICON | **Năm:** 2021

### Problem
LPA (Local Profile Assistant) quản lý eSIM (M2M và Consumer) theo chuẩn GSMA SGP.02/SGP.22. Thiết kế hiện tại trộn lẫn OEM requirements, GSMA standard, và Telematic platform integration vào cùng một class → khi migrate LPA sang dự án OEM mới, developers phải sửa toàn bộ code base; khi GSMA cập nhật standard hoặc Telematic framework thay đổi, nhiều phần không liên quan cũng bị ảnh hưởng.

### Solution
**LPA Modularization Design** — tách thành 3 component độc lập:
- **LPA OEM Part:** Implement OEM requirements — `M2MSimManager`, `ConsumerSimManager`; mỗi dự án customize tại đây.
- **LPA Standard:** Implement GSMA SGP.02/SGP.22 hoàn toàn → tái sử dụng không thay đổi. Dùng **Factory Pattern** (`SimManagerFactory`) để tạo SIM Manager theo loại (`M2M_SIM`, `CONSUMER_SIM`). Nội bộ dùng **Command Pattern** cho các LPA jobs.
- **Communication Adapter:** **Adapter Pattern** — convert `TelephonyService` và `HTTPClient` của Telematic Platform sang interface chuẩn (`ILpaTelephonyService`, `IHttpClient`) mà LPA Standard yêu cầu.

### Outcome
| Thay đổi | Thiết kế cũ | LPA Modularization |
|---|---|---|
| OEM requirement thay đổi | Sửa OEM + LPA Standard + integration | **Chỉ sửa LPA OEM Part** |
| GSMA standard upgrade | Sửa nhiều nơi | **Chỉ sửa LPA Standard** |
| Telematic framework update | Sửa nhiều nơi | **Chỉ sửa Communication Adapter** |
| Migrate sang project mới | LPA Standard cần modify | **LPA Standard dùng lại 100%** ✅ |

---

## Topic 11 — IODiagnostic Architecture Design

**Tác giả:** Do Minh Khang | **Dự án:** BMW ICONICC (BAM Core) | **Năm:** 2022

### Problem
IODiagnostic là module mới trên BAM core (ICONICC), đảm nhận giao tiếp giữa WUC core và Node0-Diagnostic (LSMF của BMW) qua SOME/IP và ICC/UDS. Cần thiết kế từ đầu để xử lý 3 loại diagnostic: DTC, DID (0x22 read/0x2E write), Routine Control (0x31). Thách thức: khi thêm requirement mới, chỉ nên thay đổi tối thiểu, và có thể tái sử dụng cho NadManager-Diagnostic.

### Solution
So sánh 2 phương án:
- **Proposal 1 — Functional Approach:** Handling class (DTC, DID, RC) mỗi loại tự chứa data item riêng → Maintainability cao nhưng các class phụ thuộc nhau, khó reuse.
- **Proposal 2 — Data Centralized Approach (được chọn):** Tách riêng `IODiagDatas` là **data central object** quản lý tất cả diagnostic items (DID/DTC/Routine). Handling classes (`IODiagRCHandling`, `IODiagDidHandling`, `IODiagDtcHandling`) chỉ xử lý logic, lấy item qua `IODiagDatas`. Strategy Pattern áp dụng cho data items (`IODiagDidItem_BAM`, `IODiagDidItem_WUC`, v.v.). `IODiagICC` (UDS) và `IODiagStubImpl` (SOME/IP) giảm coupling với external modules.

### Outcome
| QA | Proposal 1 | Proposal 2 |
|---|---|---|
| Maintainability | High | Medium |
| Modifiability | Medium | **High** |
| Reusability | Medium | **High** |

Thiết kế Proposal 2 đáp ứng KPI BMW: diagnostic jobs available **trong 10 giây sau startup**. Strategy Pattern cho phép thêm loại diagnostic item mới mà không ảnh hưởng handling classes.

---

## Topic 12 — New Design for Handwriting Recognition in VW ICAS3CHN

**Tác giả:** Nguyen Trung Hieu (hieu4.nguyen) | **Dự án:** VW ICAS3CHN | **Năm:** 2024  
**Supervised by:** Ms. 정은희 (Eunhee Jeong)

### Problem
HWR (Handwriting Recognition) module được triển khai trên ABT (Human Interface Device) yêu cầu chip chuyên dụng từ nhà cung cấp. Tình trạng thiếu chip (chip shortage) và hiệu suất HWR kém trên ABT khiến OEM yêu cầu thiết kế thay thế. Cần di chuyển HWR software từ ABT sang Main Unit, đảm bảo touch data delivered trong 40ms và không mất dữ liệu.

### Solution
So sánh 2 phương án thiết kế:

- **Design 1 — HWRService microservice:** Tạo service riêng trên IVI Framework, giao tiếp với InputService qua KIPC. Dễ cô lập nhưng latency có thể lên 100ms khi CPU > 80%, nguy cơ mất touch event.

- **Design 2 — Integrate HWRManager into InputService (được chọn):** `HWRManager` chạy như thread trong InputService. Dùng **Mediator Pattern** (`HWRMediator`) để kết nối `InputComponent` và `HWRManager`. `Proxy` wraps `libHwrEngine` → có thể test mà không cần SDK từ Hanwang. Nhận touch trực tiếp từ InputService (ưu tiên KIPC cao nhất → đảm bảo ≤20ms).

### Outcome
| Tiêu chí | Design 1 | Design 2 |
|---|---|---|
| Performance (≤40ms) | Mid (worst 100ms) | **High** (không latency) |
| Reliability (no touch lost) | Mid | **High** |
| Testability | Mid | Mid |
| Maintainability | High | High |

Proxy Pattern cho phép test toàn bộ functional requirements mà không cần HWR core engine từ Hanwang từ đầu dự án.

--- 

## Topic 13 — Design of Commonization for Communication Monitoring Function

**Tác giả:** Nguyen Trung Hieu (hieu5.nguyen) | **Dự án:** Mercedes-Benz BR167M2 (Classic AUTOSAR, MCU) | **Năm:** 2024  
**Reviewer:** Sangkyu.hwangbo

### Problem
Trong BR167M2 project (Classic AUTOSAR), nhiều SWC (DRCAMManager, PowerManager, CddFan, CddSysB) đều có **10ms timing runnable riêng** để monitor COM mode từ BSW COM service. Kết quả: 4 runnable lặp lại cùng thao tác → CPU usage 0.018%, thứ tự thực thi không ổn định. Trong worst case (suspend runnable xen vào), fan có thể tắt trước nhưng camera/display không tắt kịp → nguy cơ quá nhiệt phần cứng.

### Solution
**Proposal 2 — Delegating Tasks to Individual SWC (được chọn):** Thêm component `ComMonitor` duy nhất có 1 timing runnable (10ms). `ComMonitor` monitor COM mode, khi thay đổi → gọi **server runnable** của từng SWC qua RTE interface (client-server pattern). Mỗi SWC tự xử lý logic thiết bị của mình (modularity giữ nguyên). `ComMonitor` không biết logic thiết bị → scalable, dễ thêm SWC mới.

vs. Proposal 1 (Directly in ComMonitor): tập trung toàn bộ logic trong 1 class → maintainability tốt nhưng vi phạm modularity, khó test riêng.

### Outcome
| Tiêu chí | Before | After (Proposal 2) |
|---|---|---|
| CPU usage (monitoring) | 0.018% (4 runnables) | **0.005%** (1 runnable) |
| Cải thiện performance | — | **72% giảm** (3.6x) |
| Execution stability | Không ổn định, thứ tự ngẫu nhiên | **Stable ~1ms/event** |
| Thêm SWC mới | ~2 tuần/SWC | Chỉ cần implement action runnable |

Unpredicted case (suspend runnable) được giải quyết: hành động điều khiển thiết bị luôn thực thi tuần tự và ngay lập tức trong 1 chu kỳ.

---

## Topic 14 — Improve Carplay Reconnection Time After Cold Boot in Nissan Project

**Tác giả:** Hoan Ngoc Tran | **Dự án:** Nissan DA2 (AAOS — Android Automotive OS) | **Năm:** 2024

### Problem
Carplay reconnect time sau cold boot là **61 giây**, trong đó Android Framework mất **53 giây** để xử lý broadcast trước khi deliver tới Carplay Service. Nguyên nhân: cold boot → `BOOT_COMPLETE` broadcast → nhiều app xử lý lâu → queue broadcast bị chặn. Carplay Certificate của Apple yêu cầu reconnect trong vòng **10 giây** → không đạt.

### Solution
So sánh 2 phương án thay thế Broadcast:

- **Proposal 1 — Database (ContentProvider):** HMI write vào database → Carplay đọc qua observer. Tránh broadcast queue nhưng bảo mật thấp (3rd-party app có thể write vào DB), không phải chuẩn IPC.

- **Proposal 2 — Binder (IPC) [được chọn]:** Thay `HMICommunication` bằng `HMICommunicationHandler` + `IBinder` (Message Queue Handler). HMI bind trực tiếp tới Carplay Service. Binder hỗ trợ `getCallingPackage()` → chặn unwanted app. Không qua queue → không bị chặn bởi app khác.

### Outcome
| Tiêu chí | Current (Broadcast) | Proposal 2 (Binder) |
|---|---|---|
| Reconnect time (cold boot) | **61 giây** | **~9 giây** ✅ |
| Apple Carplay KPI ≤10s | Không đạt ❌ | Đạt ✅ |
| Security (callingPackage) | Không có | Có ✅ |
| Communication Standard | Broadcast (Android) | Binder (Android IPC) |

Binder IPC là cơ chế chuẩn của Android, loại bỏ 53s delay do broadcast queue congestion sau cold boot.

---

## Topic 15 — Architecture Improvement for CAN Dispatching Function of Micom Manager

**Tác giả:** hoang.cao | **Dự án:** GM Info3.5 AVN | **Năm:** không ghi rõ

### Problem
`CANHandler` trong Micom Manager (MgrMcm) dùng `switch-case` để route CAN frames từ Micom SPI → các manager khác qua IPC. Toàn bộ logic cho tất cả CAN frames nằm trong 1 class: khó maintain, khó extend (mỗi thay đổi phải chạm vào switch-case chính), không reusable cho dự án khác.

### Solution
So sánh 2 phương án:

- **Proposal 1 — Observer Pattern (được chọn):** `CANDatapool` đóng vai Subject. Mỗi manager tự implement `update()` và đăng ký/hủy đăng ký frame ID **dynamically**. Logic của từng manager di chuyển hoàn toàn vào class riêng. Không cần maintain fixed list trong MgrMcm.

- **Proposal 2 — Chain of Responsibility:** Handler classes nối nhau xử lý frame tuần tự. Vẫn cần maintain fixed frame ID list. Mỗi frame phải đi qua toàn bộ chain → kém hiệu quả hơn. Logic có thể bị duplicate khi 1 frame cần nhiều handler.

### Outcome
| QA | Current | Proposal 1 | Proposal 2 |
|---|---|---|---|
| Modifiability | Low | **High** | Medium |
| Maintainability | Low | **High** | High |
| Reusability | Low | **High** | High |

Proposal 1 chưa áp dụng được cho Info3.5 (production) nhưng được đề xuất cho **dự án GM AVN tiếp theo** với kiến trúc tương tự.

--- 

## Topic 16 — Architecture Design for NGeCall Supports Maintenance and Expansion

**Tác giả:** hoang2.nguyen | **Dự án:** BMW-WAVE (NGeCall application) | **Năm:** 2024

### Problem
`eCallNGProcess` là god-class với **200+ functions, 10,000+ LOC**, cyclomatic complexity rất cao. Mọi thay đổi (region mới, tiêu chuẩn eCall mới) đều phải chạm vào class này → rủi ro break working features cao. NGeCall cần mở rộng sang ICONICC, Motorrad-ICONICC → không reusable.

### Solution
Tách `eCallNGProcess` thành các class nhỏ theo chức năng: `State`, `Trigger`, `SelfTest`, `Timer`, `Diag`, `Call`, `Data`, `Log`.

- **Proposal 1 — Mediator Pattern:** `eCallNGApplication` làm mediator, điều phối method calls giữa các class. Khi thêm region mới: chỉ thêm class mới + sửa mediator. Nhược điểm: mediator có thể trở thành god-object theo thời gian.

- **Proposal 2 — Interface Classes (được chọn):** Mỗi class expose interface riêng. `eCallNGApplication` khởi tạo đúng implementation dựa trên region. Các class gọi nhau trực tiếp qua interface pointer. Hỗ trợ **Unit Test mock** tốt hơn, Maintainability cao hơn (không cần check region trong mediator).

### Outcome
| QA | Proposal 1 (Mediator) | Proposal 2 (Interface) |
|---|---|---|
| Modifiability | High | **High** |
| Maintainability | Medium (mediator phình to) | **High** |
| Reusability | High | **High** |
| Unit Test | Khó | **Dễ mock** |

Giải pháp áp dụng ngay cho NGeCall đang development, dự kiến tái sử dụng cho ICONICC và Motorrad-ICONICC.

---

## Topic 17 — Design Asset and Persistent Storage Management on Carplay Ultra

**Tác giả:** Huy Quang Le (huy3.le) | **Dự án:** ConnectWide (Carplay Ultra) | **Năm:** 2025  
**Reviewer:** Mr. Kim Tai Ho

### Problem

Carplay Ultra (next-gen CarPlay) yêu cầu quản lý **Asset Sessions** (graphical assets từ iPhone → file system xe) và **Persistent Storage** (trạng thái Local UI giữa ignition cycles) trong Carplay Engine layer. Cần thiết kế API/callback giữa Client ↔ Engine ↔ Display Plug-in (Apple library) đảm bảo: load/foreground asset trong ≤100ms (KPI Apple certification), reusability ≤5% code change khi thêm requirement mới.

### Solution

So sánh 2 phương án:

- **Proposal 1 — Manage AssetSession Objects (OOP):** `AssetSessionManager` Java quản lý nhiều `AssetSession`, mỗi instance có `PersistentStorageManager` riêng. Logic tập trung ở Java layer. Dễ debug (Android Studio), clean OOP. Nhưng logic AssetSession bị duplicate giữa Client và Engine → tăng coupling.

- **Proposal 2 — Singleton Pattern (được chọn):** `AssetSessionManager` Singleton + `PersistentStorageManager` Singleton trong Engine. Engine chỉ là communication bridge, route callbacks qua `assetSessionID`. `PersistentStorageManager` xử lý ở C++ layer (tránh Java-Native overhead). Client không cần biết logic AssetSession → giảm dependency.

### Outcome
| Tiêu chí | Proposal 1 | Proposal 2 |
|---|---|---|
| Performance (≤100ms) | High | **High** |
| Reusability (≤5% change) | Medium | **High** |
| Maintainability | High | Medium (thread-safety phức tạp hơn) |

**Proposal 2 được chọn** cho ConnectWide vì OEM ưu tiên reusability/scalability của Carplay Engine cho các dự án tương lai. Khi OEM thay đổi requirement, sửa Singleton Engine không ảnh hưởng Client.

---

## Topic 18 — Improve Internal Design of Network Management Handler

**Tác giả:** Chinh.nguyen | **Dự án:** VW Cockpit (ICAS3 EUGP, Adaptive AUTOSAR, SAFE partition) | **Năm:** 2024

### Problem

`Application` class của NMH làm nhiều việc: khởi tạo tất cả services, subscribe/unsubscribe, xử lý logic NM PDU update và network mode change. Tất cả logic nằm trong 1 file → vi phạm SRP. Các class giao tiếp nhau qua **callback functions** → execution flow không tuyến tính, khó debug. File > 500 LOC, vi phạm coding convention dự án.

### Solution

Cả hai proposals đều: (1) tách `Application` thành `NMService` + `NMChannelService`, (2) xóa callback functions.

- **Proposal 1 — Interface:** `IService` interface, inject references trực tiếp vào `NmHandlerReceiver` và `DiagnosticConversation`. Execution flow tuyến tính.

- **Proposal 2 — Factory Pattern (được chọn):** Thêm `ServiceFactory` để tách object creation logic ra khỏi Application. Application chỉ orchestrate; factory tạo và tìm proxy instances. Cleaner Single Responsibility cho từng class, dễ manage object lifecycle.

### Outcome
| QA | Current | Proposal 2 (Factory) |
|---|---|---|
| Maintainability | Low (callback maze) | **High** |
| Modifiability | Low (monolith) | **High** |
| Extensibility | Low | **High** |
| LOC/file | > 500 | **< 500** ✅ |

Factory Pattern tách biệt creation logic khỏi business logic, execution flow tuyến tính → dễ debug và thêm service mới mà không ảnh hưởng client code.

---

## Topic 19 — Design of Commonization for Wifi Manager

**Tác giả:** manh2.tran | **Dự án:** Common (VW Cockpit, BMW ICON) | **Năm:** 2024–2025

### Problem
Wifi Manager mỗi dự án (VW Cockpit, BMW ICON) triển khai riêng biệt, dẫn đến code duplication, tightly coupled components (`WifiManServer` làm dependency hub cho tất cả controllers), khó scale khi thêm variant mới. Thay đổi ở 1 module cascade ra nhiều module khác.

### Solution
Thiết kế lại theo **Chain of Responsibility** với `RootController` làm entry point điều phối events qua chuỗi sub-controllers (`WlanServiceController`, `NicController`, `WpaController`, v.v.). Controllers không gọi nhau trực tiếp, chỉ post events lên RootController.

- **Proposal 1 — Chain of Responsibility + Singleton:** Controller chain thẳng, không factory. Reusability cao nhưng Testability thấp (global state, hidden dependencies khó mock).

- **Proposal 2 — Chain of Responsibility + Abstract Factory (được chọn):** Thêm `IWanServiceControllerFactory` và `IDynamicIPClientControllerFactory`. Factory tạo service controllers (`RsiServiceController`, `SomeIPServiceController`) và IP controllers (`DHClientController`, `DHCPCCDController`) phù hợp từng variant. Dependencies có thể mock → Testability cải thiện.

### Outcome
| QA | Proposal 1 | Proposal 2 |
|---|---|---|
| Reusability | High | **High** |
| Extensibility | Medium | **High** |
| Testability | Low | **Medium** |
| Modifiability | Medium | Medium |

Common controller hierarchy có thể tái sử dụng cho cả VW Cockpit và BMW ICON, variant-specific logic tách biệt qua Factory pattern.

---

## Topic 20 — BMW ICON — BT Manager

**Tác giả:** Nhan.ngo | **Dự án:** BMW ICONICC | **Năm:** 2022

### Problem

BMW ICONICC cần thiết kế mới BT Manager từ đầu — service quản lý Bluetooth (Classic + BLE) cho ICON system. Module phải xử lý cả BTLE connection, peripheral/central mode, BLE scanning/advertising, GATT services/characteristics, và giao tiếp qua SOME/IP với các modules khác (DiagManager, Car Sharing, v.v.).

### Solution

Kiến trúc layered rõ ràng:

- **SOME/IP Server:** expose API theo OEM format, nhận request từ external components
- **BTManager:** core controller, bridge giữa SOME/IP layer và BT-Stack (Qualcomm vendor library)
- Các module chuyên trách: `Discovery` (scan/discovery), `Advertising` (peripheral mode, visibility), `Connection` (establish/maintain), `GATT DB` (services/characteristics management)
- **BT-Stack:** C API library từ chip vendor (Qualcomm), async response model
- Giao tiếp: request via SOME/IP (đồng bộ), response từ BT-Stack (async callback)

### Outcome
Document ở giai đoạn early design (v1.03, nhiều phần TBD). Thiết kế kiến trúc rõ ràng về **separation of concerns**: mỗi BT feature (scan, advertise, connect, GATT) có module riêng. SOME/IP làm transport layer chuẩn → compatible với AUTOSAR/ICON ecosystem. Performance constraint: mỗi request phải có response (success/fail/timeout).

*Lưu ý: Document ở trạng thái initial design, một số phần internal interfaces và algorithm design chưa hoàn thiện.*

---

## Topic 21 — FA 2022 Change Mode Sequence Improvement

**Tác giả:** Chuong.nguyen | **Dự án:** (HMI IVI, animation system) | **Năm:** 2022  
**Supervised by:** By.kim

### Problem

Hệ thống change mode animation có 3 elements: Toggling, Tube Layout, Tube Content. Thiết kế hiện tại: mỗi service vừa chạy animation, vừa notify cho nhau khi xong, vừa check sequence finish. Các module phụ thuộc lẫn nhau → khó extend (thêm element mới phải sửa nhiều chỗ), khó maintain sequence khi bug xảy ra. OEM yêu cầu hỗ trợ cả "fast change mode" (animation bị interrupt giữa chừng) và "normal change mode".

### Solution

- **Proposal 1 — Common Change mode Architecture (CCA) — được chọn:** Tạo common abstract class, mỗi element kế thừa và implement theo common API. Một ChangeMode service điều phối. Phần tử không biết về nhau → zero coupling. Thêm element mới: chỉ cần extend common class.

- **Proposal 2 — Monitor Change mode Architecture (MCA):** `ChangeModeMonitor` làm trung tâm, tất cả elements depend vào monitor. Dễ migrate từ code hiện tại, nhưng khi nhiều elements, `ChangeModeMonitor` phình to và phức tạp.

### Outcome

| Tiêu chí | CCA (Proposal 1) | MCA (Proposal 2) |
|---|---|---|
| Extensibility | ✅ Tốt hơn | Khó hơn |
| Developer thân thiện | ✅ Tốt hơn | Trung bình |
| Code update ít | Nhiều code mới | ✅ Ít hơn |
| Zero coupling | ✅ Có | Phụ thuộc Monitor |

**Proposal 1 (CCA) được chọn**: tốt hơn cho cả extensibility lẫn maintainability dài hạn.

---

## Topic 22 — Common Factory Service Design for AVN Virtualization Projects

**Tác giả:** Nguyen2.nguyen | **Dự án:** GM GVM MY24/27, HKMC Connect Wide, BMW RSE27 | **Năm:** 2025  
**Supervised by:** Ahn.Woosuk

### Problem

Factory Service (FS) truyền thống chỉ chạy trong Android VM, giao tiếp qua AIDL với HAL. Trong các dự án AVN virtualization (Hypervisor: QNX+Linux+Android; Container: LXC với Android+ACP), các services cần test nằm rải rác trên nhiều VMs. FS hiện tại không thể access services ngoài Android VM → không reusable across platforms.

### Solution

**Design 2 — SOA (được chọn):**

- **Factory Native Service** (C++) có thể build trên QNX, Linux, Android. Phân vai Host (nhận request từ Inspection Tool, điều phối clients) / Client (xử lý test trong từng VM).
- **Factory Java Service** (Client, Android only): dùng Android API cho display/graphics tests.
- **Core/Variant split:** Core (protocol, framework) reusable 40–60%; Variant (test logic) per-project.
- **IConnection interface** + Strategy: support TCP/IP, UART, SOME/IP, D-Bus.
- **TaskManager** (thread pool): xử lý concurrent requests, sleep khi queue empty.
- **Centralized logging**: mỗi client gửi log về Host dùng reserved command_id 0xFFFFFE.
- **First Boot Strategy**: Factory Host start sớm, tiếp nhận request từ Inspection Tool trước khi tất cả VMs khởi động xong → tối ưu UPH (Units per Hour).

vs. Design 1 (Android AIDL + Inter-VM): chỉ mở rộng FS Android hiện tại, không cross-platform.

### Outcome

| QA | Design 1 | Design 2 (SOA) |
|---|---|---|
| Reusability | Medium | **High (40–60% Core)** |
| Compatibility (QNX/Linux/Android) | Low | **High** |
| Interoperability | Medium | **High (multi-protocol)** |
| Performance (≤20ms overhead) | Medium | **Medium** (thread pool) |

---

## Topic 23 — Design of Commonization for V2X Manager

**Tác giả:** Nguyen Van Si | **Dự án:** JLR-VCM (Vehicle Connectivity Module, embedded Linux, Tiger Platform) | **Năm:** 2025  
**Reviewer:** Seungchul Yi

### Problem

V2X Manager (kế thừa từ BMW project) là 1 Tiger service đơn luồng với **Looper thread duy nhất** xử lý FIFO tất cả messages từ LocationMgr, DiagMgr, ConfigMgr, SomeipMgr. Kết quả: bottleneck nghiêm trọng, throughput chỉ **15.93 MB/s** (yêu cầu ≥20 MB/s). V2X cần xử lý emergency messages trong 10–50ms nhưng bị chặn bởi low-priority messages trước đó. Chưa có common design để reuse sang project mới.

### Solution
3 phương án so sánh:

1. **Multi-threading:** Thêm handler threads vào V2Xmgr hiện tại. Ít RAM, nhưng khó maintain khi nhiều threads, khó cho developer mới.

2. **V2XMgr with LGVF (được chọn):** Tách V2Xmgr thành sub-services (`V2xTigerProxyService`, `V2xConfigService`, `V2xDiagService`, `V2xSecurityService`, `V2xStateService`, `V2xStackProxyService`) quản lý bởi **LGVF framework** (LG Vision Framework). Publish-subscribe via LGVF Mailbox → không block looper. **Core/Variant split** (XML config per project). LGVF chạy trên Tiger, Linux, Windows → cross-platform.

3. **Split multiple Tiger services:** Tách nhỏ quá, mỗi service phải connect thẳng V2X Stack → không reusable, RAM/CPU tăng cao.

### Outcome
| Tiêu chí | Original | Alternative 2 (LGVF) |
|---|---|---|
| Throughput | **15.93 MB/s** ❌ | **92 MB/s ✅** (5.8x) |
| Performance requirement ≥20MB/s | Không đạt | **Đạt** |
| Reusability (core-variant) | Không | **Có** |
| Maintainability | Khó | **Tốt** (per-module folder) |

Đã implement và verify trên VCM board (JLR variant) với folder structure `core/` + `variant/JLR/`.

---

## Topic 24 — Improvement the Factory Service Design for Commonization (Renault/Nissan)

**Tác giả:** Thai.le | **Dự án:** Renault AIVI2 / Nissan CCS | **Năm:** 2024  
**Supervised by:** Eunhee.jeong

### Problem

Factory Service V1.0 chạy trên FactoryOS (MiniOS riêng biệt) → code duplication giữa FactoryOS và Android. V2.0 cải thiện bằng cách chạy FactoryOS trong **Linux Container** trên Android host, nhưng gặp vấn đề: (1) Boot time tuần tự (Android trước, container sau) → UPH thấp: **1st boot 80s, normal 20s**; (2) Critical bug: container fail khi 1st boot → toàn bộ SW phải re-flash (5–7 units/tuần); (3) Tightly coupled với Linux/Android.

### Solution

- **Design 1 — Android HIDL:** Loại bỏ container, thay bằng HIDL service trong Android. Boot nhanh nhưng bị lock vào Android platform → không reusable sang QNX/Linux.

- **Design 2 — CommonAPI (được chọn):** Loại bỏ container, dùng **CommonAPI** (GenIVI middleware framework) để communicate giữa Factory Service và các XXX-Manager services (BT, WiFi, Camera, USB, v.v.). CommonAPI generate stub/proxy code → platform-independent (Android/Linux/QNX), tái sử dụng không cần sửa FS code khi port sang project mới.

### Outcome

| QA | Old FS (Container) | Design 2 (CommonAPI) |
|---|---|---|
| Boot time (1st boot) | 80 giây | **45 giây** (-44%) |
| Boot time (normal) | 20 giây | **12 giây** (-40%) |
| Reliability | Data lost khi power-off | **Data không mất** |
| Platform Independence | Low (Linux-dependent) | **High** |
| Reusability | Low | **High** (Nissan → Renault CDC) |

---

## Topic 25 — [25 FA] Improvement Design of Sport Chrono Module in Porsche E3PA Cluster

**Tác giả:** thai.pham | **Dự án:** Porsche E3PA Cluster HMI | **Năm:** 2025  
**Supervised by:** Ahn.Woosuk

### Problem

Sport Chrono module có 61 issues kể từ tháng 10/2024: 29.5% sai BC page list, 27.9% wrong screen, 23% wrong focus. Root cause: `SportChronoService` là god-class gánh quá nhiều trách nhiệm — state management, event distribution, data management, UI coordination. State được quản lý qua nhiều biến status rời rạc (`isCloseSCAct`, `isLoadNode`, `isPressStop`, ...); logic focus và menu bị scatter trong nhiều Activity class.

### Solution
**Hai nhóm vấn đề, hai cặp proposal:**

**Vấn đề 1 — State Management (PROB-1, 2, 3):**
- Proposal 1: Center State Manager (`SCStateManager`) — centralized nhưng dễ phình to
- **Proposal 2 (được chọn): State Pattern** — `SCStateContext` + `SCScreenState` hierarchy. Mỗi state tự quản lý behavior; `SCMenuManagement` và `SCFocusManager` tách riêng → easy extend, localized changes

**Vấn đề 2 — Data Exchange (PROB-4, static variables):**
- **Proposal 1 (được chọn): Separated Managers** — `ChronoDataManager` + `ChronoEventManager` + Observer Pattern (IBapControlObserver, IChronoDataObserver, IChronoEventObserver, IChronoTimerObserver) → modular, clear separation
- Proposal 2: Central Manager tại `SportChronoService` → risk service bloat

### Outcome

| Chỉ tiêu | Current | Final Solution |
|---|---|---|
| Maintainability | Khó trace bug, side effect khi sửa | State pattern → isolated behavior per state |
| Modifiability | God-class, phải sửa nhiều nơi | Thêm state mới không ảnh hưởng context |
| Data coupling | Static variables dùng chung toàn hệ thống | Observer-based, decoupled producers/consumers |

---

## Topic 26 — New Design for Synchronizing "Bluetooth Remote Device Information" in RSE Applications (BMW RSE27)

**Tác giả:** thuong.le | **Dự án:** BMW RSE27 | **Năm:** 2025  
**Reviewed by:** sanghyup.lee

### Problem

BMW RSE27 có kiến trúc dual-display: 1 main CPU chạy Android OS, 2 màn hình độc lập với 2 Bluetooth chipsets riêng biệt. Ứng dụng `BluetoothSettings` cần chạy trên cả 2 màn hình (User 1 và User 2) và phải **hiển thị thông tin device đồng nhất** (tên, battery, trạng thái kết nối). Các action (connect/pair/remove) từ một màn hình phải phản ánh ngay trên màn hình còn lại.

### Solution

- **Proposal 1 — IPC Service (Dual Adapter):** 2 `BluetoothSyncService` chạy độc lập, đồng bộ qua IPC cross-user. IPC latency 10–50ms, phức tạp với nhiều edge case, risk race condition.

- **Proposal 2 — Shared Adapter (được chọn):** Dùng **chỉ 1 BluetoothAdapter** (của Android User 2) làm nguồn duy nhất cho cả 2 ứng dụng. Broadcast event từ User 2 Context → cả 2 app đều nhận, đảm bảo tự động đồng bộ. Thêm failover mechanism: health check khi startup, nếu primary adapter lỗi thì switch sang backup adapter.

  Classes chính: `RemoteControllerManager`, `BluetoothAdapterProvider` (xử lý switching), `CurrentUserBluetoothReceiver`, `CrossUserBluetoothReceiver`, `RemoteControllerUIManager`.

### Outcome

| Chỉ tiêu | Proposal 1 (IPC) | Proposal 2 (Shared Adapter) |
|---|---|---|
| GetConnectedStatus execution | **15.88ms** | **6.22ms** (-61%) |
| Data sync | Manual, conflict risk | Automatic single source of truth |
| Button response | 10–50ms IPC overhead | <100ms (đạt target) |
| Availability | Dual chipsets (full HW redundancy) | Failover mode nếu primary adapter fail |

---

## Topic 27 — ECall Application Skeleton Architecture

**Tác giả:** Tien.Nguyen | **Dự án:** BMW ICONNIC / Honda TSU / Toyota DCM 24LC | **Năm:** 2023  
**Reviewed by:** Joon.Namkoong

### Problem

ECall Application là module thiết yếu trong hầu hết các dự án telematics. Thiết kế hiện tại có 2 vấn đề chính:
1. **Design 1 (All-independent):** Mỗi region (EU, PSAP, ERA, BMW, GSC) có process riêng → BMW ICONNIC chạy **5 processes**; code trùng lặp; 1 bug phải fix tại nhiều nơi.
2. **Design 2 (State Machine không tốt):** State classes nắm `App` object, không handle business logic → `ECallApplication` god-class, phức tạp khi extend.

### Solution

**New ECall Application Skeleton** (Design 2) với 2 cải tiến:

1. **Service Wrapper Block:** Tách biệt communication giữa app và service layer. Mỗi service được wrap bởi 1 adapter class (`TelephoneManagerAdapter`, `AudioManagerAdapter`, `VehicleManagerAdapter`, v.v.) → reuse, chỉ cần register 1 receiver thay vì nhiều.

2. **4-Block Structure:**
   - `AppMain`: quản lý lifecycle, forward events
   - `ServiceWrapper`: wrap service APIs, handle reconnect khi service chết
   - `Processor` (ProcessorManager + ProcessorFeatureClass): business logic per feature, isolated
   - `Utils`: Logger, ECallHandler, common functions

### Outcome

| QA | BMW ICONNIC (Design 1) | Honda TSU / Toyota (Design 2) |
|---|---|---|
| Số processes | **5 processes** | **2 processes** |
| Init time | ~900ms | **<300ms** |
| Memory (USS total) | ~11,420 KB | ~3,100 KB (-73%) |
| Bug fix scope | Duplicate 4–5 repos | Localized 1 module |

---

## Topic 28 — Optimizing Logic and Performance in the Data Sharing Service (MgrDS)

**Tác giả:** tuan7.nguyen | **Dự án:** FPK Project (Cheetah platform) | **Năm:** 2025

### Problem

`MgrDS` (Manager Data Sharing) app download ảnh qua Ethernet từ Head Unit. Thiết kế cũ có 3 vấn đề:
1. **Multi-threading mỗi request:** Mỗi request tạo 1 thread mới → race condition, deadlock, memory leak.
2. **God-class `CDSUploadThread`:** Vừa handle message, vừa quản lý connection, vừa download.
3. **Modify tất cả ảnh:** Kể cả ảnh không cần thay đổi → overhead không cần thiết.

**Kết quả đo thực tế:** Trung bình **3.706 giây/request**, max 6.03 giây. Case #13 sai ảnh do thread sync issue, case #15 không tìm ra root cause.

### Solution

- **Proposal 1 (Thread Pool):** Thêm `CDSThreadPool` class, reuse threads → avg 0.4425s.
- **Proposal 2 (Persistent Connection, được chọn):** Tách `TLSConnection` class riêng, kết nối 1 lần khi startup và giữ alive (keep-alive). `CDSUploadThread` chỉ làm orchestration. Chỉ modify ảnh khi thực sự cần.

### Outcome

| Chỉ tiêu | Current | Proposal 1 (Thread Pool) | Proposal 2 (Persistent) |
|---|---|---|---|
| Avg download time | 3.706 s | 0.4425 s | **0.169 s** |
| Cải thiện | baseline | 8.4× | **21.9×** |
| Stability | Thread sync bugs | Cải thiện | Không còn connection error |

---

## Topic 29 — KIPC Architecture Improvement

**Tác giả:** Tran Anh Kiet | **Dự án:** VW ICAS3/MIB3 GP Project | **Năm:** 2024  
**Reviewed by:** 황보상규 (sangkyu.hwangbo)

### Problem

KIPC (Kernel IPC) library của VW platform chỉ hỗ trợ raw byte-stream send/receive cơ bản. Developer phải tự handle: (1) phân tích và parse raw data; (2) tự implement cơ chế synchronous/asynchronous; (3) thỏa thuận format message qua Word/Excel giữa các team. Kết quả: code bị fragment, khó scale khi thêm message mới, nguy cơ lỗi parse cao.

### Solution

- **Solution 1 — Expand Library:** 2 thư viện riêng: `MessageProcessing Library` (serialize/deserialize dựa vào Message Description File) + `IPC Library` (manage async/sync/one-way patterns). Developer phải tự phối hợp 2 thư viện.

- **Solution 2 — Message Encapsulation (được chọn):** Tập trung vào **Core Library** + **Abstraction Layer** chứa auto-generated Interfaces. Message Description File (JSON) → code generator tạo Interface class với API/handler method cho từng message ID. Developer gọi API typed, không phải deal với raw bytes. Core Library quản lý serialize/deserialize và communication patterns (sync/async/one-way) tập trung.

### Outcome

| QA | Solution 1 | Solution 2 (Message Encapsulation) |
|---|---|---|
| Scalability (thêm 10 messages) | 10h (manual callback register) | **1h** (re-generate interfaces) |
| Usability | Medium (2 libraries) | **High** (typed API, auto-generated) |
| Maintainability | Good per-library | **Centralized** (1 core library) |

---

## Topic 30 — New Synchronization Partition Design for Enhanced Software Update Robustness

**Tác giả:** Duc.phan | **Dự án:** Toyota 26BEV (NAD/DCM) | **Năm:** 2025

### Problem

Toyota 26BEV dùng **dual-bank update** (Bank-A active, Bank-B inactive) trên NAND flash. Sau khi update thành công và activate SW mới trên Bank-A, Bank-B vẫn chứa SW phiên bản cũ. Khi BSP reboot và chọn nhầm bank → hệ thống khởi động với SW cũ: mất tính năng mới, data incompatibility, security vulnerabilities.

Vấn đề được xác nhận trong Toyota 26BEV (JIRA: TMCBEV-2644) và BMW ICONN project (ICONSD-119799).

### Solution

**Synchronization Partition** — module mới trong `UpdateHandler`, được trigger sau khi OTA master báo kết thúc update session (khi clear reprogramming settings), chạy background để sync active → inactive bank.

- **Proposal 1 (Nandwrite):** Đọc toàn bộ source partition → lưu temporary file → dùng `nandwrite` tool để ghi. Ưu: built-in bad block management. Nhược: RAM lớn, nhiều I/O.

- **Proposal 2 (BufferWrite, được chọn):** Đọc từng buffer từ source partition → ghi trực tiếp vào target partition. Custom bad block management. Nhược: complex hơn. Ưu: memory footprint thấp hơn, ít CPU.

Gồm 3 classes mới: `SyncPartitionManager`, `SyncPartitionParser` (đọc XML config trong SW package), `CVarFlash` (low-level NAND I/O).

### Outcome

| Partition | Proposal 1 (Nandwrite) | Proposal 2 (BufferWrite) | CPU |
|---|---|---|---|
| System (411 MB) | 90,169 ms | **71,695 ms** | 55% → **46%** |
| Total (7 partitions) | 99,663 ms | **79,075 ms** (-20.7%) | 55% → **46%** |
| Sau khi reboot sai bank | SW version khác nhau | **SW version giống nhau** |

---

## Topic 31 — Bluetooth Low Energy Commonization Architecture

**Tác giả:** manh.nguyen | **Dự án:** BMW ICONICC | **Năm:** 2024

### Problem

BMW ICONICC cần support nhiều BLE chipset stack (Qualcomm Synergy, Infineon, v.v.). Thiết kế hiện tại tập trung tất cả BLE logic trong `BT manager service` → vi phạm Open-Closed Principle: thêm stack mới phải sửa code hiện tại → risk side effect, khó maintain, không reuse được.

### Solution

- **Proposal 1 — Façade Pattern:** `FaçadeAdaptorClass` wrap multiple BLE stacks. Common logic trong BTManagerService. Nhược: groups all stacks trong 1 component → khó isolate, tăng complexity.

- **Proposal 2 — Strategy Pattern (được chọn):** Refactor BLE manager thành abstract class. Mỗi BLE stack có adaptor class riêng kế thừa abstract service class:
  - `DiscoveryService` → `BmwDiscoveryAdaptor`, `VWDiscoveryAdaptor`
  - `AdvertisingService` → `BmwAdvertisingAdaptor`, `VWAdvertisingAdaptor`
  - `ConnectionService` → `BmwConnectionAdaptor`, `VWConnectionAdaptor`
  - `GattDBService` → `BmwGattDBAdaptor`, `VWGattDBAdaptor`

  Thêm **Adapter Pattern** để remove platform dependency (NeoFramework/BMW ↔ Tiger Platform/Telematics ↔ Android/AVN).

### Outcome

| QA | Current | Proposal 1 (Façade) | Proposal 2 (Strategy) |
|---|---|---|---|
| Extensibility | No (single-stack) | High | **High + isolated** |
| Maintainability | High (single-stack) | Medium (complex facade) | **High (SRP per class)** |
| Reusability | No | High | **High** |

---

## Topic 32 — Voice Recognition Common Service (JLR P-IVI)

**Tác giả:** Tong Tran Hoang De | **Dự án:** JLR Proteus IVI (QNX-based) | **Năm:** 2025  
**Supervised by:** Ms. Eunhee Jeong

### Problem

P-IVI hỗ trợ 3 VR engine: Cerence (DDFW), Alexa, TmallGenie — mỗi engine chạy trong process riêng (`SpeechService`, `AlexaService`, `TmallService`). Vấn đề: (1) Memory/CPU duplicated per process; (2) Audio focus không quản lý tập trung → engine "steal" focus của nhau gây glitch; (3) Thêm VR engine mới cần tạo service mới hoàn toàn.

### Solution
- **Alternative 1 — Centralized IPC:** Gộp IPC package, nhưng giữ `AlexaService`/`CerenceService` riêng → reuse IPC nhưng handler logic vẫn duplicate.

- **Alternative 2 — Centralized Manager + Adapter Pattern (được chọn):** Single `VRService` với common `VoiceEngine` interface. `CerenceAdapter` và `AlexaAdapter` implement interface → `<Feature>Handler` classes xử lý chung logic cho tất cả engine. Thêm engine mới = thêm Adapter class.

### Outcome

| Chỉ tiêu | Before (2 services) | After (VRService) | Diff |
|---|---|---|---|
| Init time (Alexa+Cerence) | 6.6+3.2 s | **5.4+2.8 s** | -1.6s |
| Memory IDLE | ~13,308 KB | **7,872 KB** | -5,436 KB (-41%) |
| CPU during hotword | ~7+8 = 15% | **5+6 = 11%** | -4% |
| Memory during hotword | ~20,032 KB | **12,253 KB** | -7,779 KB (-39%) |

---

## Topic 33 — Performance Improvement of Home Application (JLR P-IVI)

**Tác giả:** Tran Duc Cong | **Dự án:** JLR P-IVI AVN (QNX) | **Năm:** 2023  
**Supervised by:** Mr. Sanghun Lee (이상훈)

### Problem

Home app của JLR P-IVI là "super app" hiển thị 9–15+ tile động (Navigation, Phone, Media, EV, Climate, v.v.). Thiết kế hiện tại: Service chỉ deploy raw data → HMI View tự process. Vấn đề:

1. Data processing duplicated: cùng data process ở Home tile VÀ feature app → mismatch, waste CPU
2. Home engineer phải cover toàn bộ 15+ features → dễ miss requirement
3. View update mỗi khi data thay đổi, kể cả không cần thiết
4. Tất cả tiles được init với default value kể cả feature không fitted theo car model → waste memory

**Target:** Boot time ≤ 11s, switch view ≤ 500ms, tile show ≤ 300ms sau khi back to Home.

### Solution
- **Proposal 1 — Generalize Home Tiles:** Feature app tự cập nhật tile thông qua API của `HomeService`. Giảm duplicate. Nhược: tiles phải đợi feature app ready → delay UX.

- **Proposal 2 — MVC Architecture (được chọn):** Home được refactor theo MVC: `TileManager` + `BaseController`/`ClimateController` + `BaseModel`/`ClimateModel` + `BaseTile`/`ClimateTile` (QML). Controller layer control khi nào update; lazy-load tile component; không tạo tile cho feature không fitted.

### Outcome

| Chỉ tiêu | Current | Generalize | MVC |
|---|---|---|---|
| Boot time reduction | baseline | -0.34s | **-0.724s** |
| Memory (14 tiles, X760) | 42 MB | 24 MB | **24 MB** (-18 MB) |
| Memory (L462) | 42 MB | — | **36 MB** (-6 MB) |

---

## Topic 34 — Common 3D in P-IVI

**Tác giả:** Truong Quoc Hoang | **Dự án:** JLR P-IVI (QNX) | **Năm:** 2023

### Problem

P-IVI có 3 HMI app (4x4i, Camera, Home) đều dùng cùng 1 3D car model nhưng xử lý riêng biệt:

- Load 3D mỗi app: ~2s (4x4i: 2.09s, Camera: 1.82s, Home: 2.01s)
- CPU 6% per app khi nhận CAN signals liên tục → tổng ~32% (4x4i+Camera+Home)
- Memory: ~838 MB tổng (4x4i: 250MB, Camera: 241MB, Home: 347MB)
- Duplicate logic, synchronization issues, mismatch model giữa apps

### Solution

**Proposal 2.3 — Hybrid Common 3D Service (được chọn):** Tạo một hybrid process `Common 3D` vừa chứa service logic vừa chứa 3D HMI rendering. Apps chỉ gọi API để request visibility/position. Single process → không có cross-process communication overhead.

3D model load 1 lần trong `Common 3D`; `4x4i`, `Camera`, `Home` app không load 3D model riêng nữa. `RequestManager` xử lý priority và collision giữa các request từ các app.

### Outcome

| Chỉ tiêu | Before | After (Common 3D) | Thay đổi |
|---|---|---|---|
| 3D model load total | 5,919 ms | **1,827 ms** | -4,092 ms (-69%) |
| Memory total | 837.6 MB | **788.0 MB** | -49.6 MB |
| CPU (normal, 3 apps) | 10.41% | **6.88%** | -3.53% |
| CPU (CAN update, 3 apps) | ~32% | **~10%** | -22% |

---

## Topic 35 — Alliance Update Manager (AUM) Architecture

**Tác giả:** Thinh Nguyen Van | **Dự án:** Renault/Nissan A-IVI2 | **Năm:** 2022

### Problem

AUM (Alliance Update Manager) quản lý FOTA campaign cho IVI và các ECU ngoài (IVC, Meter, HMD, GW, v.v.). Campaign có thể có nhiều ECU, update tuần tự theo thứ tự định sẵn. Update có nhiều phases (Distribution → Installation → Activation) và có thể bị suspend khi xe ngủ rồi resume lại. Thiết kế sequential đơn giản gặp vấn đề: blocking thread khi đợi ECU response, khó resume, code không tách biệt theo phase.

### Solution

- **Design 1 — Sequential:** Mỗi `UpdateHandler` implement toàn bộ update logic → thread bị block khi polling ECU state; khó resume sau deep sleep.

- **Design 2 — State Machine + Event-Driven (được chọn):** Thêm `FotaStateManager` + `FotaStates` (mỗi state class = 1 phase nhỏ). `ProcessingThread` riêng xử lý events không block main thread. Resume = jump về previous state và tiếp tục. FOTA States: PreDownload → Download → Activation → Rollback/Cancel/Suspend...

### Outcome

| Tiêu chí | Sequential | State Machine |
|---|---|---|
| Thread blocking | Blocked khi wait ECU | **Event-based, idle khi không có event** |
| Suspend/Resume | Split logic phức tạp | **Lưu state, jump về và tiếp tục** |
| Impact khi sửa code | High (re-test toàn flow) | **Low (chỉ test state thay đổi)** |
| Mở rộng feature | Khó | **Thêm state class mới** |

---

## Topic 36 — Navigation Adaptation Layer Improvement

**Tác giả:** Quynh.pham | **Dự án:** VW ICAS3 | **Năm:** 2021

### Problem

Navi-AL là middleware adaptation giữa navi-engine và các service khác (KIPC, RSI, SharedMemory, CommonAPI). Thiết kế gốc: components tight-coupled (KipcProvider gọi trực tiếp ShmProvider, v.v.) → khó maintain; KIPC manager có nhiều thread riêng cho từng KIPC data type → nhiều threads, resource lãng phí.

### Solution

**Message Queue Concept (được chọn so với Observer Pattern):** Tách thành 4 managers riêng biệt (`KipcManager`, `RsiManager`, `ShmManager`, `CommonApiManager`), mỗi manager có message queue riêng. Managers communicate qua message thay vì gọi trực tiếp.

KIPC Manager: Thay vì N threads (1 per data type) → **Thread Pool với 3 threads** xử lý tất cả KIPC data. Khi data đến, dispatch sang thread rảnh nhất.

| Observer vs Message Queue | Điểm mạnh của Message Queue |
|---|---|
| Same thread processing | **Mỗi manager có thread riêng** |
| Event-based | **Message-based, target rõ ràng** |
| Thread-safe khó | **Thread-safe per manager** |

### Outcome
- Loose coupling: managers communicate qua message queue, không gọi nhau trực tiếp
- Thread count giảm: N threads riêng → **3 threads (thread pool)**
- Maintainability: mỗi manager chỉ responsible cho 1 interface type


---

## Topic 37 — Performance Improvement of Broadcast SXM Function

**Tác giả:** Pham Phuong Tuan | **Dự án:** LG VS DCV (Sirius XM) | **Năm:** 2022

### Problem

Sirius XM (SXM) app lấy data từ EMMA framework qua API. Data sync từ EMMA → SXM Service → SXM HMI rất chậm → user thấy blank/loading screen khi mở Super Category List, Category List, Channel List. Target: hiển thị list trong **1 giây**.

Constraints: EMMA framework không có callback khi data thay đổi; LGE không thể can thiệp vào EMMA performance; Cold boot lấy full channel list mất 15–25 giây.

### Solution

**Cache Layer + Partial Cyclic Preload (kết hợp, được chọn):**

1. **Cache Layer:** SXM Service cache data theo loại screen. Khi HMI request → trả cache ngay lập tức; song song fetch mới từ EMMA; nếu data mới khác → push update lên HMI. Cache tự động reset khi channel/profile thay đổi.

2. **Cyclic Preload (1 lần khởi tạo):** Preload data vào cache 1 lần sau cold boot (tránh khoảng 15-25s đầu). Không preload lặp lại để tránh CPU overhead.

| Category List | CPU khi preload | Execute time |
|---|---|---|
| No sleep | 7.37% | 1.6s |
| Sleep 100ms delay | 5.05% | 6.3s |

### Outcome

| Chỉ tiêu | Before | After | 
|---|---|---|
| Category List load time | 0.425 s | **0.001 s** (~425x nhanh hơn) |
| User experience | Blank/loading screen | Hiển thị tức thì từ cache |

---

## Topic 38 — Call Manager Commonization Architecture

**Tác giả:** Nam H. Tran | **Dự án:** LG VS AVN (JLR P-IVI) | **Năm:** 2021  
**Supervised by:** 이상훈 (Mr. Sanghun Lee)

### Problem

"Call Bubble" là widget hiển thị call state trên tất cả HMI apps (Home, Phone, Settings, Notification, Keyboard, Power Apertures). Hiện tại, mỗi HMI app tự subscribe call state từ BT Service và Phone Projection Service → code duplicated, ~5000 issues về mismatch call state giữa các apps, khó thêm feature mới (1 MM/feature).

### Solution

- **Proposal 1 — Virtual Call Handler (VCH):** Home HMI đảm nhận call state management, các app khác subscribe qua Home. Ít side effect. Nhược: thêm load cho Home, vẫn cần integration từng app.

- **Proposal 2 — Call Manager Commonization (được chọn):** Tạo `CallManagerService` riêng, tập trung quản lý call state từ BT Service và Phone Projection Service. Tất cả HMI apps subscribe qua `CallManagerService`. Single source of truth → loại bỏ mismatch.

### Outcome

| Chỉ tiêu | Current (CA) | VCH | Call Manager (CMC) |
|---|---|---|---|
| Home startup | 9,742 ms | 9,741 ms | **9,426 ms** |
| HMI update từ phone | 81–82 ms | 52–81 ms | **41 ms** |
| Settings CPU | 7% | 1% | **0.5%** |
| Settings Memory | 160 MB | 150 MB | **123 MB** |
| Home CPU | 16% | 16.5% | **11%** |
| Feature expansion effort | 1 MM | 0.1 MM | **0.05 MM** |

---

## Topic 39 — Process Communication Manager (PCM) Architecture

**Tác giả:** hai.phan | **Dự án:** VDC (Telematics, JLR) | **Năm:** 2023  
**Reviewed by:** joon.namkoong

### Problem

Trong VDC/Telematics project, mỗi App/Service phải import riêng lẻ library của từng service để communicate → N libraries per process, code duplicated, khó extend. Khi có requirement IPC mới, phải update source code trên toàn bộ processes. Binder IPC 1 MB bandwidth limit gây issues khi data lớn.

### Solution

- **Design 1 — Common Lib:** Merge tất cả service libraries thành 1 "Very Big Common Library" → file count không giảm (nhiều interface riêng per service), library nặng, portability = 0.

- **Design 2 — PCM (Process Communication Manager, được chọn):** Tạo 1 dedicated process `PCM` làm trung gian. Tất cả App/Service chỉ import 1 `PCMLib` nhỏ; PCM serialize/deserialize và route message tới đúng target. Extend IPC method mới = update PCM, không động các process khác.

| Tiêu chí | Common Lib | PCM |
|---|---|---|
| Files/LOC | 2/10 | 8/10 |
| Extensibility | 7/10 | **9/10** |
| Portability | 0/10 | 6/10 |
| Performance | 5/10 | **8/10** |

### Outcome

- Sau khi bỏ 7 libraries khỏi SVT process: startup **-120ms** (822ms → 712ms)
- Khi scale nhiều processes → tổng thời gian giảm đáng kể
- Engineer mới không cần hiểu Binder internals (parcel, Bn/Bp class, bandwidth limit)

---

## Topic 40 — Commonization Design For Power Mode Management

**Tác giả:** Thuong Nguyen | **Dự án:** TCUA (JLR EVA3 Telematics) | **Năm:** 2023

### Problem

`PowerExtService` trong PowerManager xử lý power mode state transitions theo kiểu sequential if-else check. Khi nhiều events overlap trong nhiều transition conditions:

- **O(N×M) complexity**: N events/condition × M total conditions → code bloat, performance kém
- **Code duplication**: cùng event check được copy ở nhiều condition functions
- **Khó mở rộng**: thêm condition mới = update nhiều file, retest toàn bộ logic

### Solution

Tạo component mới `BitMask-Driven Power State Machine` thay thế sequential checking:

- **Design 1 — BitMask-Driven State Machine (được chọn):** Encode tất cả power events thành individual bits của 1 integer (BitSet). State transitions dùng bitwise AND matching với predefined policies. Kết hợp State Pattern + Factory Pattern + Observer Pattern.

- **Design 2 — Strategy Pattern (Data Structure):** Dùng table of `TransitionStrategy` policies, so sánh từng event status → O(N×M), linh hoạt hơn nhưng chậm hơn.

| Tiêu chí | BitMask | Strategy |
|---|---|---|
| Performance | **O(1)**, cache-efficient | O(N×M) |
| Memory | **4–8 bytes** (1 int) | Linear growth |
| Extensibility | Configuration-driven | Flexible but verbose |
| Automotive suitability | ✅ Proven pattern | ✅ |

### Outcome

- Sleep transition KPI: **~337 ms** (đo trên TCUA Debug image)
- Design pass functional verification (state transition 1→5→6→7 theo sequence)
- Reusable: chỉ cần thay đổi configuration (BitMask policies) để apply cho VCM và các dự án Telematics khác
- Loại bỏ redundant code bằng Factory Pattern tạo states từ `StateTemplate` + `StateTransitionPolicy`

---

## Topic 41 — Tổng Hợp: Các Pattern và Xu Hướng Kiến Trúc trong FA Projects

### Các Design Pattern phổ biến nhất

| Pattern | Số topic | Ví dụ |
|---|---|---|
| State Pattern | 8 | Topic 05, 07, 25, 27, 35, 39, 40 |
| Strategy Pattern | 6 | Topic 11, 19, 31, 40 |
| Observer/Pub-Sub | 7 | Topic 01, 15, 25, 38, 40 |
| Factory Pattern | 5 | Topic 18, 22, 40 |
| Chain of Responsibility | 3 | Topic 08, 19 |
| Adapter/Wrapper | 5 | Topic 26, 31, 32, 34 |
| Mediator | 3 | Topic 12, 14, 38 |
| Singleton | 2 | Topic 17 |

### Xu hướng cải thiện performance

| Vấn đề gốc | Giải pháp | Kết quả điển hình |
|---|---|---|
| Sequential processing | Thread Pool / Concurrent | Topic 01: 750ms→22ms; Topic 03: 23.3s→2.85s |
| Binder broadcast overhead | Direct Binder IPC | Topic 14: 61s→9s |
| Duplicate IPC per app | Centralized Manager Service | Topic 38: 81ms→41ms |
| Cache miss / no cache | Cache Layer | Topic 37: 0.425s→0.001s |
| Heavy library per process | PCM / Common Lib | Topic 39: -120ms startup |
| Persistent TLS reconnect | TLS Connection Reuse | Topic 28: 3.7s→0.169s |
| Sequential SW Update partition | BufferWrite | Topic 30: -20.7% |

### Xu hướng commonization

Nhiều project FA tập trung vào **reusability across OEM projects**:
- Topic 19, 22, 23, 24, 31, 32, 34, 38, 40 — đều đề xuất Common Service/Manager có core + variant architecture
- Tiger Framework (LGE platform) là nền tảng chung; FA topics thường extend hoặc refactor Tiger layer components

### Top 5 kết quả ấn tượng nhất

| Topic | Cải thiện |
|---|---|
| 37 — SXM Cache | **425×** faster (0.425s → 0.001s) |
| 01 — Power Manager Thread Pool | **34×** faster (750ms → 22ms) |
| 28 — TLS Connection Reuse | **21.9×** faster (3.706s → 0.169s) |
| 03 — Factory Manager Thread Pool | **8.2×** faster (23.3s → 2.85s) |
| 14 — Carplay Reconnection (Binder IPC) | **6.8×** faster (61s → 9s) |

- Tiger services giao tiếp qua SOME/IP (AUTOSAR) hoặc Binder IPC (Android)
- Tiger 3.0 giới thiệu Service Manager class + Legacy Interface → nhiều topics refactor để compatible
- LGVF (LG Vision Framework) được dùng trong Topic 23 như pub-sub framework trên Tiger

### Phân loại theo dự án

| OEM/Project                 | Số topics | Topics                                 |
| --------------------------- | --------- | -------------------------------------- |
| BMW (ICON, WAVE, RSE27)     | 11        | 04, 09, 10, 16, 20, 22, 26, 27, 30, 31 |
| VW (ICAS3, Cockpit)         | 7         | 05, 08, 12, 18, 19, 29, 36             |
| JLR (P-IVI, VCM, VDC, TCUA) | 8         | 23, 32, 33, 34, 38, 39, 40             |
| Toyota/Honda/Nissan         | 5         | 01, 03, 06, 14, 30                     |
| GM/Porsche/MB/Renault       | 7         | 02, 07, 13, 15, 21, 22, 24, 25, 35     |
