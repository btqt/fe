# eCall SAGA Pattern Demo — Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Build a runnable Python demo illustrating SAGA pattern (Orchestration + Choreography) for an automotive eCall system, aligned with Android Binder/Looper concepts.

**Architecture:** Shared core infrastructure (Looper, Handler, IBinder, ServiceManager, EventBus) used by three services (UIService, ECallService, MCUService). Two interchangeable SAGA strategies (OrchestratorSaga, ChoreographySaga) run the same scenario: SOS call → airbag interrupt → compensation → new high-priority call → service crash recovery.

**Tech Stack:** Python 3.11+, `pytest`, `threading`, `heapq`, no external dependencies.

---

## File Map

```
ecall_saga_demo/
├── core/
│   ├── state.py            # EventType, ECallState, SagaState enums
│   ├── message.py          # Message dataclass (priority-ordered)
│   ├── looper.py           # MessageQueue (priority heap) + Looper (thread)
│   ├── handler.py          # Handler (abstract), IBinder, DeathRecipient
│   ├── service_manager.py  # ServiceManager singleton
│   └── event_bus.py        # EventBus: EventType → list[IBinder] routing
├── services/
│   ├── base_service.py     # BaseService(Handler): lifecycle + ACK helper
│   ├── ui_service.py       # UIService: SOS input, display state
│   ├── ecall_service.py    # ECallService: make/terminate call (mock sleep)
│   └── mcu_service.py      # MCUService: airbag trigger, hardware notify
├── saga/
│   ├── base_saga.py        # BaseSaga: state machine + compensation (Variant B + C)
│   ├── orchestration.py    # OrchestratorSaga: drives each step, awaits ACKs
│   └── choreography.py     # ChoreographySaga: EventBus subscription setup
├── tests/
│   ├── test_message_queue.py
│   ├── test_looper.py
│   ├── test_handler.py
│   ├── test_service_manager.py
│   ├── test_base_service.py
│   └── test_saga_orchestration.py
└── main.py                 # run_scenario(mode="orchestration"|"choreography")
```

---

## Task 1: Project Scaffold

**Files:**

- Create: `ecall_saga_demo/` (root directory for the demo)
- Create: `ecall_saga_demo/core/__init__.py`
- Create: `ecall_saga_demo/services/__init__.py`
- Create: `ecall_saga_demo/saga/__init__.py`
- Create: `ecall_saga_demo/tests/__init__.py`

- [ ] **Step 1: Create directory structure**

```
mkdir ecall_saga_demo
mkdir ecall_saga_demo\core
mkdir ecall_saga_demo\services
mkdir ecall_saga_demo\saga
mkdir ecall_saga_demo\tests
```

Tạo `__init__.py` rỗng trong mỗi folder.

- [ ] **Step 2: Verify pytest available**

```
python -m pytest --version
```

Expected: `pytest 7.x.x` hoặc cao hơn. Nếu thiếu: `pip install pytest`.

- [ ] **Step 3: Commit scaffold**

```bash
git add ecall_saga_demo/
git commit -m "chore: scaffold ecall_saga_demo project"
```

---

## Task 2: Core Enums — `core/state.py`

**Files:**

- Create: `ecall_saga_demo/core/state.py`
- Test: `ecall_saga_demo/tests/test_state.py` (inline, trivial)

- [ ] **Step 1: Write `core/state.py`**

```python
# ecall_saga_demo/core/state.py
from enum import Enum, auto


class EventType(Enum):
    # Lifecycle
    QUIT = auto()
    PING = auto()
    # ACKs
    ACK_PING = auto()
    ACK_CALL_STARTED = auto()
    ACK_TERMINATED = auto()
    ACK_UI = auto()
    ACK_MCU = auto()
    # SAGA triggers
    SOS_REQUESTED = auto()
    AIRBAG_TRIGGERED = auto()
    # Orchestrator commands
    INITIATE_CALL = auto()
    TERMINATE_CALL = auto()
    UPDATE_STATUS = auto()
    NOTIFY_CALL_ACTIVE = auto()
    # Service events (Choreography)
    CALL_STARTED = auto()
    CALL_TERMINATED = auto()


class ECallState(Enum):
    IDLE = auto()
    SOS_ACTIVE = auto()
    ECALL_ACTIVE = auto()
    TERMINATED = auto()


class SagaState(Enum):
    IDLE = auto()
    INITIATING = auto()
    ACTIVE = auto()
    COMPENSATING = auto()
    ROLLING_BACK = auto()
    ROLLED_BACK = auto()
    COMPLETED = auto()
    FAILED = auto()
```

- [ ] **Step 2: Verify import**

```
python -c "from ecall_saga_demo.core.state import EventType, ECallState, SagaState; print('OK')"
```

Expected: `OK`

- [ ] **Step 3: Commit**

```bash
git add ecall_saga_demo/core/state.py
git commit -m "feat: add core enums (EventType, ECallState, SagaState)"
```

---

## Task 3: Message Dataclass — `core/message.py`

**Files:**

- Create: `ecall_saga_demo/core/message.py`
- Test: `ecall_saga_demo/tests/test_message_queue.py`

- [ ] **Step 1: Write failing test**

```python
# ecall_saga_demo/tests/test_message_queue.py
import heapq
from ecall_saga_demo.core.message import Message
from ecall_saga_demo.core.state import EventType


def test_message_ordering_by_priority():
    """Higher priority message must sort before lower priority."""
    low = Message(what=EventType.SOS_REQUESTED, priority=7)
    high = Message(what=EventType.AIRBAG_TRIGGERED, priority=10)
    heap = []
    heapq.heappush(heap, low)
    heapq.heappush(heap, high)
    first = heapq.heappop(heap)
    assert first.priority == 10, "High-priority message must come first"


def test_message_fifo_same_priority():
    """Same priority → earlier timestamp comes first."""
    import time
    a = Message(what=EventType.SOS_REQUESTED, priority=7)
    time.sleep(0.001)
    b = Message(what=EventType.SOS_REQUESTED, priority=7)
    heap = []
    heapq.heappush(heap, a)
    heapq.heappush(heap, b)
    first = heapq.heappop(heap)
    assert first is a, "Earlier message must come first at same priority"
```

- [ ] **Step 2: Run to verify FAIL**

```
python -m pytest ecall_saga_demo/tests/test_message_queue.py -v
```

Expected: `ImportError` hoặc `ModuleNotFoundError`.

- [ ] **Step 3: Write `core/message.py`**

