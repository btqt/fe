# Design: eCall SAGA Pattern Demo

**Ngày:** 2026-06-25
**Mục tiêu:** Code demo học SAGA pattern trong hệ thống telematics ô tô, sử dụng eCall/SOS flow làm ví dụ thực tế. Aligned với Android Binder/Looper concepts.

---

## 1. Bối cảnh & Yêu cầu

Hệ thống telematics gồm 3 services:

- **UIService** — người dùng bấm nút SOS, hiển thị trạng thái eCall
- **ECallService** — thực hiện/terminate emergency call tới PSAP
- **MCUService** — nhận tín hiệu airbag, điều khiển hardware

**Vấn đề cần giải quyết:**

1. Nhất quán state giữa UI / eCall-service / MCU trong suốt một SAGA transaction
2. Khi có request priority cao hơn (airbag=10 > SOS=7) interrupt call đang active → compensation (terminate graceful) → start new call
3. Service crash giữa chừng (`binderDied`) → recovery + retry

**Tech stack:** Python, class skeleton with mock/stubs (runnable nhưng không cần real IPC).

---

## 2. Approach: Shared Infrastructure + Strategy Pattern

Một bộ services và EventBus chung. `main.py` chạy cùng scenario với 2 strategy:

```
mode=orchestration  →  OrchestratorSaga điều phối tập trung
mode=choreography   →  Mỗi Handler tự react qua EventBus
```

---

## 3. Architecture Overview

```mermaid
graph TB
    subgraph proc_ui["UIService process"]
        UL[Looper] --> UQ[MessageQueue\npriority-aware]
        UH[UIHandler\nhandle_message]
        UQ --> UH
    end

    subgraph proc_ec["ECallService process"]
        EL[Looper] --> EQ[MessageQueue]
        EH[ECallHandler\nhandle_message]
        EQ --> EH
    end

    subgraph proc_mcu["MCUService process"]
        ML[Looper] --> MQ[MessageQueue]
        MH[MCUHandler\nhandle_message]
        MQ --> MH
    end

    subgraph saga["saga/"]
        OR[OrchestratorSaga\nown Looper + ACK handler]
        CH[ChoreographySaga\nEventBus routing]
    end

    MAIN[main.py\nrun_scenario mode=orchestration#124;choreography]

    MAIN --> OR
    MAIN --> CH
    OR -- "binder.send_message()" --> UH
    OR -- "binder.send_message()" --> EH
    OR -- "binder.send_message()" --> MH
    UH -- "reply_to.send_message(ACK)" --> OR
    EH -- "reply_to.send_message(ACK)" --> OR
    CH --> EB[EventBus\nroutes by subscription]
    EB --> UH
    EB --> EH
    EB --> MH
```

---

## 4. Android Binder/Looper Mapping

| Android Concept  | Demo Implementation                                                 |
| ---------------- | ------------------------------------------------------------------- |
| `Looper`         | Mỗi service chạy trên thread riêng với message loop                 |
| `MessageQueue`   | Priority heap: `(-priority, timestamp)` → airbag chen hàng          |
| `Handler`        | `handle_message()` chạy trên Looper thread của service              |
| `Message`        | `what=EventType`, `priority`, `saga_id`, `obj`, `reply_to`          |
| `IBinder`        | Reference tới remote Handler; expose `send_message()`, `is_alive()` |
| `linkToDeath`    | `IBinder.link_to_death(recipient: DeathRecipient)`                  |
| `binderDied`     | `DeathRecipient.binder_died(service_name)` callback                 |
| `ServiceManager` | Singleton registry: `add_service()`, `get_service()`                |

---

## 5. Core Components

### 5.1 Message & Event Types

```python
@dataclass
class Message:
    what: EventType          # SOS_REQUESTED, AIRBAG_TRIGGERED, INITIATE_CALL,
                             # TERMINATE_CALL, CALL_STARTED, CALL_TERMINATED,
                             # UPDATE_STATUS, ACK_*, PING, SAGA_FAILED...
    priority: int            # 1=low … 10=critical (airbag=10, SOS=7)
    saga_id: str             # UUID — trace toàn bộ 1 SAGA transaction
    obj: dict                # flexible payload
    reply_to: Handler | None # ACK sẽ gửi về Handler này
    timestamp: float         # for FIFO ordering within same priority
```

### 5.2 Looper & MessageQueue

```python
class MessageQueue:
    # Thread-safe priority heap: key = (-priority, timestamp)
    def put(msg: Message) -> None
    def get() -> Message          # blocking
    def get_nowait() -> Message | None
    def peek_priority() -> int

class Looper:
    queue: MessageQueue
    def loop() -> None            # while True: handler.dispatch(queue.get())
    def quit() -> None
    def prepare() -> None         # bind Looper to current thread
```

