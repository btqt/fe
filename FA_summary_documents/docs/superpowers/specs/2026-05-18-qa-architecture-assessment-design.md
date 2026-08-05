# QA Architecture Assessment Skill — Design Spec

**Date:** 2026-05-18
**Status:** Approved

---

## Goal

Thiết kế một agent skill độc lập để phân tích source code của một module C++ automotive/embedded, đánh giá kiến trúc hiện tại theo 4 Quality Attributes (Performance, Reliability, Availability, Interoperability), và đề xuất cải tiến ở mức design-level.

---

## Scope

- **Input:** Đường dẫn đến module/folder source code
- **Output:** Assessment report Markdown với QA scorecard, danh sách issue có evidence cụ thể, và improvement proposal ở mức class/component diagram
- **Không bao gồm:** Code snippet implementation, reference đến 40 FA topics, integration với skill khác
- **Platform target:** C++ automotive/embedded (AUTOSAR, Linux/Tiger, Android AAOS)

---

## Skill Metadata

```yaml
name: qa-architecture-assessment
description: "Đánh giá kiến trúc module C++ automotive theo 4 QA: Performance, Reliability, Availability, Interoperability. Tìm anti-pattern từ source code, đề xuất cải tiến ở design-level. Use for: QA assessment, architecture review, improvement proposal, performance bottleneck, reliability risk, availability gap, interoperability coupling."
argument-hint: "Đường dẫn đến module cần đánh giá (ví dụ: src/PowerManager, src/BTManager)"
```

**Location:** `c:\Users\phi.vu\.agents\skills\qa-architecture-assessment\SKILL.md`

---

## Process Flow

```
[Bước 1] Scope Setup
  └─ Nhận source path từ argument
  └─ Scan file list (.cpp, .h, .hpp)
  └─ Xác định platform/framework nếu có thể (AUTOSAR, Linux service, Android HAL)

[Bước 2] Code Reading & Pattern Identification
  └─ Đọc source files theo thứ tự: header files trước, implementation files sau
  └─ Nhận diện các pattern/anti-pattern theo Pattern Catalog
  └─ Ghi nhận evidence: file path + line number + code snippet

[Bước 3] QA Impact Mapping
  └─ Với mỗi pattern tìm được → đánh giá impact lên từng QA liên quan
  └─ Mỗi pattern chỉ xuất hiện một lần, có thể tag nhiều QA
  └─ Chỉ flag khi có bằng chứng cụ thể từ code, không phỏng đoán

[Bước 4] Improvement Proposal
  └─ Với mỗi issue có QA impact tiêu cực → thiết kế giải pháp
  └─ Mức độ: class/component interface, không phải code snippet
  └─ Mô tả: current design → proposed design, bao gồm interface contract chính

[Bước 5] Report Generation
  └─ Tạo file: docs/qa-assessment-<module>-<YYYY-MM-DD>.md
  └─ Cấu trúc: QA Scorecard → Issue List → Priority Matrix
```

---

## Pattern Catalog

### Performance

| Pattern                          | Cách phát hiện                                                         | Threshold cảnh báo |
| -------------------------------- | ---------------------------------------------------------------------- | ------------------ |
| Thread-per-event                 | `std::thread`, `pthread_create` trong event/request handler            | Bất kỳ             |
| Blocking call trong hot path     | `sleep()`, `waitFor()`, sync IPC call trong event handler              | Bất kỳ             |
| Missing move semantics           | Copy lớn (vector/string qua function boundary) không có `std::move`    | > 3 occurrences    |
| Polling loop                     | `while(true) + sleep` thay vì notification/callback/condition_variable | Bất kỳ             |
| Repeated computation không cache | Cùng hàm tính toán nặng gọi nhiều lần với input giống nhau             | > 5 call sites     |

### Reliability