```python
# ecall_saga_demo/core/message.py
from __future__ import annotations
import time
import uuid
from dataclasses import dataclass, field
from typing import TYPE_CHECKING, Any

from ecall_saga_demo.core.state import EventType

if TYPE_CHECKING:
    from ecall_saga_demo.core.handler import Handler


@dataclass
class Message:
    what: EventType
    priority: int = 5
    saga_id: str = field(default_factory=lambda: str(uuid.uuid4()))
    obj: dict = field(default_factory=dict)
    reply_to: Any = None          # Handler | None
    timestamp: float = field(default_factory=time.monotonic)

    # Heap ordering: higher priority first, then FIFO by timestamp
    def __lt__(self, other: Message) -> bool:
        if self.priority != other.priority:
            return self.priority > other.priority   # higher priority = smaller in heap
        return self.timestamp < other.timestamp     # earlier = smaller (FIFO)

    def __le__(self, other: Message) -> bool:
        return self == other or self < other

    def __eq__(self, other: object) -> bool:
        if not isinstance(other, Message):
            return NotImplemented
        return self.saga_id == other.saga_id and self.what == other.what

    def short_id(self) -> str:
        return self.saga_id[:8]
```

- [ ] **Step 4: Run to verify PASS**

```
python -m pytest ecall_saga_demo/tests/test_message_queue.py -v
```

Expected: `2 passed`

- [ ] **Step 5: Commit**

```bash
git add ecall_saga_demo/core/message.py ecall_saga_demo/tests/test_message_queue.py
git commit -m "feat: add Message dataclass with priority ordering"
```

---

## Task 4: MessageQueue + Looper — `core/looper.py`

**Files:**

- Create: `ecall_saga_demo/core/looper.py`
- Test: `ecall_saga_demo/tests/test_looper.py`

- [ ] **Step 1: Write failing tests**

```python
# ecall_saga_demo/tests/test_looper.py
import threading
import time
from ecall_saga_demo.core.looper import MessageQueue, Looper
from ecall_saga_demo.core.message import Message
from ecall_saga_demo.core.state import EventType


def test_queue_put_get():
    q = MessageQueue()
    msg = Message(what=EventType.PING, priority=5)
    q.put(msg)
    result = q.get_nowait()
    assert result is msg


def test_queue_priority_order():
    q = MessageQueue()
    q.put(Message(what=EventType.SOS_REQUESTED, priority=7))
    q.put(Message(what=EventType.AIRBAG_TRIGGERED, priority=10))
    first = q.get_nowait()
    assert first.what == EventType.AIRBAG_TRIGGERED


def test_looper_delivers_message():
    received = []
    q_ready = threading.Event()

    class FakeHandler:
        def dispatch(self, msg):
            received.append(msg)
            q_ready.set()

    looper = Looper()
    looper.attach_handler(FakeHandler())
    looper.start()

    looper.queue.put(Message(what=EventType.PING, priority=5))
    q_ready.wait(timeout=1.0)
    looper.quit()

    assert len(received) == 1
    assert received[0].what == EventType.PING
```

- [ ] **Step 2: Run to verify FAIL**

```
python -m pytest ecall_saga_demo/tests/test_looper.py -v
```

Expected: `ImportError`.

- [ ] **Step 3: Write `core/looper.py`**

```python
# ecall_saga_demo/core/looper.py
from __future__ import annotations
import heapq
import threading
from typing import TYPE_CHECKING

from ecall_saga_demo.core.message import Message
from ecall_saga_demo.core.state import EventType

if TYPE_CHECKING:
    from ecall_saga_demo.core.handler import Handler


class MessageQueue:
    def __init__(self) -> None:
        self._heap: list[Message] = []
        self._lock = threading.Lock()
        self._ready = threading.Event()

    def put(self, msg: Message) -> None:
        with self._lock:
            heapq.heappush(self._heap, msg)
        self._ready.set()

    def get(self) -> Message:
        """Block until a message is available."""
        while True:
            self._ready.wait()
            with self._lock:
                if self._heap:
                    msg = heapq.heappop(self._heap)
                    if not self._heap:
                        self._ready.clear()
                    return msg

    def get_nowait(self) -> Message | None:
        with self._lock:
            return heapq.heappop(self._heap) if self._heap else None

    def peek_priority(self) -> int:
        with self._lock:
            return self._heap[0].priority if self._heap else 0


class Looper:
    def __init__(self) -> None:
        self.queue = MessageQueue()
        self._handler: Handler | None = None
        self._running = False
        self._thread: threading.Thread | None = None

    def attach_handler(self, handler: Handler) -> None:
        self._handler = handler

    def start(self) -> None:
        self._running = True
        self._thread = threading.Thread(target=self._loop, daemon=True)
        self._thread.start()

    def _loop(self) -> None:
        while self._running:
            msg = self.queue.get()
            if msg.what == EventType.QUIT:
                break
            if self._handler:
                self._handler.dispatch(msg)

    def quit(self) -> None:
        self._running = False
        self.queue.put(Message(what=EventType.QUIT, priority=0))
        if self._thread:
            self._thread.join(timeout=2.0)
```

- [ ] **Step 4: Run to verify PASS**

```
python -m pytest ecall_saga_demo/tests/test_looper.py -v
```

Expected: `3 passed`

- [ ] **Step 5: Commit**

```bash
git add ecall_saga_demo/core/looper.py ecall_saga_demo/tests/test_looper.py
git commit -m "feat: add MessageQueue (priority heap) and Looper (thread)"
```

---

## Task 5: Handler + IBinder + DeathRecipient — `core/handler.py`

**Files:**

- Create: `ecall_saga_demo/core/handler.py`
- Test: `ecall_saga_demo/tests/test_handler.py`

- [ ] **Step 1: Write failing tests**

```python
# ecall_saga_demo/tests/test_handler.py
import threading
from ecall_saga_demo.core.handler import Handler, IBinder, DeathRecipient
from ecall_saga_demo.core.looper import Looper
from ecall_saga_demo.core.message import Message
from ecall_saga_demo.core.state import EventType


class EchoHandler(Handler):
    def __init__(self, looper):
        super().__init__(looper)
        self.received: list[Message] = []
        self.ready = threading.Event()

    def handle_message(self, msg: Message) -> None:
        self.received.append(msg)
        self.ready.set()


def test_handler_receives_message():
    looper = Looper()
    handler = EchoHandler(looper)
    looper.start()

    msg = Message(what=EventType.PING, priority=5)
    handler.send_message(msg)
    handler.ready.wait(timeout=1.0)
    looper.quit()

    assert len(handler.received) == 1
    assert handler.received[0].what == EventType.PING


def test_ibinder_send_and_alive():
    looper = Looper()
    handler = EchoHandler(looper)
    looper.start()
    binder = IBinder("test_svc", handler)

    assert binder.is_alive()
    binder.send_message(Message(what=EventType.PING, priority=5))
    handler.ready.wait(timeout=1.0)
    looper.quit()
    assert len(handler.received) == 1


def test_ibinder_death_notification():
    looper = Looper()
    handler = EchoHandler(looper)
    looper.start()
    binder = IBinder("ecall", handler)

    died_events: list[str] = []

    class Recipient(DeathRecipient):
        def binder_died(self, service_name: str) -> None:
            died_events.append(service_name)

    recipient = Recipient()
    binder.link_to_death(recipient)
    binder.simulate_death()
    looper.quit()

    assert not binder.is_alive()
    assert died_events == ["ecall"]
    # After death, send_message should be a no-op
    binder.send_message(Message(what=EventType.PING))
    assert len(handler.received) == 0
```

