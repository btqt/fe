# QA Architecture Assessment Skill — Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Tạo agent skill `qa-architecture-assessment` — đọc source code C++ automotive, đánh giá theo 4 QA (Performance, Reliability, Availability, Interoperability), xuất assessment report với design-level improvement proposals.

**Architecture:** Skill file duy nhất tại `c:\Users\phi.vu\.agents\skills\qa-architecture-assessment\SKILL.md`, kèm report template tại `assets/report-template.md`. Agent follow quy trình 5 bước: Scope Setup → Pattern Identification → QA Impact Mapping → Improvement Proposal → Report Generation.

**Tech Stack:** Markdown skill file, grep_search / file_search / read_file tools, output là Markdown report.

---

## File Structure

```
c:\Users\phi.vu\.agents\skills\qa-architecture-assessment\
├── SKILL.md                          ← Main skill instructions (tạo mới)
└── assets\
    └── report-template.md            ← Blank report template (tạo mới)
```

---

### Task 1: Tạo thư mục skill và SKILL.md chính

**Files:**

- Create: `c:\Users\phi.vu\.agents\skills\qa-architecture-assessment\SKILL.md`

- [ ] **Step 1: Tạo SKILL.md với YAML frontmatter và phần mục đích**

Tạo file với nội dung sau:

```markdown
---
name: qa-architecture-assessment
description: "Đánh giá kiến trúc module C++ automotive theo 4 QA: Performance, Reliability, Availability, Interoperability. Tìm anti-pattern từ source code, đề xuất cải tiến ở design-level. Use for: QA assessment, architecture review, improvement proposal, performance bottleneck, reliability risk, availability gap, interoperability coupling."
argument-hint: "Đường dẫn đến module cần đánh giá (ví dụ: src/PowerManager, src/BTManager)"
---

# QA Architecture Assessment

## Mục đích

Phân tích source code **C++ automotive/embedded** (AUTOSAR, Linux/Tiger, Android AAOS) để đánh giá kiến trúc hiện tại theo 4 Quality Attributes:

- **Performance** — Độ nhanh và hiệu quả sử dụng tài nguyên
- **Reliability** — Khả năng hoạt động đúng và không bị crash/leak
- **Availability** — Khả năng tiếp tục hoạt động khi có lỗi hoặc dependency fail
- **Interoperability** — Khả năng tích hợp, thay thế component, và mở rộng

Output là assessment report Markdown với:

1. **QA Scorecard** — Tổng quan sức khỏe của module theo từng QA
2. **Issue List** — Danh sách vấn đề có evidence cụ thể (file:line) và improvement proposal ở design-level
3. **Priority Matrix** — Ưu tiên xử lý dựa trên QA impact và effort

## Khi nào dùng

- Nhận module mới cần đánh giá trước khi quyết định cải tiến hướng nào
- Chuẩn bị FA assessment report cho một service/component
- Module hay xảy ra issue về performance, crash, hoặc khó tích hợp

---

## Quy trình thực hiện

### Bước 1 — Scope Setup
```

1. Nhận source path từ argument (ví dụ: src/PowerManager)
2. Dùng file_search để scan tất cả file .cpp, .h, .hpp trong path
3. Liệt kê danh sách file tìm được, ước lượng tổng LOC
4. Xác định platform/framework từ include headers:
   - AUTOSAR: #include <ara/...> hoặc <autosar/...>
   - Linux service: #include <dbus/...> hoặc <systemd/...>
   - Android HAL: #include <hardware/...> hoặc <hidl/...>
5. Ghi nhận platform để điều chỉnh pattern detection phù hợp

```

### Bước 2 — Code Reading & Pattern Identification

Đọc source files theo thứ tự: **header files (.h/.hpp) trước**, implementation files (.cpp) sau.

Với mỗi file, dùng `read_file` và `grep_search` để tìm các pattern trong **Pattern Catalog** bên dưới.

**Cách ghi nhận evidence:**
```

Pattern: Thread-per-event
File: src/EventHandler.cpp:142
Evidence: std::thread t([this, event]{ process(event); }); t.detach();

