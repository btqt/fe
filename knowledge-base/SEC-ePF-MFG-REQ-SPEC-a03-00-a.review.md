# Review: SEC-ePF-MFG-REQ-SPEC-a03-00-a vs. Kiến trúc TCU Hiện tại

Tài liệu này đánh giá các điểm trong spec _In-Vehicle Network Requirements Specification of Message Filtering_ (SEC-ePF-MFG-REQ-SPEC-a03-00-a, Nov. 28, 2025) có tác động đến kiến trúc TCU được mô tả trong [ArchitectureChange.md](./ArchitectureChange.md).

---

## Phần 1: Gap Analysis

### 1.1 Tổng quan trạng thái

| Trạng thái                                     | Số lượng requirement |
| ---------------------------------------------- | -------------------- |
| 🔴 Missing — Chưa có trong kiến trúc           | 15                   |
| 🟡 Partial — Có concept nhưng chưa đủ chi tiết | 20                   |
| ⚪ N/A — Tool-side, ngoài scope TCU            | 8                    |

### 1.2 Gap Chi Tiết

#### GAP-01: Message Filter chưa được định nghĩa (🔴 Critical)

**Spec yêu cầu:** MFGREQ_00001–00003, 00057–00061 định nghĩa routing rules cụ thể cho 6 loại traffic (control, external tool, diagnostic request, diagnostic response) giữa các bus types. Trong đó, MFGREQ_00058–00061 là các requirement **mới hoàn toàn** được thêm vào version a03-00-a.

**Kiến trúc hiện tại:** `ArchitectureChange.md` Section 4.3 chỉ ghi `Filter (chi tiết chưa xác định - TBD)` cho `InternalOnBoardClient`. Section 7, OQ-2 đang pending CyberSecurity team.

**Tác động:** Đây là core security function của Internal Container. Thiếu spec này, `InternalOnBoardClient` không thể được implement đúng cách.

**Gap chi tiết:**

- 6 routing rules trong Table 4-1 của spec chưa được ánh xạ vào component nào trong kiến trúc
- Logic xử lý WriteOnce (MFGREQ_00057) chưa được đề cập
- Routing rules cho In-Vehicle Bus (MFGREQ_00058–00061 — version mới a03) hoàn toàn thiếu

---

#### GAP-02: Diagnostic Filtering SID list chưa được đưa vào kiến trúc (🔴 Critical)

**Spec yêu cầu:** MFGREQ_00006 liệt kê 5 SID cụ thể cần filter (0x10, 0x11, 0x28, 0x34, 0x85). MFGREQ_00007 định nghĩa điều kiện deactivate (center authentication). MFGREQ_00016 định nghĩa điều kiện reactivate.

**Kiến trúc hiện tại:** OQ-2 trong Section 7 ghi "Depends on CyberSecurity team" — thông tin thực tế đã có trong spec nhưng chưa được integrate vào kiến trúc.

**Tác động:** `InternalOnBoardClient` cần implement state machine với 2 states (Active/Deactivated) và trigger từ center authentication events. OQ-2 **đã được đóng** bởi spec này.

---

#### GAP-03: Logging Filtering hoàn toàn thiếu (🔴 Critical)

**Spec yêu cầu:** MFGREQ_00008–00010, 00017 định nghĩa logging filtering — discard control messages gửi đến DLC bus, deactivate khi Data Logger Tool authentication thành công, reactivate khi session interrupt.

**Kiến trúc hiện tại:** `ArchitectureChange.md` không có bất kỳ đề cập nào về logging filtering. `InternalCommMgr` chưa được assign trách nhiệm này.

**Tác động:** Đây là requirement mới hoàn toàn cho `InternalCommMgr`. Cần:

- Logic kiểm tra destination bus trước khi forward messages
- State management: Logging Filtering Active/Deactivated
- Trigger từ Data Logger Tool authentication events (offline auth success/session interrupt)

---