- [ ] **Step 2: Run to verify FAIL**

```
python -m pytest ecall_saga_demo/tests/test_handler.py -v
```

Expected: `ImportError`.

- [ ] **Step 3: Write `core/handler.py`**

```python
# ecall_saga_demo/core/handler.py
from __future__ import annotations
from abc import abstractmethod
from typing import runtime_checkable, Protocol

from ecall_saga_demo.core.looper import Looper
from ecall_saga_demo.core.message import Message


class Handler:
    def __init__(self, looper: Looper) -> None:
        self.looper = looper
        looper.attach_handler(self)

    def send_message(self, msg: Message) -> None:
        self.looper.queue.put(msg)

    def dispatch(self, msg: Message) -> None:
        self.handle_message(msg)

    @abstractmethod
    def handle_message(self, msg: Message) -> None: ...


@runtime_checkable
class DeathRecipient(Protocol):
    def binder_died(self, service_name: str) -> None: ...


class IBinder:
    def __init__(self, service_name: str, handler: Handler) -> None:
        self._service_name = service_name
        self._handler = handler
        self._alive = True
        self._recipients: list[DeathRecipient] = []

    def link_to_death(self, recipient: DeathRecipient) -> None:
        self._recipients.append(recipient)

    def unlink_to_death(self, recipient: DeathRecipient) -> None:
        self._recipients = [r for r in self._recipients if r is not recipient]

    def send_message(self, msg: Message) -> None:
        if self._alive:
            self._handler.send_message(msg)

    def is_alive(self) -> bool:
        return self._alive

    def simulate_death(self) -> None:
        """Simulate process crash — notifies all linkToDeath recipients."""
        self._alive = False
        for recipient in self._recipients:
            recipient.binder_died(self._service_name)
```

- [ ] **Step 4: Run to verify PASS**

```
python -m pytest ecall_saga_demo/tests/test_handler.py -v
```

Expected: `3 passed`

- [ ] **Step 5: Commit**

```bash
git add ecall_saga_demo/core/handler.py ecall_saga_demo/tests/test_handler.py
git commit -m "feat: add Handler, IBinder, DeathRecipient"
```

---

## Task 6: ServiceManager — `core/service_manager.py`

**Files:**

- Create: `ecall_saga_demo/core/service_manager.py`
- Test: `ecall_saga_demo/tests/test_service_manager.py`

- [ ] **Step 1: Write failing tests**

```python
# ecall_saga_demo/tests/test_service_manager.py
from ecall_saga_demo.core.service_manager import ServiceManager
from ecall_saga_demo.core.handler import Handler, IBinder
from ecall_saga_demo.core.looper import Looper
from ecall_saga_demo.core.message import Message
from ecall_saga_demo.core.state import EventType


class DummyHandler(Handler):
    def handle_message(self, msg: Message) -> None:
        pass


def make_handler() -> Handler:
    looper = Looper()
    return DummyHandler(looper)


def test_add_and_get_service():
    ServiceManager.reset()
    sm = ServiceManager.get_instance()
    h = make_handler()
    binder = sm.add_service("ecall", h)
    assert isinstance(binder, IBinder)
    assert sm.get_service("ecall") is binder


def test_get_missing_service_returns_none():
    ServiceManager.reset()
    sm = ServiceManager.get_instance()
    assert sm.get_service("nonexistent") is None


def test_remove_service():
    ServiceManager.reset()
    sm = ServiceManager.get_instance()
    sm.add_service("ui", make_handler())
    sm.remove_service("ui")
    assert sm.get_service("ui") is None


def test_singleton():
    ServiceManager.reset()
    a = ServiceManager.get_instance()
    b = ServiceManager.get_instance()
    assert a is b
```

- [ ] **Step 2: Run to verify FAIL**

```
python -m pytest ecall_saga_demo/tests/test_service_manager.py -v
```

Expected: `ImportError`.

- [ ] **Step 3: Write `core/service_manager.py`**

```python
# ecall_saga_demo/core/service_manager.py
from __future__ import annotations
from ecall_saga_demo.core.handler import Handler, IBinder


class ServiceManager:
    _instance: ServiceManager | None = None

    def __init__(self) -> None:
        self._registry: dict[str, IBinder] = {}

    @classmethod
    def get_instance(cls) -> ServiceManager:
        if cls._instance is None:
            cls._instance = ServiceManager()
        return cls._instance

    @classmethod
    def reset(cls) -> None:
        """Reset singleton — use in tests only."""
        cls._instance = None

    def add_service(self, name: str, handler: Handler) -> IBinder:
        binder = IBinder(name, handler)
        self._registry[name] = binder
        return binder

    def get_service(self, name: str) -> IBinder | None:
        return self._registry.get(name)

    def remove_service(self, name: str) -> None:
        self._registry.pop(name, None)
```

- [ ] **Step 4: Run to verify PASS**

```
python -m pytest ecall_saga_demo/tests/test_service_manager.py -v
```

Expected: `4 passed`

- [ ] **Step 5: Commit**

```bash
git add ecall_saga_demo/core/service_manager.py ecall_saga_demo/tests/test_service_manager.py
git commit -m "feat: add ServiceManager singleton"
```

---

## Task 7: EventBus — `core/event_bus.py`

Dùng cho Choreography SAGA: route message từ EventType → list IBinder subscribers.

**Files:**

- Create: `ecall_saga_demo/core/event_bus.py`

- [ ] **Step 1: Write `core/event_bus.py`**

```python
# ecall_saga_demo/core/event_bus.py
from __future__ import annotations
from collections import defaultdict
from ecall_saga_demo.core.handler import IBinder
from ecall_saga_demo.core.message import Message
from ecall_saga_demo.core.state import EventType


class EventBus:
    def __init__(self) -> None:
        self._subs: dict[EventType, list[IBinder]] = defaultdict(list)

    def subscribe(self, event_type: EventType, binder: IBinder) -> None:
        self._subs[event_type].append(binder)

    def unsubscribe(self, event_type: EventType, binder: IBinder) -> None:
        self._subs[event_type] = [
            b for b in self._subs[event_type] if b is not binder
        ]

    def publish(self, msg: Message) -> None:
        """Deliver msg to all IBinders subscribed to msg.what."""
        for binder in list(self._subs.get(msg.what, [])):
            if binder.is_alive():
                binder.send_message(msg)
```

- [ ] **Step 2: Verify import**

```
python -c "from ecall_saga_demo.core.event_bus import EventBus; print('OK')"
```