```

Chỉ flag khi có bằng chứng cụ thể — không phỏng đoán từ tên class hay comment.

### Bước 3 — QA Impact Mapping

Với mỗi pattern đã tìm được:
1. Xác định QA nào bị ảnh hưởng (một pattern có thể ảnh hưởng nhiều QA)
2. Đánh giá mức độ: **❌ Negative** (rõ ràng gây hại) hoặc **⚠️ Risk** (tiềm ẩn)
3. Mỗi pattern chỉ xuất hiện **một lần** trong danh sách issue

### Bước 4 — Improvement Proposal

Với mỗi issue có QA impact tiêu cực, tạo improvement proposal ở **design-level**:
- Mô tả **current design**: component hiện tại làm gì, flow như thế nào
- Mô tả **proposed design**: component mới, interface contract, trách nhiệm từng phần
- **Không** viết code C++ implementation — chỉ mô tả interface và data flow

### Bước 5 — Report Generation

Tạo file report tại: `docs/qa-assessment-<module-name>-<YYYY-MM-DD>.md`

Dùng template tại `assets/report-template.md` làm khung. Điền đầy đủ 3 phần:
1. QA Scorecard
2. Issue List (mỗi issue theo template ISS-XX)
3. Priority Matrix

---

## Pattern Catalog

### Performance Patterns

#### P1 — Thread-per-event
**Phát hiện:**
```

grep_search: "std::thread|pthread_create|new thread" trong .cpp files
→ Kiểm tra context: nằm trong event handler, request handler, hoặc callback không?

```
**Threshold:** Bất kỳ occurrence trong handler function

**QA Impact:** Performance ❌ (unbounded thread count), Reliability ⚠️ (detached threads không trackable)

---

#### P2 — Blocking call trong hot path
**Phát hiện:**
```

grep_search: "sleep|usleep|nanosleep|waitFor|wait_for|std::this_thread::sleep" trong .cpp
→ Kiểm tra context: nằm trong hàm xử lý event/request không?

```
**Threshold:** Bất kỳ occurrence trong event/request handler

**QA Impact:** Performance ❌ (tăng latency), Availability ⚠️ (block toàn bộ thread)

---

#### P3 — Polling loop
**Phát hiện:**
```

grep_search: "while\s*\(true\)|while\s*\(1\)" trong .cpp
→ Kiểm tra body: có sleep + check condition không? Không có condition_variable/callback?

```
**Threshold:** Bất kỳ occurrence không dùng condition_variable

**QA Impact:** Performance ❌ (CPU waste), Reliability ⚠️ (busy wait dễ miss update)

---

#### P4 — Missing move semantics
**Phát hiện:**
```

grep_search: function signatures trả về hoặc nhận std::vector/std::string by value
→ Kiểm tra: có std::move() ở call site không?

```
**Threshold:** > 3 occurrences pass-by-value không có std::move

**QA Impact:** Performance ⚠️ (unnecessary copy overhead)

---

### Reliability Patterns

#### R1 — Unchecked return value
**Phát hiện:**
```

grep_search: function calls mà return value không được assign hoặc check
Tập trung vào: open(), read(), write(), send(), recv(), connect(), system calls

```
**Threshold:** > 3 occurrences với system/IPC calls

**QA Impact:** Reliability ❌ (silent failure, undefined behavior sau đó)

---

#### R2 — Missing RAII / resource leak
**Phát hiện:**
```

grep_search: "new " (raw pointer allocation) — kiểm tra có delete tương ứng không
grep_search: "fopen|open(" — kiểm tra có fclose/close tương ứng không
grep_search: "malloc|calloc" — kiểm tra có free không

```
**Threshold:** Bất kỳ raw new/open không có matching delete/close hoặc RAII wrapper

**QA Impact:** Reliability ❌ (memory/resource leak), Availability ❌ (resource exhaustion theo thời gian)

---

#### R3 — Race condition risk
**Phát hiện:**
```

grep*search: member variables được access trong nhiều method
→ Kiểm tra: có mutex/lock_guard bảo vệ không?
grep_search: "m*|\_m|this->" member access — cross-check với thread creation (P1)

```
**Threshold:** Bất kỳ shared mutable state trong class có multiple threads

**QA Impact:** Reliability ❌ (data corruption, crash)

---

#### R4 — No retry on transient failure
**Phát hiện:**
```

grep_search: IPC/network call patterns (send, connect, dbus_send, vsomeip)
→ Kiểm tra: có retry loop, backoff, hoặc error recovery không?

```
**Threshold:** Bất kỳ critical IPC/network call không có retry

**QA Impact:** Reliability ⚠️ (single transient error = permanent failure)

---

#### R5 — Unhandled exception at boundary
**Phát hiện:**
```

grep_search: "catch" — kiểm tra catch body có rỗng không, có chỉ log không?
grep_search: service entry points (main, init, onEvent) — có try-catch bao không?

```
**Threshold:** Bất kỳ empty catch hoặc thiếu try-catch tại service boundary

**QA Impact:** Reliability ❌ (uncaught exception = process crash)

---

### Availability Patterns

#### A1 — Synchronous hard dependency không có timeout
**Phát hiện:**
```

grep_search: synchronous IPC patterns: dbus_call, someip_call, rpc_call
→ Kiểm tra tham số: có timeout value không?