### 5.3 Handler & IBinder

```python
class Handler:
    looper: Looper
    def send_message(msg: Message) -> None   # enqueue vào looper.queue
    def handle_message(msg: Message) -> None # abstract

class IBinder:
    _handler: Handler
    _death_recipients: list[DeathRecipient]
    def link_to_death(recipient: DeathRecipient) -> None
    def unlink_to_death(recipient: DeathRecipient) -> None
    def send_message(msg: Message) -> None
    def is_alive() -> bool
    def simulate_death() -> None  # notify tất cả recipients

class DeathRecipient(Protocol):
    def binder_died(service_name: str) -> None
```

### 5.4 ServiceManager

```python
class ServiceManager:  # Singleton
    def add_service(name: str, handler: Handler) -> IBinder
    def get_service(name: str) -> IBinder | None
    def remove_service(name: str) -> None
```

### 5.5 Base Service

```python
class BaseService(Handler):
    local_state: ECallState  # IDLE | SOS_ACTIVE | ECALL_ACTIVE | TERMINATED
    saga_id: str | None      # SAGA đang active
    priority: int            # priority của SAGA hiện tại

    def handle_message(msg: Message) -> None  # abstract
    def get_state() -> ECallState
    def _send_ack(msg: Message, success: bool) -> None
```

---

## 6. Service Lifecycle

### Startup

```mermaid
sequenceDiagram
    participant SVC as ECallService
    participant SM as ServiceManager
    participant OR as OrchestratorSaga (client)

    SVC->>SVC: Looper.prepare()
    SVC->>SVC: ECallHandler(looper)
    SVC->>SM: add_service("ecall", handler) -> IBinder
    OR->>SM: get_service("ecall") -> IBinder
    OR->>OR: binder.link_to_death(self)
    OR->>SVC: send_message(PING, reply_to=orch_handler)
    SVC->>OR: ACK_PING (service sẵn sàng)
```

### binderDied + Recovery

```mermaid
sequenceDiagram
    participant OR as OrchestratorSaga
    participant SM as ServiceManager
    participant SVC as ECallService

    SVC--xOR: simulate_death() → binder_died("ecall")
    OR->>OR: SAGA → COMPENSATING<br/>log(saga_id, step)

    loop Exponential backoff
        OR->>SM: get_service("ecall")
        SM-->>OR: None
        OR->>OR: sleep(backoff)<br/>backoff = min(backoff*2, 30s)
    end

    SM-->>OR: new IBinder
    OR->>OR: link_to_death(new_binder)<br/>restart SAGA
```

---

## 7. SAGA Flows

### 7.1 Orchestration — Happy Path (SOS)

```mermaid
sequenceDiagram
    participant USER as User (SOS)
    participant UI as UIService
    participant OR as OrchestratorSaga
    participant EC as ECallService
    participant MCU as MCUService

    USER->>UI: press SOS
    UI->>OR: send_message(SOS_REQUESTED, priority=7)
    Note over OR: SAGA start<br/>saga_id=uuid<br/>state=INITIATING

    OR->>EC: INITIATE_CALL (reply_to=orch)
    EC->>OR: ACK_CALL_STARTED
    OR->>UI: UPDATE_STATUS=SOS_ACTIVE (reply_to=orch)
    UI->>OR: ACK_UI
    OR->>MCU: NOTIFY_CALL_ACTIVE (reply_to=orch)
    MCU->>OR: ACK_MCU

    Note over OR: SAGA state=COMPLETED
```

### 7.2 Orchestration — Airbag Interrupt + Compensation

```mermaid
sequenceDiagram
    participant MCU as MCUService
    participant OR as OrchestratorSaga
    participant EC as ECallService
    participant UI as UIService

    Note over OR: SOS SAGA active<br/>priority=7

    MCU->>OR: AIRBAG_TRIGGERED<br/>priority=10
    Note over OR: priority(10) > current(7)\n→ COMPENSATING

    OR->>EC: TERMINATE_CALL (reply_to=orch)
    EC->>OR: ACK_TERMINATED
    OR->>UI: UPDATE_STATUS=TERMINATED (reply_to=orch)
    UI->>OR: ACK_UI

    Note over OR: ROLLED_BACK<br/>start new SAGA<br/>priority=10
    OR->>EC: INITIATE_CALL<br/>priority=10
    EC->>OR: ACK_CALL_STARTED
    OR->>UI: UPDATE_STATUS=ECALL_ACTIVE
    OR->>MCU: ACK_MCU
```