Expected: `OK`

- [ ] **Step 3: Commit**

```bash
git add ecall_saga_demo/core/event_bus.py
git commit -m "feat: add EventBus for choreography routing"
```

---

## Task 8: BaseService — `services/base_service.py`

**Files:**

- Create: `ecall_saga_demo/services/base_service.py`
- Test: `ecall_saga_demo/tests/test_base_service.py`

- [ ] **Step 1: Write failing tests**

```python
# ecall_saga_demo/tests/test_base_service.py
import threading
from ecall_saga_demo.services.base_service import BaseService
from ecall_saga_demo.core.message import Message
from ecall_saga_demo.core.state import EventType, ECallState


class ConcreteService(BaseService):
    def __init__(self):
        super().__init__("test_svc")
        self.handled: list[Message] = []
        self.done = threading.Event()

    def handle_message(self, msg: Message) -> None:
        self.handled.append(msg)
        self.done.set()


def test_service_starts_idle():
    svc = ConcreteService()
    assert svc.local_state == ECallState.IDLE
    assert svc.saga_id is None


def test_service_receives_message():
    svc = ConcreteService()
    svc.start()
    svc.send_message(Message(what=EventType.PING, priority=5))
    svc.done.wait(timeout=1.0)
    svc.stop()
    assert len(svc.handled) == 1


def test_send_ack_to_reply_handler():
    received_acks: list[Message] = []
    ack_ready = threading.Event()

    # Create a reply handler to capture ACK
    from ecall_saga_demo.core.looper import Looper
    from ecall_saga_demo.core.handler import Handler

    class ReplyCapture(Handler):
        def handle_message(self, msg: Message) -> None:
            received_acks.append(msg)
            ack_ready.set()

    reply_looper = Looper()
    reply_handler = ReplyCapture(reply_looper)
    reply_looper.start()

    svc = ConcreteService()
    svc.start()

    msg = Message(what=EventType.PING, priority=5, reply_to=reply_handler)
    svc._send_ack(msg, EventType.ACK_PING)

    ack_ready.wait(timeout=1.0)
    reply_looper.quit()
    svc.stop()

    assert len(received_acks) == 1
    assert received_acks[0].what == EventType.ACK_PING
```

- [ ] **Step 2: Run to verify FAIL**

```
python -m pytest ecall_saga_demo/tests/test_base_service.py -v
```

Expected: `ImportError`.

- [ ] **Step 3: Write `services/base_service.py`**

```python
# ecall_saga_demo/services/base_service.py
from __future__ import annotations
from abc import abstractmethod

from ecall_saga_demo.core.handler import Handler
from ecall_saga_demo.core.looper import Looper
from ecall_saga_demo.core.message import Message
from ecall_saga_demo.core.state import ECallState, EventType


class BaseService(Handler):
    def __init__(self, name: str) -> None:
        self.name = name
        self._looper = Looper()
        super().__init__(self._looper)
        self.local_state: ECallState = ECallState.IDLE
        self.saga_id: str | None = None
        self.priority: int = 0

    def start(self) -> None:
        self._looper.start()
        print(f"[{self.name}] Started")

    def stop(self) -> None:
        self._looper.quit()
        print(f"[{self.name}] Stopped")

    def _send_ack(self, msg: Message, ack_type: EventType, success: bool = True) -> None:
        if msg.reply_to is not None:
            msg.reply_to.send_message(Message(
                what=ack_type,
                priority=msg.priority,
                saga_id=msg.saga_id,
                obj={"success": success, "service": self.name},
            ))

    @abstractmethod
    def handle_message(self, msg: Message) -> None: ...
```

- [ ] **Step 4: Run to verify PASS**

```
python -m pytest ecall_saga_demo/tests/test_base_service.py -v
```

Expected: `3 passed`

- [ ] **Step 5: Commit**

```bash
git add ecall_saga_demo/services/base_service.py ecall_saga_demo/tests/test_base_service.py
git commit -m "feat: add BaseService with Looper lifecycle and ACK helper"
```

---

## Task 9: UIService, ECallService, MCUService

**Files:**

- Create: `ecall_saga_demo/services/ui_service.py`
- Create: `ecall_saga_demo/services/ecall_service.py`
- Create: `ecall_saga_demo/services/mcu_service.py`

- [ ] **Step 1: Write `services/ui_service.py`**

```python
# ecall_saga_demo/services/ui_service.py
import time
from ecall_saga_demo.core.message import Message
from ecall_saga_demo.core.state import ECallState, EventType
from ecall_saga_demo.services.base_service import BaseService


class UIService(BaseService):
    def __init__(self) -> None:
        super().__init__("UIService")

    def handle_message(self, msg: Message) -> None:
        print(f"[UIService] ← {msg.what.name} | saga={msg.short_id()}")

        if msg.what == EventType.UPDATE_STATUS:
            status_name = msg.obj.get("status", "IDLE")
            self.local_state = ECallState[status_name]
            self.saga_id = msg.saga_id
            self.priority = msg.priority
            print(f"[UIService] Display → {self.local_state.name}")
            self._send_ack(msg, EventType.ACK_UI)

        elif msg.what == EventType.PING:
            self._send_ack(msg, EventType.ACK_PING)

        # Choreography: react to CALL_STARTED / CALL_TERMINATED
        elif msg.what == EventType.CALL_STARTED:
            self.local_state = (
                ECallState.SOS_ACTIVE if msg.priority <= 7 else ECallState.ECALL_ACTIVE
            )
            self.saga_id = msg.saga_id
            self.priority = msg.priority
            print(f"[UIService] Display → {self.local_state.name}")

        elif msg.what == EventType.CALL_TERMINATED:
            self.local_state = ECallState.TERMINATED
            reason = msg.obj.get("reason", "unknown")
            print(f"[UIService] Display → TERMINATED (reason={reason})")
```

- [ ] **Step 2: Write `services/ecall_service.py`**