```
**Threshold:** Bất kỳ blocking IPC call không có timeout

**QA Impact:** Availability ❌ (nếu dependency hang → caller hang vĩnh viễn)

---

#### A2 — No graceful degradation
**Phát hiện:**
```

Xem xét flow của service: khi dependency không available → service làm gì?
grep_search: error handling sau IPC/connect call
→ Kiểm tra: có fallback path, cached value, hoặc default behavior không?

```
**Threshold:** Bất kỳ critical dependency failure path không có fallback

**QA Impact:** Availability ❌ (single dependency failure = full service failure)

---

#### A3 — No health/watchdog mechanism
**Phát hiện:**
```

grep_search: "heartbeat|watchdog|health|keepalive|ping" — kiểm tra có implementation không
→ Nếu không tìm thấy bất kỳ health check mechanism nào trong toàn bộ service

```
**Threshold:** Không có bất kỳ health mechanism nào

**QA Impact:** Availability ⚠️ (silent failure không được detect và recover)

---

### Interoperability Patterns

#### I1 — Direct vendor SDK coupling
**Phát hiện:**
```

grep*search: vendor-specific prefixes/namespaces trong .cpp và .h
Ví dụ: Cinemo*, BT*Stack*, GSMA\_, vendor-specific types
→ Kiểm tra: gọi trực tiếp không qua interface/adapter layer?

```
**Threshold:** Bất kỳ direct vendor call không có abstraction layer

**QA Impact:** Interoperability ❌ (không thể swap vendor, khó port sang project khác)

---

#### I2 — Hard-coded protocol/format
**Phát hiện:**
```

grep_search: magic numbers trong IPC message handling
grep_search: hard-coded message ID, command code, format string không qua constant/enum

```
**Threshold:** > 5 occurrences magic number trong message handling

**QA Impact:** Interoperability ⚠️ (protocol change = scattered code changes), Reliability ⚠️

---

#### I3 — Manager-to-Manager tight coupling
**Phát hiện:**
```

grep_search: "#include" patterns trong Manager/Service class headers
→ Tìm Manager A include Manager B header trực tiếp
→ Tìm Manager A gọi method của Manager B trực tiếp (không qua interface)

```
**Threshold:** Bất kỳ direct Manager-to-Manager include/call

**QA Impact:** Interoperability ❌ (circular dependency risk, không thể test/deploy độc lập)

---

#### I4 — No interface abstraction
**Phát hiện:**
```

grep_search: constructor/method parameters nhận concrete class type
→ Kiểm tra: có abstract base class / interface (pure virtual) không?

````
**Threshold:** > 3 concrete class dependencies trong class quan trọng

**QA Impact:** Interoperability ❌ (không thể mock, không thể swap implementation)

---

## Output Format — QA Scorecard

```markdown
## QA Scorecard

| QA               | Score     | Issues Found |
|------------------|-----------|--------------|
| Performance      | ⚠️ Fair   | 3            |
| Reliability      | ❌ Poor   | 5            |
| Availability     | ✅ Good   | 1            |
| Interoperability | ❌ Poor   | 4            |

**Thang điểm:** ✅ Good (0–1 issue) | ⚠️ Fair (2–3 issues) | ❌ Poor (4+ issues)
````

---

## Output Format — Issue Block

```markdown
### [ISS-XX] <Tên ngắn gọn mô tả vấn đề>

- **File:** `path/to/file.cpp:LINE`
- **Pattern:** <Pattern ID — tên pattern>
- **QA Impact:** Performance ❌, Reliability ⚠️
- **Evidence:** `<code snippet ngắn từ file thực tế>`
- **Risk:** <Mô tả rủi ro cụ thể nếu không fix>

#### Improvement Proposal

**Current design:**
```

<Mô tả ngắn thiết kế hiện tại dạng component/flow>
Ví dụ: EventHandler → tạo std::thread mới mỗi event → thread xử lý rồi detach

```

**Proposed design:**
```

<Mô tả thiết kế mới với interface chính và trách nhiệm>
Ví dụ:
EventHandler → submit(Task) → ThreadPool
ThreadPool interface: + submit(std::function<void()> task) — thêm task vào queue + shutdown() — đợi task hiện tại xong, reject task mới
BoundedQueue (internal): - max_size: N (configurable) - backpressure: block hoặc drop+log khi full
WorkerThreads (internal): fixed-size pool, tạo lúc startup, destroy lúc shutdown

```

```

---

## Output Format — Priority Matrix

```markdown
## Priority Matrix