### 7.3 Choreography — Happy Path (SOS)

```mermaid
sequenceDiagram
    participant USER as User
    participant UI as UIHandler
    participant EB as EventBus
    participant EC as ECallHandler
    participant MCU as MCUHandler

    USER->>UI: press SOS
    UI->>EB: publish(SOS_REQUESTED, priority=7)
    EB->>EC: send_message [subscribed]
    EC->>EC: make_call()
    EC->>EB: publish(CALL_STARTED, saga_id=uuid)
    EB->>UI: send_message [subscribed]
    UI->>UI: update display → SOS_ACTIVE
    EB->>MCU: send_message [subscribed]
    MCU->>MCU: notify hardware
```

### 7.4 Choreography — Airbag Interrupt

```mermaid
sequenceDiagram
    participant MCU as MCUHandler
    participant EB as EventBus
    participant EC as ECallHandler
    participant UI as UIHandler

    Note over EC,UI: SOS active<br/>priority=7

    MCU->>EB: publish(AIRBAG_TRIGGERED, priority=10)
    EB->>EC: send_message [subscribed]
    Note over EC: priority(10) > local_state.priority(7)\n→ self-compensate
    EC->>EC: terminate_call()
    EC->>EB: publish(CALL_TERMINATED, reason=PREEMPTED)
    EB->>UI: send_message [subscribed]
    UI->>UI: update display → TERMINATED
    EC->>EC: initiate airbag call
    EC->>EB: publish(CALL_STARTED, priority=10)
    EB->>UI: send_message
    EB->>MCU: send_message
```

---

## 8. Priority & SAGA State Machine

### SAGA State Machine

```mermaid
stateDiagram-v2
    [*] --> IDLE
    IDLE --> INITIATING: new request
    INITIATING --> ACTIVE: all ACKs received
    INITIATING --> COMPENSATING: binderDied OR higher-priority preempt
    ACTIVE --> COMPENSATING: higher-priority preempt
    ACTIVE --> COMPLETED: call ended normally
    COMPENSATING --> ROLLING_BACK: send TERMINATE to all services
    ROLLING_BACK --> ROLLED_BACK: all ACK_TERMINATED received (Variant B - Graceful)
    ROLLING_BACK --> ROLLED_BACK: timeout → force (Variant C - reference only)
    ROLLED_BACK --> INITIATING: pending higher-priority request
    ROLLED_BACK --> IDLE: no pending request
    COMPLETED --> IDLE
    COMPENSATING --> FAILED: max retry exceeded
    FAILED --> [*]
```

### Compensation Variants

**Variant B — Graceful terminate with ACK (main demo):**

- Gửi `TERMINATE_CALL` tới tất cả active binders
- `await _wait_for_acks(expected=all_services, timeout=None)`
- Nhận đủ ACK → `ROLLED_BACK`

**Variant C — Timeout-based (reference only, không implement đầy đủ):**

- Giống B nhưng `timeout=COMPENSATION_TIMEOUT_SEC`
- Nếu timeout: force `ROLLED_BACK`, log missing ACKs

---

## 9. File Structure

```
ecall_saga_demo/
├── core/
│   ├── message.py          # Message, EventType enum, AckResult
│   ├── looper.py           # Looper, MessageQueue (priority heap)
│   ├── handler.py          # Handler base, IBinder, DeathRecipient protocol
│   ├── service_manager.py  # ServiceManager singleton
│   └── state.py            # ECallState, SagaState enums
├── services/
│   ├── base_service.py     # BaseService(Handler) + lifecycle helpers
│   ├── ui_service.py       # UIService: SOS input, display status
│   ├── ecall_service.py    # ECallService: make/terminate call (mock)
│   └── mcu_service.py      # MCUService: airbag trigger, hardware notify
├── saga/
│   ├── base_saga.py        # BaseSaga: state machine, compensation logic
│   ├── orchestration.py    # OrchestratorSaga: drives each step, owns ACK wait
│   └── choreography.py     # ChoreographySaga: EventBus routing setup
└── main.py                 # run_scenario(mode="orchestration"|"choreography")
```

---

## 10. Scenario được demo trong main.py

```
1. Khởi động 3 services (Looper threads)
2. Register với ServiceManager
3. Orchestrator/Choreography linkToDeath với mỗi service
4. [SOS Flow] User nhấn SOS → SAGA bắt đầu
5. [Interrupt] Airbag trigger giữa chừng → Compensation → New SAGA
6. [Recovery] Simulate ECallService crash → binderDied → retry → restart SAGA
7. In log toàn bộ state transitions
```