```python
# ecall_saga_demo/services/ecall_service.py
import time
from typing import Callable
from ecall_saga_demo.core.message import Message
from ecall_saga_demo.core.state import ECallState, EventType
from ecall_saga_demo.services.base_service import BaseService


class ECallService(BaseService):
    def __init__(self, publish_fn: Callable[[Message], None] | None = None) -> None:
        super().__init__("ECallService")
        # publish_fn is injected in Choreography mode to emit events to EventBus
        self.publish_fn = publish_fn

    def handle_message(self, msg: Message) -> None:
        print(f"[ECallService] ← {msg.what.name} | saga={msg.short_id()}")

        if msg.what == EventType.INITIATE_CALL:
            time.sleep(0.05)    # mock: simulate call setup latency
            self.local_state = (
                ECallState.SOS_ACTIVE if msg.priority <= 7 else ECallState.ECALL_ACTIVE
            )
            self.saga_id = msg.saga_id
            self.priority = msg.priority
            print(f"[ECallService] Call active → {self.local_state.name}")
            self._send_ack(msg, EventType.ACK_CALL_STARTED)
            # Choreography: publish CALL_STARTED to EventBus
            if self.publish_fn:
                self.publish_fn(Message(
                    what=EventType.CALL_STARTED,
                    priority=msg.priority,
                    saga_id=msg.saga_id,
                ))

        elif msg.what == EventType.TERMINATE_CALL:
            self._do_terminate()
            self._send_ack(msg, EventType.ACK_TERMINATED)
            # Choreography: publish CALL_TERMINATED
            if self.publish_fn:
                reason = msg.obj.get("reason", "TERMINATED")
                self.publish_fn(Message(
                    what=EventType.CALL_TERMINATED,
                    priority=msg.priority,
                    saga_id=msg.saga_id,
                    obj={"reason": reason},
                ))

        elif msg.what == EventType.AIRBAG_TRIGGERED:
            # Choreography self-compensation: higher priority preempts current
            if self.local_state != ECallState.IDLE and msg.priority > self.priority:
                print(f"[ECallService] Preempted! priority {msg.priority} > {self.priority}")
                self._do_terminate()
                if self.publish_fn:
                    self.publish_fn(Message(
                        what=EventType.CALL_TERMINATED,
                        priority=msg.priority,
                        saga_id=msg.saga_id,
                        obj={"reason": "PREEMPTED_BY_AIRBAG"},
                    ))
                # Now initiate the airbag call
                time.sleep(0.05)
                self.local_state = ECallState.ECALL_ACTIVE
                self.saga_id = msg.saga_id
                self.priority = msg.priority
                print(f"[ECallService] Airbag call active")
                if self.publish_fn:
                    self.publish_fn(Message(
                        what=EventType.CALL_STARTED,
                        priority=msg.priority,
                        saga_id=msg.saga_id,
                    ))

        elif msg.what == EventType.PING:
            self._send_ack(msg, EventType.ACK_PING)

    def _do_terminate(self) -> None:
        print(f"[ECallService] Call terminated")
        self.local_state = ECallState.TERMINATED
        self.saga_id = None
        self.priority = 0
```

- [ ] **Step 3: Write `services/mcu_service.py`**

```python
# ecall_saga_demo/services/mcu_service.py
from ecall_saga_demo.core.message import Message
from ecall_saga_demo.core.state import ECallState, EventType
from ecall_saga_demo.services.base_service import BaseService


class MCUService(BaseService):
    def __init__(self) -> None:
        super().__init__("MCUService")

    def handle_message(self, msg: Message) -> None:
        print(f"[MCUService] ← {msg.what.name} | saga={msg.short_id()}")

        if msg.what == EventType.NOTIFY_CALL_ACTIVE:
            self.local_state = ECallState.SOS_ACTIVE if msg.priority <= 7 else ECallState.ECALL_ACTIVE
            self.saga_id = msg.saga_id
            print(f"[MCUService] Hardware notified → {self.local_state.name}")
            self._send_ack(msg, EventType.ACK_MCU)

        elif msg.what == EventType.CALL_STARTED:
            # Choreography: react to ECallService publishing CALL_STARTED
            self.local_state = ECallState.SOS_ACTIVE if msg.priority <= 7 else ECallState.ECALL_ACTIVE
            self.saga_id = msg.saga_id
            print(f"[MCUService] Hardware notified (choreo) → {self.local_state.name}")

        elif msg.what == EventType.CALL_TERMINATED:
            self.local_state = ECallState.TERMINATED
            print(f"[MCUService] Hardware: call terminated")

        elif msg.what == EventType.PING:
            self._send_ack(msg, EventType.ACK_PING)
```

- [ ] **Step 4: Verify imports**

```
python -c "
from ecall_saga_demo.services.ui_service import UIService
from ecall_saga_demo.services.ecall_service import ECallService
from ecall_saga_demo.services.mcu_service import MCUService
print('All services OK')
"
```

Expected: `All services OK`

- [ ] **Step 5: Commit**

```bash
git add ecall_saga_demo/services/
git commit -m "feat: add UIService, ECallService, MCUService"
```

---

## Task 10: BaseSaga + Compensation Logic — `saga/base_saga.py`

**Files:**

- Create: `ecall_saga_demo/saga/base_saga.py`

- [ ] **Step 1: Write `saga/base_saga.py`**