| Issue  | QA Impact                       | Effort | Priority |
| ------ | ------------------------------- | ------ | -------- |
| ISS-01 | Reliability ❌                  | Low    | P1       |
| ISS-02 | Performance ❌ + Reliability ⚠️ | Medium | P1       |
| ISS-05 | Interoperability ❌             | High   | P2       |

**Effort:** Low = interface refactor | Medium = new component | High = architectural restructure
**Priority:** P1 = QA ❌ + Low/Medium effort | P2 = QA ❌ + High effort hoặc QA ⚠️
```

---

## Constraints

- Chỉ flag vấn đề khi có **bằng chứng cụ thể** (file + line) — không nhận xét chung chung
- Improvement proposal ở mức **design-level** — không viết C++ implementation code
- Mỗi pattern/issue chỉ xuất hiện **một lần** trong report dù ảnh hưởng nhiều QA
- Không reference 40 FA topics — dựa trên SOLID, design patterns, automotive best practices

```

- [ ] **Step 2: Xác nhận file đã tạo thành công**

Kiểm tra file tồn tại:
```

file_search: c:\Users\phi.vu\.agents\skills\qa-architecture-assessment\SKILL.md

````
Expected: File hiện trong kết quả search.

- [ ] **Step 3: Commit**

```bash
cd c:\Users\phi.vu\.agents\skills
git add qa-architecture-assessment\SKILL.md
git commit -m "feat: add qa-architecture-assessment skill"
````

---

### Task 2: Tạo Report Template

**Files:**

- Create: `c:\Users\phi.vu\.agents\skills\qa-architecture-assessment\assets\report-template.md`

- [ ] **Step 1: Tạo report-template.md**

Tạo file với nội dung sau:

```markdown
# QA Architecture Assessment — <Module Name>

**Date:** YYYY-MM-DD
**Module:** <đường dẫn module>
**Platform:** <AUTOSAR | Linux/Tiger | Android AAOS>
**Analyst:** GitHub Copilot (qa-architecture-assessment skill)

---

## QA Scorecard

| QA               | Score | Issues Found |
| ---------------- | ----- | ------------ |
| Performance      |       |              |
| Reliability      |       |              |
| Availability     |       |              |
| Interoperability |       |              |

**Thang điểm:** ✅ Good (0–1 issue) | ⚠️ Fair (2–3 issues) | ❌ Poor (4+ issues)

---

## Issue List

### [ISS-01] <Tên vấn đề>

- **File:** `path/to/file.cpp:LINE`
- **Pattern:** <Pattern ID — tên>
- **QA Impact:** <QA> ❌
- **Evidence:** `<code snippet>`
- **Risk:** <mô tả rủi ro>

#### Improvement Proposal

**Current design:**
```

<mô tả current>

```

**Proposed design:**
```

<mô tả proposed>

```

---

## Priority Matrix

| Issue  | QA Impact | Effort | Priority |
|--------|-----------|--------|----------|
|        |           |        |          |

**Effort:** Low = interface refactor | Medium = new component | High = architectural restructure
**Priority:** P1 = QA ❌ + Low/Medium effort | P2 = QA ❌ + High effort hoặc QA ⚠️
```

- [ ] **Step 2: Commit**

```bash
cd c:\Users\phi.vu\.agents\skills
git add qa-architecture-assessment\assets\report-template.md
git commit -m "feat: add report template for qa-architecture-assessment skill"
```

---

### Task 3: Validation — Chạy thử skill trên module thực tế

**Mục đích:** Xác nhận skill hoạt động đúng trước khi dùng chính thức.

- [ ] **Step 1: Chọn module test**

Invoke skill với một module C++ có sẵn trong workspace hiện tại (nếu có), hoặc dùng một trong các module được mention trong FA topics như `PowerManager`, `BTManager`, `FactoryService`.

Cách invoke:

```
@qa-architecture-assessment src/PowerManager
```

- [ ] **Step 2: Verify output structure**

Kiểm tra report được tạo có đủ 3 phần:

- [ ] QA Scorecard với 4 dòng (Performance / Reliability / Availability / Interoperability)
- [ ] Ít nhất 1 Issue block có đủ fields: File, Pattern, QA Impact, Evidence, Risk, Improvement Proposal
- [ ] Priority Matrix có ít nhất 1 dòng

- [ ] **Step 3: Verify evidence quality**

Mỗi issue phải có:

- [ ] File path + line number cụ thể (không phải "somewhere in the code")
- [ ] Code snippet ngắn từ file thực tế (không phải generic example)
- [ ] Improvement proposal mô tả interface/component (không phải code implementation)

- [ ] **Step 4: Fix nếu cần, commit final**

```bash
cd c:\Users\phi.vu\.agents\skills
git add -A qa-architecture-assessment\
git commit -m "fix: qa-architecture-assessment skill validation fixes"
```