| Pattern                         | Cách phát hiện                                                    | Threshold cảnh báo |
| ------------------------------- | ----------------------------------------------------------------- | ------------------ |
| Unchecked return value          | Return value của function critical bị ignore                      | > 3 occurrences    |
| Missing RAII / resource leak    | `new` không có smart pointer, file/socket handle không được close | Bất kỳ             |
| Race condition risk             | Shared mutable state không được bảo vệ bởi mutex/lock             | Bất kỳ             |
| No retry on transient failure   | Network/IPC call không có retry logic                             | Bất kỳ             |
| Unhandled exception at boundary | `catch(...)` rỗng hoặc thiếu try-catch tại service boundary       | Bất kỳ             |

### Availability

| Pattern                      | Cách phát hiện                                            | Threshold cảnh báo |
| ---------------------------- | --------------------------------------------------------- | ------------------ |
| Synchronous hard dependency  | Service block chờ dependency không có timeout             | Bất kỳ             |
| Missing timeout              | IPC/RPC/network call không có timeout parameter           | Bất kỳ             |
| No graceful degradation      | Không có fallback path khi dependency fail                | Bất kỳ             |
| No health/watchdog mechanism | Không có heartbeat, health check, hoặc recovery sau crash | Bất kỳ             |

### Interoperability

| Pattern                           | Cách phát hiện                                                  | Threshold cảnh báo |
| --------------------------------- | --------------------------------------------------------------- | ------------------ |
| Direct vendor SDK coupling        | Gọi vendor API trực tiếp không qua interface/adapter layer      | Bất kỳ             |
| Hard-coded protocol/format        | Magic number, hard-coded message ID, format string              | > 5 occurrences    |
| Manager-to-Manager tight coupling | `#include` trực tiếp giữa Manager classes, gọi method chéo nhau | Bất kỳ             |
| No interface abstraction          | Phụ thuộc vào concrete class thay vì abstract interface         | > 3 occurrences    |

---

## Output Format

Report lưu tại: `docs/qa-assessment-<module>-<YYYY-MM-DD>.md`

### Phần 1 — QA Scorecard

```markdown
| QA               | Score   | Issues Found |
| ---------------- | ------- | ------------ |
| Performance      | ⚠️ Fair | 3            |
| Reliability      | ❌ Poor | 5            |
| Availability     | ✅ Good | 1            |
| Interoperability | ❌ Poor | 4            |
```

Thang điểm:

- ✅ **Good** — 0–1 issue
- ⚠️ **Fair** — 2–3 issues
- ❌ **Poor** — 4+ issues

### Phần 2 — Issue List

Mỗi issue theo template:

```markdown
### [ISS-XX] <Tên ngắn gọn mô tả vấn đề>

- **File:** <path/to/file.cpp:line>
- **Pattern:** <tên pattern từ catalog>
- **QA Impact:** <QA1> ❌, <QA2> ⚠️
- **Evidence:** `<code snippet ngắn>`
- **Risk:** <mô tả rủi ro cụ thể>

#### Improvement Proposal

**Current design:**
<mô tả ngắn thiết kế hiện tại, dạng component/flow>

**Proposed design:**
<mô tả thiết kế mới, bao gồm interface chính và trách nhiệm từng component>
```

### Phần 3 — Priority Matrix

```markdown
| Issue  | QA Impact           | Effort | Priority |
| ------ | ------------------- | ------ | -------- |
| ISS-XX | <QA> ❌             | Low    | P1       |
| ISS-YY | <QA1> ❌ + <QA2> ⚠️ | Medium | P2       |
```

Effort: Low (interface refactor) / Medium (new component) / High (architectural restructure)
Priority: P1 (QA ❌ + Low/Medium effort) / P2 (QA ❌ + High effort hoặc QA ⚠️)

---

## Constraints

- Agent chỉ flag vấn đề khi có **bằng chứng cụ thể** (file + line), không đưa ra nhận xét chung chung
- Improvement proposal ở mức **design-level** (interface, component, data flow) — không viết code implementation
- Mỗi pattern chỉ xuất hiện **một lần** trong report dù ảnh hưởng nhiều QA
- Không bắt buộc reference 40 FA topics; dựa trên kiến thức kiến trúc chung (SOLID, design patterns, automotive best practices)