```python
# ecall_saga_demo/saga/base_saga.py
from __future__ import annotations
import threading
import time
import uuid
from typing import TYPE_CHECKING

from ecall_saga_demo.core.handler import Handler, IBinder, DeathRecipient
from ecall_saga_demo.core.looper import Looper
from ecall_saga_demo.core.message import Message
from ecall_saga_demo.core.service_manager import ServiceManager
from ecall_saga_demo.core.state import EventType, SagaState

COMPENSATION_TIMEOUT_SEC = 2.0   # Variant C reference constant
MAX_RETRY_BACKOFF_SEC = 30.0


class SagaACKHandler(Handler):
    """Internal handler on the Orchestrator's own Looper to receive ACKs."""

    def __init__(self, looper: Looper, saga: BaseSaga) -> None:
        super().__init__(looper)
        self._saga = saga
        self._pending: dict[str, threading.Event] = {}  # saga_id+ack_type → Event
        self._lock = threading.Lock()

    def handle_message(self, msg: Message) -> None:
        key = f"{msg.saga_id}:{msg.what.name}"
        with self._lock:
            event = self._pending.get(key)
        if event:
            event.set()
        self._saga.on_ack(msg)

    def register_ack(self, saga_id: str, ack_type: EventType) -> threading.Event:
        key = f"{saga_id}:{ack_type.name}"
        event = threading.Event()
        with self._lock:
            self._pending[key] = event
        return event

    def clear_acks(self, saga_id: str) -> None:
        with self._lock:
            to_remove = [k for k in self._pending if k.startswith(saga_id)]
            for k in to_remove:
                del self._pending[k]


class BaseSaga(DeathRecipient):
    SERVICE_NAMES = ["ecall", "ui", "mcu"]

    def __init__(self) -> None:
        self.saga_id: str = str(uuid.uuid4())
        self.state: SagaState = SagaState.IDLE
        self.priority: int = 0
        self._looper = Looper()
        self._ack_handler = SagaACKHandler(self._looper, self)
        self._binders: dict[str, IBinder] = {}
        self._looper.start()

    # ------------------------------------------------------------------ #
    # Lifecycle                                                            #
    # ------------------------------------------------------------------ #

    def connect_services(self) -> None:
        """Resolve all service binders and register linkToDeath."""
        sm = ServiceManager.get_instance()
        for name in self.SERVICE_NAMES:
            binder = sm.get_service(name)
            if binder is None:
                raise RuntimeError(f"Service '{name}' not registered")
            binder.link_to_death(self)
            self._binders[name] = binder
        print(f"[Saga] Connected to all services")

    def stop(self) -> None:
        self._looper.quit()

    # ------------------------------------------------------------------ #
    # ACK wait helpers                                                     #
    # ------------------------------------------------------------------ #

    def _send_and_wait_ack(
        self,
        binder: IBinder,
        msg: Message,
        ack_type: EventType,
        timeout: float | None = None,
    ) -> bool:
        """
        Send msg and block until ACK is received.
        Variant B: timeout=None (wait forever).
        Variant C: timeout=COMPENSATION_TIMEOUT_SEC.
        """
        event = self._ack_handler.register_ack(msg.saga_id, ack_type)
        msg.reply_to = self._ack_handler
        binder.send_message(msg)
        ok = event.wait(timeout=timeout)
        if not ok:
            print(f"[Saga] TIMEOUT waiting for {ack_type.name} from {binder._service_name}")
        return ok

    # ------------------------------------------------------------------ #
    # Compensation — Variant B (Graceful, main demo)                      #
    # ------------------------------------------------------------------ #

    def compensate_graceful(self, reason: str) -> bool:
        """
        Variant B: Send TERMINATE to all alive services and wait for ACKs.
        No timeout — guaranteed clean state before proceeding.
        """
        print(f"[Saga] COMPENSATION START (graceful) reason={reason} saga={self.saga_id[:8]}")
        self.state = SagaState.ROLLING_BACK

        for name, binder in self._binders.items():
            if not binder.is_alive():
                print(f"[Saga] Skip {name} — binder dead")
                continue
            msg = Message(
                what=EventType.TERMINATE_CALL,
                priority=self.priority,
                saga_id=self.saga_id,
                obj={"reason": reason},
            )
            ok = self._send_and_wait_ack(
                binder, msg, EventType.ACK_TERMINATED, timeout=None  # Variant B
            )
            if not ok:
                print(f"[Saga] WARNING: No ACK from {name}")

        self.state = SagaState.ROLLED_BACK
        print(f"[Saga] ROLLED_BACK saga={self.saga_id[:8]}")
        return True

    # ------------------------------------------------------------------ #
    # Compensation — Variant C (Timeout-based, reference)                 #
    # ------------------------------------------------------------------ #

    def compensate_with_timeout(self, reason: str) -> bool:
        """
        Variant C (reference): Same as graceful but with timeout.
        Missing ACKs are logged and execution continues (force rollback).
        """
        print(f"[Saga] COMPENSATION START (timeout-based) reason={reason}")
        self.state = SagaState.ROLLING_BACK
        missing_acks: list[str] = []

        for name, binder in self._binders.items():
            if not binder.is_alive():
                continue
            msg = Message(
                what=EventType.TERMINATE_CALL,
                priority=self.priority,
                saga_id=self.saga_id,
                obj={"reason": reason},
            )
            ok = self._send_and_wait_ack(
                binder, msg, EventType.ACK_TERMINATED,
                timeout=COMPENSATION_TIMEOUT_SEC      # Variant C
            )
            if not ok:
                missing_acks.append(name)

        if missing_acks:
            print(f"[Saga] Force rollback — missing ACK from: {missing_acks}")
        self.state = SagaState.ROLLED_BACK
        return True

    # ------------------------------------------------------------------ #
    # binderDied recovery                                                  #
    # ------------------------------------------------------------------ #

    def binder_died(self, service_name: str) -> None:
        print(f"[Saga] !! binderDied: {service_name} | saga={self.saga_id[:8]}")
        self.state = SagaState.COMPENSATING
        self._retry_reconnect(service_name)

    def _retry_reconnect(self, service_name: str) -> None:
        sm = ServiceManager.get_instance()
        backoff = 0.5
        for attempt in range(1, 6):
            print(f"[Saga] Reconnect attempt {attempt} for '{service_name}' (backoff={backoff}s)")
            time.sleep(backoff)
            binder = sm.get_service(service_name)
            if binder and binder.is_alive():
                binder.link_to_death(self)
                self._binders[service_name] = binder
                print(f"[Saga] Reconnected to '{service_name}'")
                return
            backoff = min(backoff * 2, MAX_RETRY_BACKOFF_SEC)
        self.state = SagaState.FAILED
        print(f"[Saga] FAILED: Could not reconnect to '{service_name}'")

    def on_ack(self, msg: Message) -> None:
        """Called by SagaACKHandler when an ACK arrives. Override in subclasses."""
        pass
```

- [ ] **Step 2: Verify import**

```
python -c "from ecall_saga_demo.saga.base_saga import BaseSaga; print('OK')"
```

Expected: `OK`

- [ ] **Step 3: Commit**

```bash
git add ecall_saga_demo/saga/base_saga.py
git commit -m "feat: add BaseSaga with compensation Variant B+C and binderDied recovery"
```

---

## Task 11: OrchestratorSaga — `saga/orchestration.py`

**Files:**

- Create: `ecall_saga_demo/saga/orchestration.py`
- Test: `ecall_saga_demo/tests/test_saga_orchestration.py`

- [ ] **Step 1: Write failing tests**

```python
# ecall_saga_demo/tests/test_saga_orchestration.py
import time
from ecall_saga_demo.core.service_manager import ServiceManager
from ecall_saga_demo.core.state import ECallState, SagaState
from ecall_saga_demo.services.ui_service import UIService
from ecall_saga_demo.services.ecall_service import ECallService
from ecall_saga_demo.services.mcu_service import MCUService
from ecall_saga_demo.saga.orchestration import OrchestratorSaga


def setup_services():
    ServiceManager.reset()
    sm = ServiceManager.get_instance()
    ui = UIService(); ui.start()
    ec = ECallService(); ec.start()
    mcu = MCUService(); mcu.start()
    sm.add_service("ui", ui)
    sm.add_service("ecall", ec)
    sm.add_service("mcu", mcu)
    return ui, ec, mcu


def test_sos_call_happy_path():
    ui, ec, mcu = setup_services()
    orch = OrchestratorSaga()
    orch.connect_services()

    orch.run_sos_call()
    time.sleep(0.5)   # allow async steps to complete

    assert ec.local_state == ECallState.SOS_ACTIVE
    assert ui.local_state == ECallState.SOS_ACTIVE
    assert orch.state == SagaState.COMPLETED

    orch.stop(); ui.stop(); ec.stop(); mcu.stop()


def test_airbag_preempts_sos():
    ui, ec, mcu = setup_services()
    orch = OrchestratorSaga()
    orch.connect_services()

    orch.run_sos_call()
    time.sleep(0.2)
    orch.run_airbag_call()
    time.sleep(0.8)

    assert ec.local_state == ECallState.ECALL_ACTIVE
    assert orch.state == SagaState.COMPLETED

    orch.stop(); ui.stop(); ec.stop(); mcu.stop()
```