#### GAP-04: Data Logger Tool Authentication chưa được tích hợp (🟡 Partial)

**Spec yêu cầu:** MFGREQ_00018–00056 định nghĩa đầy đủ online/offline authentication flow giữa ECU, Data Logger Tool và Center. Bao gồm:

- Online auth: RSASSA-PKCS1_v1_5, 3072 bit, SHA-256
- Offline auth: AES128 ECB, 128 bit, valid count = 600
- Logger Authentication Mode: Prototype (`0x00`) vs Production (`0x01`)
- Các UDS message format cụ thể (SID 0x22, 0x27, 0x31)

**Kiến trúc hiện tại:** `OnBoardClient` được đề cập là nhận Authorized Communication nhưng không có detail về authentication protocol. `ArchitectureChange.md` Section 5.1 mô tả giao tiếp EP → Internal nhưng không có authentication flow.

**Tác động:** `OnBoardClient` (EP Container) cần update đáng kể để implement UDS-based authentication protocol (~20 MFGREQ). `InternalCommMgr` cần nhận trigger deactivation của logging filtering khi offline authentication thành công.

---

#### GAP-05: In-Vehicle Bus Routing Rules chưa được tích hợp (🔴 Missing)

**Spec yêu cầu:** MFGREQ_00058–00061 (mới trong version a03-00-a) định nghĩa routing rules giữa các In-Vehicle Bus:

- Không route diagnostic request từ non-Diagnostic Client Bus đến các In-Vehicle Bus khác
- Không route diagnostic response từ In-Vehicle Bus trừ Diagnostic Client Bus
- Không route External tool message giữa các In-Vehicle Bus
- Route Control messages theo routing map được xác định trước

**Kiến trúc hiện tại:** `ArchitectureChange.md` tập trung vào boundary EP Container ↔ Internal Container, không đề cập routing rules giữa các In-Vehicle Bus.

**Tác động:** Cần xác định component trong Internal Container enforce các inter-bus routing rules này. Khả năng cao là `InternalOnBoardClient` hoặc cần thêm một routing component riêng biệt.

---

#### GAP-06: Tamper Protection cho Filtering Config (🟡 Partial)

**Spec yêu cầu:** MFGREQ_00004 yêu cầu countermeasures chống tamper cho filtering configuration (ví dụ: HSM secure storage hoặc Secure Boot detection).

**Kiến trúc hiện tại:** `ArchitectureChange.md` đề cập `QSEECOM` trong hardware stack (AS-IS) nhưng không có explicit reference trong To-Be architecture cho secure storage của filtering config.

**Tác động:** Internal Container cần explicit secure storage mechanism cho filtering configuration. Liên kết với `AppSecurityMgr` và HSM access.

---

#### GAP-07: China GB Cryptography — Regulatory Deadline (🟡 Partial)

**Spec yêu cầu:** Notes section của spec nêu rõ: từ **January 2028**, vehicles subject to China GB phải dùng cryptographic algorithms theo VULCMN_00110 (SM4, SM2, SM3, ZUC). Thay thế cho RSASSA-PKCS1_v1_5 và AES128 ECB trong authentication flow.

**Kiến trúc hiện tại:** Không có đề cập.

**Tác động:** Cần xem xét crypto agility trong thiết kế `OnBoardClient` — đặc biệt deadline cụ thể Jan 2028 ảnh hưởng đến cả online auth (RSASSA) và offline auth (AES128 ECB). Xem thêm appendix.md Sheet A-1.

---

### 1.3 Tóm tắt tác động theo component

| Component                              | Container | GAP liên quan      | Tác động                                                                                           |
| -------------------------------------- | --------- | ------------------ | -------------------------------------------------------------------------------------------------- |
| InternalOnBoardClient (Message Filter) | Internal  | GAP-01, 02, 05, 06 | Cần implement mới: routing rules Table 9-1, SID filter Table 9-2, state machine, tamper protection |
| InternalCommMgr                        | Internal  | GAP-03             | Cần thêm: logging filtering logic, state management (Active/Deactivated), trigger từ auth events   |
| OnBoardClient                          | EP        | GAP-04, 07         | Cần update: UDS-based Data Logger auth protocol (~20 MFGREQ), crypto agility cho GB market         |
| MCU (CAN SWC)                          | MCU       | GAP-05             | Có thể cần update routing table — TBD (ghi chú "Update (TBD)" đã có trong Section 4.4)             |