- [ ] **Step 2: Run to verify FAIL**

```
python -m pytest ecall_saga_demo/tests/test_saga_orchestration.py -v
```

Expected: `ImportError`.

- [ ] **Step 3: Write `saga/orchestration.py`**

```python
# ecall_saga_demo/saga/orchestration.py
from __future__ import annotations
import threading
import uuid

from ecall_saga_demo.core.message import Message
from ecall_saga_demo.core.state import EventType, SagaState
from ecall_saga_demo.saga.base_saga import BaseSaga


class OrchestratorSaga(BaseSaga):
    """
    Orchestration SAGA: central coordinator drives each step and waits for ACK
    before proceeding. Priority preemption triggers graceful compensation.
    """

    def _start_new_saga(self) -> None:
        self.saga_id = str(uuid.uuid4())
        self.state = SagaState.INITIATING
        self._ack_handler.clear_acks(self.saga_id)

    def run_sos_call(self) -> None:
        """Trigger SOS flow in a background thread (non-blocking for caller)."""
        t = threading.Thread(target=self._execute_sos, daemon=True)
        t.start()

    def run_airbag_call(self) -> None:
        """Trigger airbag flow: compensate current SAGA then start high-priority call."""
        t = threading.Thread(target=self._execute_airbag, daemon=True)
        t.start()

    # ------------------------------------------------------------------ #
    # Private execution methods (run on background threads)               #
    # ------------------------------------------------------------------ #

    def _execute_sos(self) -> None:
        self.priority = 7
        self._start_new_saga()
        print(f"\n[Orch] === SOS SAGA START === saga={self.saga_id[:8]}")

        # Step 1: Initiate call on ECallService
        ok = self._send_and_wait_ack(
            self._binders["ecall"],
            Message(what=EventType.INITIATE_CALL, priority=self.priority,
                    saga_id=self.saga_id),
            EventType.ACK_CALL_STARTED,
        )
        if not ok or self.state == SagaState.COMPENSATING:
            return

        self.state = SagaState.ACTIVE

        # Step 2: Update UI status
        ok = self._send_and_wait_ack(
            self._binders["ui"],
            Message(what=EventType.UPDATE_STATUS, priority=self.priority,
                    saga_id=self.saga_id, obj={"status": "SOS_ACTIVE"}),
            EventType.ACK_UI,
        )
        if not ok or self.state != SagaState.ACTIVE:
            return

        # Step 3: Notify MCU
        ok = self._send_and_wait_ack(
            self._binders["mcu"],
            Message(what=EventType.NOTIFY_CALL_ACTIVE, priority=self.priority,
                    saga_id=self.saga_id),
            EventType.ACK_MCU,
        )
        if not ok or self.state != SagaState.ACTIVE:
            return

        self.state = SagaState.COMPLETED
        print(f"[Orch] === SOS SAGA COMPLETED === saga={self.saga_id[:8]}")

    def _execute_airbag(self) -> None:
        if self.state in (SagaState.INITIATING, SagaState.ACTIVE):
            print(f"\n[Orch] Airbag preempts saga={self.saga_id[:8]} (priority {self.priority} → 10)")
            self.state = SagaState.COMPENSATING
            self.compensate_graceful(reason="PREEMPTED_BY_AIRBAG")

        self.priority = 10
        self._start_new_saga()
        print(f"\n[Orch] === AIRBAG SAGA START === saga={self.saga_id[:8]}")

        ok = self._send_and_wait_ack(
            self._binders["ecall"],
            Message(what=EventType.INITIATE_CALL, priority=self.priority,
                    saga_id=self.saga_id),
            EventType.ACK_CALL_STARTED,
        )
        if not ok:
            return

        self.state = SagaState.ACTIVE

        self._send_and_wait_ack(
            self._binders["ui"],
            Message(what=EventType.UPDATE_STATUS, priority=self.priority,
                    saga_id=self.saga_id, obj={"status": "ECALL_ACTIVE"}),
            EventType.ACK_UI,
        )
        self._send_and_wait_ack(
            self._binders["mcu"],
            Message(what=EventType.NOTIFY_CALL_ACTIVE, priority=self.priority,
                    saga_id=self.saga_id),
            EventType.ACK_MCU,
        )

        self.state = SagaState.COMPLETED
        print(f"[Orch] === AIRBAG SAGA COMPLETED === saga={self.saga_id[:8]}")
```

- [ ] **Step 4: Run to verify PASS**

```
python -m pytest ecall_saga_demo/tests/test_saga_orchestration.py -v
```

Expected: `2 passed`

- [ ] **Step 5: Commit**

```bash
git add ecall_saga_demo/saga/orchestration.py ecall_saga_demo/tests/test_saga_orchestration.py
git commit -m "feat: add OrchestratorSaga with preemption and compensation"
```

---

## Task 12: ChoreographySaga — `saga/choreography.py`

**Files:**

- Create: `ecall_saga_demo/saga/choreography.py`

- [ ] **Step 1: Write `saga/choreography.py`**

```python
# ecall_saga_demo/saga/choreography.py
from __future__ import annotations

from ecall_saga_demo.core.event_bus import EventBus
from ecall_saga_demo.core.message import Message
from ecall_saga_demo.core.service_manager import ServiceManager
from ecall_saga_demo.core.state import EventType
from ecall_saga_demo.saga.base_saga import BaseSaga
from ecall_saga_demo.services.ecall_service import ECallService


class ChoreographySaga(BaseSaga):
    """
    Choreography SAGA: no central coordinator.
    ECallService reacts to events, publishes outcomes to EventBus.
    UIService and MCUService subscribe and update their own state.
    Preemption is handled by ECallService itself when it sees AIRBAG_TRIGGERED.
    """

    def __init__(self, ecall_service: ECallService) -> None:
        super().__init__()
        self._event_bus = EventBus()
        # Inject publish_fn into ECallService so it can emit to EventBus
        ecall_service.publish_fn = self._event_bus.publish
        self._ecall_service = ecall_service

    def connect_services(self) -> None:
        super().connect_services()
        self._setup_subscriptions()

    def _setup_subscriptions(self) -> None:
        """Wire EventBus subscriptions. Each service subscribes to relevant events."""
        eb = self._event_bus
        ui_binder = self._binders["ui"]
        ec_binder = self._binders["ecall"]
        mcu_binder = self._binders["mcu"]

        # SOS_REQUESTED → ECallService initiates call
        eb.subscribe(EventType.SOS_REQUESTED, ec_binder)

        # AIRBAG_TRIGGERED → ECallService handles self-compensation
        eb.subscribe(EventType.AIRBAG_TRIGGERED, ec_binder)

        # CALL_STARTED → UI and MCU update their state
        eb.subscribe(EventType.CALL_STARTED, ui_binder)
        eb.subscribe(EventType.CALL_STARTED, mcu_binder)

        # CALL_TERMINATED → UI and MCU clean up
        eb.subscribe(EventType.CALL_TERMINATED, ui_binder)
        eb.subscribe(EventType.CALL_TERMINATED, mcu_binder)

        print("[Choreo] EventBus subscriptions configured")

    def trigger_sos(self) -> None:
        """User presses SOS — publish to EventBus, ECallService reacts."""
        print(f"\n[Choreo] === SOS TRIGGER ===")
        self._event_bus.publish(Message(
            what=EventType.SOS_REQUESTED,
            priority=7,
            saga_id=self.saga_id,
        ))

    def trigger_airbag(self) -> None:
        """Airbag fires — publish to EventBus, ECallService self-compensates."""
        import uuid
        new_id = str(uuid.uuid4())
        print(f"\n[Choreo] === AIRBAG TRIGGER (saga={new_id[:8]}) ===")
        self._event_bus.publish(Message(
            what=EventType.AIRBAG_TRIGGERED,
            priority=10,
            saga_id=new_id,
        ))
```

- [ ] **Step 2: Verify import**

```
python -c "from ecall_saga_demo.saga.choreography import ChoreographySaga; print('OK')"
```

Expected: `OK`

- [ ] **Step 3: Commit**

```bash
git add ecall_saga_demo/saga/choreography.py
git commit -m "feat: add ChoreographySaga with EventBus pub/sub wiring"
```

---

## Task 13: main.py — Full Demo

**Files:**

- Create: `ecall_saga_demo/main.py`

- [ ] **Step 1: Write `main.py`**

```python
# ecall_saga_demo/main.py
import sys
import time

from ecall_saga_demo.core.service_manager import ServiceManager
from ecall_saga_demo.services.ui_service import UIService
from ecall_saga_demo.services.ecall_service import ECallService
from ecall_saga_demo.services.mcu_service import MCUService


def run_scenario(mode: str) -> None:
    print(f"\n{'='*60}")
    print(f"  SAGA Demo — mode={mode.upper()}")
    print(f"{'='*60}")

    # ------------------------------------------------------------------ #
    # 1. Start services                                                    #
    # ------------------------------------------------------------------ #
    ServiceManager.reset()
    sm = ServiceManager.get_instance()

    if mode == "orchestration":
        ui = UIService()
        ec = ECallService()          # no publish_fn in orchestration
    else:
        ui = UIService()
        ec = ECallService()          # publish_fn injected by ChoreographySaga
    mcu = MCUService()

    ui.start(); ec.start(); mcu.start()
    sm.add_service("ui", ui)
    sm.add_service("ecall", ec)
    sm.add_service("mcu", mcu)
    time.sleep(0.1)   # allow Looper threads to start

    # ------------------------------------------------------------------ #
    # 2. Run SAGA                                                          #
    # ------------------------------------------------------------------ #
    if mode == "orchestration":
        from ecall_saga_demo.saga.orchestration import OrchestratorSaga
        saga = OrchestratorSaga()
        saga.connect_services()

        print("\n--- Scenario: SOS call ---")
        saga.run_sos_call()
        time.sleep(0.4)

        print("\n--- Scenario: Airbag interrupt during SOS ---")
        saga.run_sos_call()
        time.sleep(0.15)
        saga.run_airbag_call()
        time.sleep(0.8)

        print("\n--- Scenario: binderDied recovery ---")
        saga.run_sos_call()
        time.sleep(0.1)
        print("[Demo] Simulating ECallService crash...")
        sm.get_service("ecall").simulate_death()
        time.sleep(0.3)
        # Re-register service (simulate process restart)
        ec2 = ECallService(); ec2.start()
        sm.add_service("ecall", ec2)
        time.sleep(2.5)   # wait for retry backoff + reconnect

    else:  # choreography
        from ecall_saga_demo.saga.choreography import ChoreographySaga
        saga = ChoreographySaga(ec)
        saga.connect_services()

        print("\n--- Scenario: SOS call (choreography) ---")
        saga.trigger_sos()
        time.sleep(0.4)

        print("\n--- Scenario: Airbag interrupt (choreography self-compensation) ---")
        saga.trigger_sos()
        time.sleep(0.15)
        saga.trigger_airbag()
        time.sleep(0.4)

    # ------------------------------------------------------------------ #
    # 3. Print final states                                                #
    # ------------------------------------------------------------------ #
    print(f"\n--- Final States ---")
    print(f"  UIService:    {ui.local_state.name}")
    print(f"  ECallService: {ec.local_state.name}")
    print(f"  MCUService:   {mcu.local_state.name}")

    saga.stop()
    ui.stop(); ec.stop(); mcu.stop()


if __name__ == "__main__":
    mode = sys.argv[1] if len(sys.argv) > 1 else "orchestration"
    if mode not in ("orchestration", "choreography"):
        print("Usage: python -m ecall_saga_demo.main [orchestration|choreography]")
        sys.exit(1)
    run_scenario(mode)
```

- [ ] **Step 2: Run orchestration scenario**

```
python -m ecall_saga_demo.main orchestration
```

Expected output (tóm tắt):

```
[ECallService] Call active → SOS_ACTIVE
[UIService] Display → SOS_ACTIVE
[MCUService] Hardware notified → SOS_ACTIVE
[Orch] === SOS SAGA COMPLETED ===
...
[Orch] Airbag preempts ...
[Saga] COMPENSATION START (graceful)
[ECallService] Call terminated
[ECallService] Call active → ECALL_ACTIVE
[Orch] === AIRBAG SAGA COMPLETED ===
```

- [ ] **Step 3: Run choreography scenario**

```
python -m ecall_saga_demo.main choreography
```

Expected: tương tự nhưng không có `[Orch]` lines, thay bằng `[Choreo]`.

- [ ] **Step 4: Run all tests**

```
python -m pytest ecall_saga_demo/tests/ -v
```

Expected: tất cả PASS.

- [ ] **Step 5: Commit**

```bash
git add ecall_saga_demo/main.py
git commit -m "feat: add main.py demo runner (orchestration + choreography)"
```

---

## Spec Coverage Check

| Spec requirement                            | Task        |
| ------------------------------------------- | ----------- |
| SAGA pattern demo — Orchestration           | Task 11     |
| SAGA pattern demo — Choreography            | Task 12     |
| Android Binder/Looper alignment             | Task 4, 5   |
| linkToDeath / binderDied / retry            | Task 5, 10  |
| ServiceManager (add/get/remove)             | Task 6      |
| Priority-aware MessageQueue                 | Task 4      |
| Compensation Variant B (Graceful)           | Task 10     |
| Compensation Variant C (Timeout, reference) | Task 10     |
| State consistency via EventBus              | Task 7, 12  |
| Airbag preempts SOS scenario                | Task 11, 12 |
| binderDied recovery scenario                | Task 10, 13 |
| Runnable demo with log output               | Task 13     |