---

## Phần 2: Traceability Matrix Đầy Đủ

**Legend:**

- 🔴 **Missing** — Chưa có trong kiến trúc hiện tại, cần develop mới
- 🟡 **Partial** — Có concept nhưng chưa đủ chi tiết để implement
- ⚪ **N/A** — Yêu cầu phía Data Logger Tool, ngoài scope TCU

| MFGREQ_ID    | Mô tả                                                                            | Gap Status | Container          | Component                              | Ghi chú                                                    |
| ------------ | -------------------------------------------------------------------------------- | ---------- | ------------------ | -------------------------------------- | ---------------------------------------------------------- |
| MFGREQ_00001 | Discard control messages từ buses/ports nối DLC                                  | 🔴 Missing | Internal Container | InternalOnBoardClient (Message Filter) | Core của GAP-01                                            |
| MFGREQ_00002 | Diagnostic filtering cho diagnostic request từ DLC                               | 🔴 Missing | Internal Container | InternalOnBoardClient (Message Filter) | Concept có, SID list chưa xác định — đóng bởi MFGREQ_00006 |
| MFGREQ_00003 | Logging filtering cho control messages đến DLC                                   | 🔴 Missing | Internal Container | InternalCommMgr                        | GAP-03                                                     |
| MFGREQ_00004 | Chống tamper cấu hình filtering                                                  | 🟡 Partial | Internal Container | InternalOnBoardClient / Secure storage | GAP-06; cần explicit secure storage design                 |
| MFGREQ_00005 | Mục tiêu diagnostic filtering (địa chỉ hợp pháp, tham chiếu Related Doc [2][10]) | 🔴 Missing | Internal Container | InternalOnBoardClient (Message Filter) | Cần có diagnostic address ledger tích hợp                  |
| MFGREQ_00006 | Discard SID: 0x10, 0x11, 0x28, 0x34, 0x85                                        | 🔴 Missing | Internal Container | InternalOnBoardClient (Message Filter) | Đóng OQ-2                                                  |
| MFGREQ_00007 | Deactivate diagnostic filtering khi center connection auth OK                    | 🔴 Missing | Internal Container | InternalOnBoardClient (Message Filter) | Cần nhận event từ OnBoardClient qua IPC                    |
| MFGREQ_00008 | Control messages là đối tượng logging filtering                                  | 🔴 Missing | Internal Container | InternalCommMgr                        | GAP-03                                                     |
| MFGREQ_00009 | Discard control messages đến DLC (logging filtering)                             | 🔴 Missing | Internal Container | InternalCommMgr                        | GAP-03                                                     |
| MFGREQ_00010 | Deactivate logging filtering khi Data Logger auth OK                             | 🔴 Missing | Internal Container | InternalCommMgr                        | Trigger từ offline auth success (MFGREQ_00043)             |
| MFGREQ_00016 | Reactivate diagnostic filtering trước khi auth → Unauthenticated                 | 🔴 Missing | Internal Container | InternalOnBoardClient (Message Filter) | State machine requirement                                  |
| MFGREQ_00017 | Reactivate logging filtering khi điều kiện thỏa                                  | 🔴 Missing | Internal Container | InternalCommMgr                        | Trigger từ session interrupt (MFGREQ_00044)                |
| MFGREQ_00018 | Xác định Logger Auth Mode từ public key type                                     | 🟡 Partial | EP Container       | OnBoardClient                          | Auth cơ bản có, mode tracking chưa implement               |
| MFGREQ_00019 | Respond với logger authentication mode (0xA9D0)                                  | 🟡 Partial | EP Container       | OnBoardClient                          | UDS message format chưa implement                          |
| MFGREQ_00020 | Lưu public key với tamper-prevention                                             | 🟡 Partial | EP Container       | OnBoardClient / Secure storage         | Secure storage approach chưa xác định                      |
| MFGREQ_00021 | Request format: SID 0x22, dataIdentifier 0xA9D0                                  | 🟡 Partial | EP Container       | OnBoardClient                          | Format spec chưa implement                                 |
| MFGREQ_00022 | Response format: SID 0x62, dataIdentifier 0xA9D0, mode 0x00/0x01                 | 🟡 Partial | EP Container       | OnBoardClient                          | Format spec chưa implement                                 |
| MFGREQ_00023 | Tool gửi ID/password lên Center; Center verify; gửi kết quả về tool              | ⚪ N/A     | N/A                | Data Logger Tool                       | Ngoài scope TCU                                            |
| MFGREQ_00024 | Tool xác định Center dựa theo logger auth mode                                   | ⚪ N/A     | N/A                | Data Logger Tool                       | Ngoài scope TCU                                            |
| MFGREQ_00025 | Seed request/response flow (online auth)                                         | 🟡 Partial | EP Container       | OnBoardClient                          | Flow concept có, chi tiết UDS chưa implement               |
| MFGREQ_00026 | Seed request: SID 0x27, Security Level 4                                         | 🟡 Partial | EP Container       | OnBoardClient                          | Format chưa implement                                      |
| MFGREQ_00027 | Seed request giữa tool và Center                                                 | ⚪ N/A     | N/A                | Data Logger Tool                       | Ngoài scope TCU                                            |
| MFGREQ_00028 | Signature gen (Center): RSASSA-PKCS1_v1_5, SHA-256; ECU verify                   | 🟡 Partial | EP Container       | OnBoardClient                          | Crypto spec chưa được map vào component                    |
| MFGREQ_00029 | Key transmission: SID 0x27, Security Level 4                                     | 🟡 Partial | EP Container       | OnBoardClient                          | Format chưa implement                                      |
| MFGREQ_00030 | Key transmission giữa tool và Center                                             | ⚪ N/A     | N/A                | Data Logger Tool                       | Ngoài scope TCU                                            |
| MFGREQ_00031 | Generate offline auth key; valid count = 600                                     | 🟡 Partial | EP Container       | OnBoardClient                          | Count management chưa có                                   |
| MFGREQ_00032 | Request generate offline key: RoutineControl 0x31, routineIdentifier 0xD9D0      | 🟡 Partial | EP Container       | OnBoardClient                          | Format chưa implement                                      |
| MFGREQ_00033 | Response generate offline key: SID 0x71, routineInfo 0x02, key 16 bytes          | 🟡 Partial | EP Container       | OnBoardClient                          | Format chưa implement                                      |
| MFGREQ_00034 | Lưu offline key per VULCMN_01700–01702                                           | 🟡 Partial | EP Container       | OnBoardClient / Secure storage         | Secure storage chưa xác định                               |
| MFGREQ_00035 | Lưu valid offline auth count với tamper protection                               | 🟡 Partial | EP Container       | OnBoardClient / Secure storage         | Tamper protection approach chưa xác định                   |
| MFGREQ_00036 | Tool lưu offline key securely                                                    | ⚪ N/A     | N/A                | Data Logger Tool                       | Ngoài scope TCU                                            |
| MFGREQ_00038 | Seed request/response (offline auth); check Valid State                          | 🟡 Partial | EP Container       | OnBoardClient                          | Valid State check chưa có                                  |
| MFGREQ_00039 | Seed request offline: SID 0x27, Security Level 3 (khác online Level 4)           | 🟡 Partial | EP Container       | OnBoardClient                          | Format chưa implement                                      |
| MFGREQ_00040 | Key value gen & verify: AES128 ECB                                               | 🟡 Partial | EP Container       | OnBoardClient                          | Crypto spec chưa được map vào component                    |
| MFGREQ_00041 | Key transmission offline: SID 0x27, Security Level 3                             | 🟡 Partial | EP Container       | OnBoardClient                          | Format chưa implement                                      |
| MFGREQ_00042 | Tool request control frame monitor mode                                          | ⚪ N/A     | N/A                | Data Logger Tool                       | Ngoài scope TCU                                            |
| MFGREQ_00043 | Decrement valid count; deactivate logging filtering                              | 🔴 Missing | Internal Container | InternalCommMgr                        | GAP-03; trigger từ offline auth success                    |
| MFGREQ_00044 | Session interrupt → offline Unauthenticated; reactivate logging filtering        | 🔴 Missing | Internal Container | InternalCommMgr                        | GAP-03; session management                                 |
| MFGREQ_00045 | Valid count = 0 → Invalid state                                                  | 🔴 Missing | Internal Container | InternalCommMgr                        | State machine requirement                                  |
| MFGREQ_00046 | Deactivate offline auth theo request tool (RoutineControl 0xD9D1)                | 🟡 Partial | EP Container       | OnBoardClient                          | Format chưa implement                                      |
| MFGREQ_00047 | Request deactivate offline auth: RoutineControl 0x31, 0xD9D1                     | 🟡 Partial | EP Container       | OnBoardClient                          | Format chưa implement                                      |
| MFGREQ_00048 | Response deactivate offline auth: SID 0x71, routineInfo 0x02                     | 🟡 Partial | EP Container       | OnBoardClient                          | Format chưa implement                                      |
| MFGREQ_00049 | Respond với valid offline auth count khi được query                              | 🟡 Partial | EP Container       | OnBoardClient                          | Query mechanism chưa có                                    |
| MFGREQ_00050 | Request valid count: RDBI 0x22, dataIdentifier 0xA9D1                            | 🟡 Partial | EP Container       | OnBoardClient                          | Format chưa implement                                      |
| MFGREQ_00051 | Response valid count format                                                      | 🟡 Partial | EP Container       | OnBoardClient                          | Format chưa implement                                      |
| MFGREQ_00054 | Prototype key từ Related Doc [11]; Production key từ Center                      | 🟡 Partial | EP Container       | OnBoardClient                          | Key provisioning mechanism chưa xác định                   |
| MFGREQ_00055 | Generate offline key per VULCMN_00200, 00300                                     | 🟡 Partial | EP Container       | OnBoardClient                          | VULCMN compliance chưa verify                              |
| MFGREQ_00056 | Online auth → Unauthenticated khi session interrupt                              | 🔴 Missing | EP Container       | OnBoardClient                          | Session interrupt detection chưa có                        |
| MFGREQ_00057 | Discard External tool messages từ DLC sau WriteOnce                              | 🔴 Missing | Internal Container | InternalOnBoardClient (Message Filter) | Cần tích hợp WriteOnce state                               |
| MFGREQ_00058 | Không route diag request từ non-Diag Client Bus đến In-Vehicle Bus khác          | 🔴 Missing | Internal Container | InternalOnBoardClient (Message Filter) | GAP-05; mới trong version a03-00-a                         |
| MFGREQ_00059 | Không route diag response từ In-Vehicle Bus trừ Diagnostic Client Bus            | 🔴 Missing | Internal Container | InternalOnBoardClient (Message Filter) | GAP-05; mới trong version a03-00-a                         |
| MFGREQ_00060 | Không route External tool message giữa các In-Vehicle Bus                        | 🔴 Missing | Internal Container | InternalOnBoardClient (Message Filter) | GAP-05; mới trong version a03-00-a                         |
| MFGREQ_00061 | Route Control messages theo routing map định sẵn                                 | 🔴 Missing | Internal Container | InternalOnBoardClient (Message Filter) | GAP-05; routing map cần xác định                           |
