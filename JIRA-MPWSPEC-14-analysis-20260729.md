---
JIRA System: Toyota JIRA
Issue Key: MPWSPEC-14
Analysis Date: 2026-07-29
Language: Vietnamese
---

# MPWSPEC-14 — Phân tích JIRA Ticket

## 📋 PHẦN 1: THÔNG TIN CƠ BẢN

| Field               | Value |
|---------------------|-------|
| JIRA System         | Toyota JIRA (toyota-11f.rickcloud.jp) |
| Ticket ID           | MPWSPEC-14 |
| Tiêu đề             | Review and Comments on Security Requirements for Entry Point Aggregation Alternative |
| Trạng thái          | ⏳ 進行中 (In Progress) |
| Priority            | Medium |
| Loại ticket         | Task |
| Label               | CS/IDS |
| Reporter            | Tatsuya Shintai (Toyota 2DED) |
| Assignee hiện tại   | Osamu Hosono (Toyota 2DED) |
| Created             | 2026-04-09 |
| Updated             | 2026-07-27 |
| Due Date (lần 2)    | 2026-05-27 (đã gia hạn từ 2026-04-22) |

> **Linked Ticket**: MPWSPEC-282 (Ethernet VLAN separation), MPWSPEC-51 (DCE container feasibility)

---

## 🎯 PHẦN 2: MÔ TẢ VẤN ĐỀ & PHÂN TÍCH NGUYÊN NHÂN

### Vấn đề chính

Ticket này là **spec QA ticket** (không phải bug report), được Toyota 2DED tạo ra để yêu cầu LGE review và xác nhận khả năng đáp ứng các **yêu cầu bảo mật liên quan đến Entry Point (EP) Aggregation Alternative** trên platform MPW ePF (26BEV/24DCM).

Toyota yêu cầu LGE đối chiếu kiến trúc hiện tại với các tài liệu:

1. **多層分離要件書 (MLS)** — `SEC-ePF-MLS-REQ-SPEC-a02-00-a`:
   - `MLSREQ_00008~00016`: Phân tách truy cập register, memory, HSM, giao diện in-vehicle/out-vehicle

2. **メッセージフィルタリング要求仕様書 (Message Filtering)**:
   - `MFGREQ_00006`: SID Filtering cho diagnostic requests không hợp lệ

3. **パーティション間通信 セキュリティ対策要求仕様書 (Inter-Partition Communication Security)**

### Môi trường / Platform

- **Platform**: MPW ePF (26BEV-DCM, 24DCM)
- **Kiến trúc**: MCU + SoC (Linux), phân tách EP Container vs Internal Container bằng LXC
- **Mô hình xe**: Low (MM), Mid, High (C-DC)

### Các điểm kỹ thuật chính cần xác nhận

1. **SID Filtering**: Lọc các SID không hợp lệ từ EP area đến On Board Client: `SID 0x10, 0x11, 0x28, 0x34, 0x85`
2. **Vị trí On Board Client**: Phải đặt ở **Internal Container** (không được đặt ở EP Container vì rủi ro bị tamper)
3. **Phân tách Remote Diag**: Phải tách thành **EP Remote Diag** (server-side) + **Internal Remote Diag** (MM/OTA-side)
4. **Ethernet separation**: Phân tách VLAN10 thành EP Container VLAN và Internal Container VLAN

### Kết quả mong đợi vs Thực tế

| | Kết quả mong đợi |
|--|--|
| LGE phản hồi | Xác nhận hoặc nêu vấn đề với từng yêu cầu bảo mật |
| Kiến trúc | Đề xuất architecture thỏa mãn EP separation requirements |
| Spec clarification | Làm rõ các điểm mơ hồ trong spec để Toyota cập nhật tài liệu |

---

## 📈 PHẦN 3: TIMELINE CHI TIẾT

| Date | Time (JST) | Event | Action By | Owner | Status |
|------|------------|-------|-----------|-------|--------|
| 2026-04-09 | 14:23 | 🆕 Ticket tạo, yêu cầu LGE review các security spec | Tatsuya Shintai (TMC) | SOOHYUN JUNG (LGE) | To Do |
| 2026-04-09 | 16:34 | Label `CS/IDS` được thêm | Tatsuya Shintai | — | — |
| 2026-04-09 | 18:27 | 🔍 LGE comment: hỏi về filtering path cho MFGREQ_00006 | SOOHYUN JUNG (LGE) | Tatsuya Shintai | — |
| 2026-04-10 | 18:00 | 🔵 TMC giải thích SID Filter chỉ áp dụng cho EP→OBC requests bất hợp lệ | Tatsuya Shintai | LGE | 進行中 |
| 2026-04-13 | — | 🔍 LGE tiếp tục hỏi về MFGREQ_00002, tiêu chí "không hợp lệ" | SOOHYUN JUNG (LGE) | Tatsuya Shintai | — |
| 2026-04-13 | 14:44 | 🔵 TMC làm rõ: MFGREQ_00002 không áp dụng cho DCM | Tatsuya Shintai | LGE | — |
| 2026-04-13 | 21:30 | 🔍 LGE hỏi về Ethernet của MM/C-DC trong EP area | SOOHYUN JUNG (LGE) | Tatsuya Shintai | — |
| 2026-04-14 | 08:13 | ⏳ TMC cần hỏi Security team về vấn đề này | Tatsuya Shintai | TMC Security | — |
| 2026-04-14 | 19:25 | 🔵 TMC cập nhật sơ đồ hệ thống diag (3 luồng: Remote/Tool/C-DC) | Tatsuya Shintai | LGE | — |
| 2026-04-14 | 20:11 | 🔍 LGE đề xuất đặt SID Filter ở MCU; hỏi về Remote Diag placement | SOOHYUN JUNG (LGE) | Tatsuya Shintai | — |
| 2026-04-15–17 | — | 🔍 Thảo luận OTA repro SID (SID 0x10/sub 0x02, SID 0x36) | Both | — | — |
| 2026-04-17 | 11:11 | 🟡 LGE đề xuất dùng Secure Storage key + Signature cho OBC authentication | SOOHYUN JUNG (LGE) | Tatsuya Shintai | — |
| 2026-04-17 | 12:21 | 🟢 LGE xác nhận sơ đồ MCU (Internal) / SoC (EP) của TMC | SOOHYUN JUNG (LGE) | TMC Security | — |
| 2026-04-21 | 08:52 | ⬆️ **TMC Security xác nhận**: On Board Client KHÔNG được đặt ở EP area | Tatsuya Shintai | LGE | — |
| 2026-04-21 | 18:55 | 🔵 TMC chia sẻ sơ đồ cập nhật Diag + OTA repro | Tatsuya Shintai | LGE | — |
| 2026-04-23 | 10:26 | 🔵 TMC hỏi về lọc SID 0x10/sub 0x02 qua Remote Diag và SID Filter | Tatsuya Shintai | LGE | — |
| 2026-04-23 | 19:25 | 🟣 TMC phát hành PDF `EP分離_20260423.pdf` sau khi Security team đồng ý | Tatsuya Shintai | LGE | — |
| 2026-04-24 | 08:24 | 🔍 TMC hỏi về phân chia Ethernet theo mục đích (센타/App/OTA) | Tatsuya Shintai | LGE | — |
| 2026-04-28 | 14:03 | 🟢 **LGE đề xuất kiến trúc ban đầu**: EP Container + Internal Container với LXC | SOOHYUN JUNG (LGE) | Tatsuya Shintai | — |
| 2026-05-11 | 21:39 | 🔍 LGE hỏi về Remote Diag/OBC trong Mid/High model | SOOHYUN JUNG (LGE) | Tatsuya Shintai | — |
| 2026-05-14 | 18:20 | 🔵 TMC xác nhận: Remote Diag/OBC dùng chung cho Low/Mid/High, chỉ dùng khi self-repro | Tatsuya Shintai | LGE | — |
| 2026-05-19 | 10:30 | 🟣 **Meeting**: LGE chia sẻ minutes + PPT `EPContainer_Archi_260513.pptx` | SOOHYUN JUNG (LGE) | — | — |
| 2026-05-19 | 13:55 | 🔵 TMC chia sẻ notes meeting 3DEX (Container OK, UDS/Shared Memory có điều kiện) | Tatsuya Shintai | LGE | — |
| 2026-05-20 | 17:46 | 🟡 TMC nêu vấn đề: LGE không phân biệt được Remote Diag SID 0x10/sub0x02 và OTA repro | Tatsuya Shintai | LGE | — |
| 2026-05-20 | 19:20 | 🟢 **LGE đề xuất kiến trúc chính thức**: Tách Remote Diag thành 2 phần (EP + Internal) — `EPContainer_Archi_260520.pptx` | SOOHYUN JUNG (LGE) | Tatsuya Shintai | — |
| 2026-05-21 | 14:47 | ✅ **TMC xác nhận kiến trúc LGE thỏa mãn EP separation** | Tatsuya Shintai | — | — |
| 2026-05-21 | 14:49 | ➡️ Assign sang Saito Hiroki (46F - Remote Diag spec team) | Tatsuya Shintai | 斎藤 寛樹(46F) | — |
| 2026-05-21 | 20:54 | 🔍 Sano-san (Toyota Remote Diag spec) hỏi về CommMgr trong EP container | 佐野 智(2DED) | LGE | — |
| 2026-05-22 | — | 🔍 Thảo luận CommMgr split, OTA arbitration trong kiến trúc mới | Both | — | — |
| 2026-05-26 | 11:27 | 🟡 LGE (Gwansu Shin) đề xuất OBC quản lý arbitration, Internal RemoteDiag riêng | Gwansu Shin (LGE) | Toyota Spec Team | — |
| 2026-05-29 | 09:32 | 🔍 Toyota Sano đề xuất phương án thay thế (ít thay đổi hơn) | 佐野 智(2DED) | LGE | — |
| 2026-05-29 | 11:49 | ❌ **LGE từ chối phương án Sano**: vi phạm CyberSecurity requirements | Gwansu Shin (LGE) | Toyota | — |
| 2026-06-02 | 08:13 | 🟣 **TMC phát hành Design Guidance** từ 3DEX: `SEC-ePF-MILS-CMN-SEP-GUD2` & `AC-GUD2` | Tatsuya Shintai | LGE | — |
| 2026-06-04 | 17:16 | 🔵 TMC cập nhật architecture `EP分離_20260604.pptx` | Tatsuya Shintai | LGE | — |
| 2026-06-05 | 11:36 | 🔍 LGE yêu cầu tài liệu tiếng Anh; TMC từ chối, yêu cầu LGE tự dịch | LGE/TMC | — | — |
| 2026-06-08 | 15:11 | 🔵 TMC cập nhật `EP分離_20260608.pptx` (bỏ HOST Service) | Tatsuya Shintai | LGE | — |
| 2026-06-10 | 12:51 | 🔍 LGE hỏi về DCE feasibility trong container → tham chiếu MPWSPEC-51 | SOOHYUN JUNG (LGE) | — | — |
| 2026-06-17 | — | ➡️ MPWSPEC-51 được tạo cho DCE discussion | Minseung Kim (LGE) | — | — |
| 2026-06-18 | 08:41 | 🔵 TMC xác nhận Ethernet separation requirements (VLAN, logical ok) | Tatsuya Shintai | LGE | — |
| 2026-06-18 | 15:44 | 🔵 TMC xác nhận: TMC lo VLAN splitting, LGE lo VLAN tag protection | Tatsuya Shintai | LGE | — |
| 2026-06-22–26 | — | 🔍 Thảo luận về SID filter scope, OTA path qua VLAN | Both | — | — |
| 2026-07-02 | 11:02 | ➡️ Ethernet VLAN separation moved to MPWSPEC-282 | Tatsuya Shintai | — | — |
| 2026-07-10 | 18:19 | 🟣 Hosono chia sẻ draft Remote Diag spec cho EP separation | Osamu Hosono (TMC) | LGE | — |
| 2026-07-27 | 14:34 | ✅ **LGE xác nhận không còn câu hỏi về draft spec** | Gwansu Shin (LGE) | — | — |

**Tổng kết Timeline**:
- Số lần assign qua lại: >15 lần (chủ yếu giữa Tatsuya Shintai và SOOHYUN JUNG)
- Tổng thời gian: ~110 ngày (2026-04-09 → 2026-07-27)
- Tổ chức tham gia: Toyota 2DED (Shintai, Hosono, Sano), Toyota 3DEX (Moriya-san), Toyota 46F (Saito), LGE (Jung, Gwansu Shin, Minseung Kim)
- Assignment flow: `TMC → LGE → TMC → LGE → ... → TMC 2DED (Hosono)` hiện tại

---

## 💬 PHẦN 4: DỊCH VÀ PHÂN TÍCH COMMENT

---

**Comment 1** — SOOHYUN JUNG, LGE (2026-04-09)  
🔵 _[Investigation/LGE]_

> _Shintai-san, "多層分離要件書" và "パーティション間通信 セキュリティ対策要求仕様書" sẽ được xem xét nội bộ. Trước tiên, tôi có câu hỏi về MFGREQ_00006 trong tài liệu Message Filtering:_
> _1. Tin nhắn diag đến DCM qua 2 đường: bên ngoài xe (Remote Diag) và bên trong xe (Global CAN 24DCM hoặc Ethernet 26BEV). Đường nào là đối tượng filtering?_
> _2. Có cách nào để tắt filtering không?_

**Phân tích**: LGE bắt đầu với câu hỏi xác định phạm vi áp dụng — hợp lý vì spec không nêu rõ "đường nào". Đây là điểm mơ hồ cần làm rõ sớm.

---

**Comment 2** — Tatsuya Shintai, Toyota (2026-04-10)  
🔵 _[Investigation/Toyota]_

> _Message Filtering spec KHÔNG filter các diag request hợp lệ (①Remote Diag 24DCM, ②Global-CAN 24DCM, ③Ethernet 26BEV-DCM). [SID Filter] trong hình lọc các diag request không hợp lệ (SID10,11,28,34,85) từ EP area đến [On Board Client]. Không có cơ chế tắt filter vì mục đích là lọc request bất hợp lệ._

**Phân tích**: Làm rõ quan trọng — SID Filter chỉ nhắm đến traffic từ EP area đến OBC, không ảnh hưởng đến các đường diag chính thống.

---

**Comment 3** — SOOHYUN JUNG, LGE (2026-04-13)  
🔵 _[Investigation]_

> _Q1: MFGREQ_00002 "các bus/port kết nối với công cụ diag bên ngoài xe" có bao gồm ①②③ không?_
> _Q2: Tiêu chí "không hợp lệ" của các mũi tên xanh lá là gì?_

**Phân tích**: LGE tiếp tục làm rõ scope của spec — cần thiết để xác định implementation boundary.

---

**Comment 4** — Tatsuya Shintai, Toyota (2026-04-13)  
🔵 _[Clarification]_

> _A1: MFGREQ_00002 dành cho ECU kết nối trực tiếp với DLC như CGW, DCM không liên quan._
> _A2: "Không hợp lệ" = diag request từ EP area đến [On Board Client] trong internal area. Internal OBC chỉ chấp nhận: internal Remote DIAG, CAN (DoCAN), Ethernet (DoIP/DoSoAd)._

**Phân tích**: Xác nhận rõ ràng — DCM không thuộc phạm vi MFGREQ_00002. Định nghĩa "không hợp lệ" = bất kỳ gì không đến từ 3 nguồn được phép.

---

**Comment 5** — SOOHYUN JUNG, LGE (2026-04-13)  
🔍 _[Technical Question]_

> _Cảm ơn. Câu hỏi về Multi-Layered Separation: Ethernet cho MM và C-DC có được phép tồn tại trong EP area không? Tôi hiểu đây là giao tiếp EP-to-EP trong hệ thống xe._

**Phân tích**: Đây là câu hỏi kỹ thuật quan trọng — nếu Ethernet nằm trong EP area, có thể bị tấn công từ EP container.

---

**Comment 6** — Tatsuya Shintai, Toyota (2026-04-14)  
⏳ _[Pending/Investigation]_

> _Chúng tôi chưa xem xét vấn đề này. ③ liên lạc với C-DC không chỉ là diag (DoIP/DoSoAd) mà còn là đường truyền tải thông tin C-DC-center. Tôi cần tham vấn với bộ phận Security, xin hãy đợi._

**Phân tích**: TMC thừa nhận đây là điểm chưa được xem xét — gap trong spec review ban đầu.

---

**Comment 7–11** (2026-04-14) — SOOHYUN JUNG / Tatsuya Shintai  
🔵 _[Technical Discussion — Diag Routes]_

> _Làm rõ 3 luồng diag bên ngoài: ①Remote Diag 24DCM, ②GTS Tool (DoCANで24DCM, DoIPで26BEV), ③Remote Diag 26BEV. LGE đề xuất SID Filter đặt tại MCU. Toyota xác nhận GTS tool dùng DoCANで共通化._

**Phân tích**: Quan trọng — làm rõ topology trước khi quyết định vị trí SID Filter. LGE đề xuất MCU-based filter để tận dụng MCU/SoC domain separation.

---

**Comment 12** — SOOHYUN JUNG, LGE (2026-04-17)  
🟡 _[Architecture Proposal]_

> _Đề xuất dùng Secure Storage key + Signature để phân biệt On Board Client vs các AP khác khi gửi diag request đến MCU. Software-level design chưa hoàn thiện._

**Phân tích**: Đây là đề xuất authentication mechanism — cách tiếp cận security-by-design, nhưng chưa đủ chi tiết để đánh giá feasibility.

---

**Comment 13** — Tatsuya Shintai, Toyota (2026-04-17)  
🟢 _[Architecture Confirmation]_

> _MCU = Internal area, SoC = EP area, MCU có SID Filter. Mũi tên xanh: OBC từ SoC → MCU được phép. Mũi tên đỏ: non-OBC từ SoC → MCU bị từ chối. Nếu đồng ý, tôi sẽ xác nhận với Security team._

**Phân tích**: TMC tóm tắt kiến trúc đề xuất và chuẩn bị đưa cho Security team confirm — milestone quan trọng.

---

**Comment 14** — SOOHYUN JUNG, LGE (2026-04-17)  
✅ _[Confirmation]_

> _Chính xác. Xin hãy xử lý._

---

**Comment 15** — Tatsuya Shintai, Toyota (2026-04-21)  
🔴 _[Constraint from Security]_

> _Security team xác nhận: On Board Client KHÔNG được phép đặt ở EP area. Nếu OBC ở EP area, có thể bị tamper và:_ 
> _- Ngừng xử lý OTA repro request_
> _- Gửi repro package không mong muốn đến các ECU khác_

**Phân tích**: Ràng buộc quan trọng từ Security team — đây là lý do tại sao kiến trúc ban đầu (OBC ở EP) không được chấp nhận.

---

**Comment 16** — SOOHYUN JUNG, LGE (2026-04-21)  
🔍 _[Counter-argument]_

> _Ngay cả khi OBC ở Internal area, EP area cũng có thể ngừng giao tiếp với Internal area → không hoàn toàn ngăn được. Về SID 0x36 repro package — có thể filter bằng SID 0x10._

**Phân tích**: LGE nêu điểm valid — domain separation không hoàn hảo 100%. Nhưng Security requirement vẫn phải được đáp ứng theo spec.

---

**Comment 17** — Tatsuya Shintai, Toyota (2026-04-21)  
🔵 _[Architecture Diagram Update]_

> _Chia sẻ sơ đồ Diag và OTA repro riêng biệt. Hỏi LGE về phân tách Ethernet (center/App/OTA)._

---

**Comment 18** — SOOHYUN JUNG, LGE (2026-04-21)  
🔵 _[Architecture Detail]_

> _Chi tiết flow 24DCM và 26BEV cho Diag và OTA repro: Remote Diag → OnBoardClient → CommMgr → MCU → CAN → other ECU / DiagMgr → ProgMgr._

---

**Comment 19** — Tatsuya Shintai, Toyota (2026-04-23)  
🔵 _[Security Requirement Clarification]_

> _SID 0x10/sub0x02 (Programming Session) cần filter — có thể implement qua Remote Diag hoặc SID Filter không?_

---

**Comment 20** — SOOHYUN JUNG, LGE (2026-04-23)  
✅ _[Feasibility Confirmed]_

> _Có thể implement cả qua Remote Diag lẫn SID Filter._

---

**Comment 21** — Tatsuya Shintai, Toyota (2026-04-23)  
🟣 _[Design Document Issued]_

> _Đã xác nhận với Security team. Đính kèm `EP分離_20260423.pdf` — yêu cầu LGE thiết kế kiến trúc theo cấu hình này._

**Phân tích**: Milestone quan trọng — TMC phát hành tài liệu chính thức sau khi Security team đồng ý.

---

**Comment 22** — Tatsuya Shintai, Toyota (2026-04-24)  
🔍 _[Architecture Question]_

> _Hỏi về việc phân chia Ethernet theo mục đích (center/App/OTA) trong DCM Linux — có cần qua EP Container Application không?_

---

**Comment 23** — SOOHYUN JUNG, LGE (2026-04-28)  
🟢 _[Initial Architecture Proposal]_

> _Ethernet có thể phân tách qua LXC. LGE đề xuất kiến trúc ban đầu: Remote Diag ở EP Container, On Board Client tách thành 2 phần._

**Phân tích**: Đây là kiến trúc draft đầu tiên từ LGE — sử dụng LXC container isolation.

---

**Comment 24** — SOOHYUN JUNG, LGE (2026-04-28)  
🔍 _[MLS Requirement Question]_

> _MLSREQ_00011: EP Container ↔ Internal Container chỉ được phép qua "virtual CAN" và "virtual Ethernet" — Unix Socket và Shared Memory có được không? EP-to-Host communication có hạn chế không?_

**Phân tích**: Câu hỏi critical về inter-container communication constraints.

---

**Comment 25** — SOOHYUN JUNG, LGE (2026-05-11)  
🔍 _[Model Variant Question]_

> _Trong tài liệu `EP分離_20260423.pdf`, Mid/High model DCM không có Remote DIAG và OnBoardClient. Với DoCANで giao tiếp, SID 0x36 không thể đến qua Ethernet đúng không? Xin xác nhận lại._

---

**Comment 26** — Tatsuya Shintai, Toyota (2026-05-14)  
🔵 _[Model Confirmation]_

> _Đã xác nhận với Remote Diag spec team: Remote DIAG, On Board Client, OTA Slave (CAN Client) **dùng chung cho Low/Mid/High**, chỉ dùng khi DCM self-repro trong Mid/High._

---

**Comment 27** — SOOHYUN JUNG, LGE (2026-05-14)  
🔍 _[SID Filter Concern]_

> _Nếu filter SID 0x10/sub0x02 trong Remote Diag, repro session sẽ không vào được. Xin xác nhận đây có phải yêu cầu chính xác không?_

---

**Comment 28** — Tatsuya Shintai, Toyota (2026-05-18)  
🔵 _[Filter Scope Clarification]_

> _EP Container request (mũi tên xanh) là đối tượng SID Filter. OTA repro request (mũi tên xanh lam) KHÔNG phải đối tượng SID Filter._

---

**Comment 29** — SOOHYUN JUNG, LGE (2026-05-19)  
🟣 _[Meeting Minutes & Architecture PPT]_

> _Chia sẻ minutes cuộc họp và `EPContainer_Archi_260513.pptx`. Summary: Container method OK, CommMgr split, MLSREQ về register khó với container (cần dùng LXC option/config)._

---

**Comment 30** — Tatsuya Shintai, Toyota (2026-05-19)  
🔵 _[Meeting Notes from Toyota]_

> _3DEX Moriya-san notes: Remote Diag KHÔNG cần tách khỏi EP Container; UDS/Shared Memory được phép (với điều kiện); Shared Memory dễ bị tấn công nhất._

---

**Comment 31** — Tatsuya Shintai, Toyota (2026-05-20)  
🟡 _[Critical Issue Identified]_

> _Khi Remote Diag App ở EP Container: Remote Diag SID0x10/sub0x02 cần filter, nhưng OTA repro SID0x10/sub0x02 KHÔNG được filter — **LGE không phân biệt được hai trường hợp này**._

**Phân tích**: Đây là vấn đề kỹ thuật quan trọng nhất trong ticket. SID 0x10/sub0x02 có 2 nguồn gốc khác nhau nhưng cùng form → cần architecture giải quyết ambiguity này.

---

**Comment 32** — SOOHYUN JUNG, LGE (2026-05-20)  
🟢 _[Key Architecture Proposal]_

> _LGE đề xuất tách Remote Diag thành ①Server-side EP Remote Diag + ②MM-side Internal Remote Diag. Mọi 5 SID filter áp dụng cho EP Remote Diag. Internal Remote Diag chỉ nhận từ MM và chuyển sang MCU. `EPContainer_Archi_260520.pptx`._

**Phân tích**: **Kiến trúc chốt**. Giải quyết ambiguity bằng cách tách nguồn request theo domain. Elegant solution cho bài toán SID disambiguation.

---

**Comment 33** — Tatsuya Shintai, Toyota (2026-05-21)  
✅ _[Architecture Approved]_

> _Kiến trúc và message routing trong `EPContainer_Archi_260520.pptx` thỏa mãn EP separation requirements._

---

**Comment 34** — 佐野 智 (Sano, Toyota 2DED — Remote Diag spec) (2026-05-21)  
🔍 _[Feature Inquiry]_

> _CommMgr trong EP container là gì? Quan hệ với Remote Diag?_

---

**Comment 35** — SOOHYUN JUNG, LGE (2026-05-22)  
🔵 _[CommMgr Explanation]_

> _CommMgr truyền thống = giao tiếp MCU-SoC. Trong EP separation: EP side = CommMgr, Internal side = Internal CommMgr._

---

**Comment 36** — 佐野 智 (Sano, Toyota 2DED) (2026-05-22)  
🔍 _[OTA Arbitration Concern]_

> _Remote Diag hiện tại có arbitration với OTA. Trong kiến trúc mới, OBC có chứa arbitration function không?_

---

**Comment 37** — SOOHYUN JUNG, LGE (2026-05-22)  
🔵 _[Arbitration Design]_

> _Dự định đặt "Internal Remote Diag" riêng biệt để handle arbitration với OTA, không phải OBC._

---

**Comment 38** — Gwansu Shin, LGE (2026-05-26)  
🟡 _[Architecture Decision]_

> _Priority Control (arbitration từ 13.1 và 10.1 chapter) nên do OBC đảm nhận. ECU Information List cũng cần route: RemoteDiag → OBC → Internal RemoteDiag._

---

**Comment 39** — 佐野 智 (Sano, Toyota 2DED) (2026-05-29)  
🟡 _[Alternative Architecture Proposal]_

> _Đề xuất phương án đơn giản hơn (ít rủi ro hơn với OTA arbitration). Hỏi về EP separation feasibility và development scale._

---

**Comment 40** — Gwansu Shin, LGE (2026-05-29)  
❌ _[Rejection of Alternative]_

> _Phương án của Sano không được chấp nhận vì vi phạm CyberSecurity:_
> _(1) Internal area và EP area phải có communication path tối thiểu._
> _(2) Internal Remote Diag với arbitration phải giao tiếp với nhiều EP modules → vi phạm quy tắc (1)._
> _(3) Internal area phải hoạt động độc lập khi EP tắt._

**Phân tích**: LGE có lý khi từ chối. Phương án Toyota Sano vi phạm nguyên tắc isolation cơ bản.

---

**Comment 41–46** (2026-05-29 → 2026-06-01) — 佐野/Gwansu Shin  
🔵 _[Technical Discussion]_

> _Thảo luận về EP-Internal communication constraints, NVM access trong internal area, khả năng tăng số lượng I/F nếu được định nghĩa trước._

---

**Comment 47** — Tatsuya Shintai, Toyota (2026-06-02)  
🟣 _[Design Guidance Released]_

> _3DEX Moriya-san đã phát hành Design Guidance cho EP separation alternative:_
> _- `SEC-ePF-MILS-CMN-SEP-GUD2-DOC-a00-00-a`_
> _- `SEC-ePF-MILS-CMN-AC-GUD2-DOC-a00-00-a`_
> _Yêu cầu LGE kiểm tra xem yêu cầu relaxation đã được phản ánh chưa._

---

**Comment 48** — 佐野 智 (Sano, Toyota) (2026-06-02)  
🔍 _[Constraint Clarification]_

> _Nếu I/F thêm vào được định nghĩa trước và có mục đích giới hạn thì vẫn được phép. Hỏi về "communication bị cấm" cụ thể theo LGE._

---

**Comment 49** — Tatsuya Shintai, Toyota (2026-06-04)  
🔵 _[Architecture Diagram Update]_

> _Tạo sơ đồ kiến trúc với Internal Remote DIAG đặt ở Internal Container. `EP分離_20260604.pptx`. Yêu cầu LGE xác nhận._

---

**Comment 50** — SOOHYUN JUNG, LGE (2026-06-05)  
🔍 _[Translation Request + Architecture Comment]_

> _Xin cung cấp bản tiếng Anh của tài liệu. Ethernet IF có bị block từ EP không? Container-to-container: dùng Unix Domain Socket và Binder (không dùng HOST Service)._

---

**Comment 51** — Tatsuya Shintai, Toyota (2026-06-05)  
🔵 _[Response]_

> _3DEX không có kế hoạch dịch tiếng Anh. LGE tự dịch. Ethernet ngoài xe → EP Container, trong xe → HOST, phân tách được._

---

**Comment 52** — Gwansu Shin, LGE (2026-06-05)  
🔵 _[Communication Constraint Clarification]_

> _"Hạn chế communication" = chỉ cho phép các I/F được định nghĩa trước. Remote Diag chỉ giao tiếp với OBC và Internal Remote Diag qua domain boundary._

---

**Comment 53** — Tatsuya Shintai, Toyota (2026-06-08)  
🔵 _[Architecture Updated]_

> _Đã xóa HOST Service khỏi sơ đồ theo phản hồi của LGE. `EP分離_20260608.pptx`._

---

**Comment 54** — SOOHYUN JUNG, LGE (2026-06-10)  
🔍 _[DCE Feasibility]_

> _Cần xác nhận với DCE team về feasibility trong container — refer MPWSPEC-51._

---

**Comment 55** — Minseung Kim, LGE (2026-06-17)  
🔵 _[MPWSPEC-51 Created]_

> _MPWSPEC-51 đã được tạo cho DCE discussion._

---

**Comment 56** — Tatsuya Shintai, Toyota (2026-06-17)  
🔍 _[VLAN Separation Feasibility]_

> _VLAN chỉ có VLAN10 hiện tại cho in-car. Cần tách EP Container VLAN và Internal Container VLAN — đang hỏi TMC In-car team._

---

**Comment 57** — Tatsuya Shintai, Toyota (2026-06-18)  
🔵 _[Ethernet Separation Requirements]_

> _Sau tham vấn 3DEX:_
> _① Ethernet separation: logical OK (physical preferred)_
> _② TMC tách VLAN cho EP/Internal Container_
> _③ LGE đảm bảo EP không thể thay đổi VLAN tag_

---

**Comment 58** — SOOHYUN JUNG, LGE (2026-06-22)  
🔍 _[VLAN Spec Clarification]_

> _VLAN trong 6-DCF-10-01 (out-of-vehicle spec) — spec đó có được sửa không?_

---

**Comment 59** — Osamu Hosono, Toyota 2DED (2026-06-22)  
🔵 _[Alternative for SID Filter]_

> _Đề xuất implementation: SID Filter check xem collection condition có chứa filter target SID không — nhưng complex hơn._

---

**Comment 60** — Gwansu Shin, LGE (2026-06-23)  
🔍 _[Design Confirmation Request]_

> _Xác nhận: I/F tăng lên vẫn ổn nếu được định nghĩa trước? Có đã confirm với TMC Security (Shintai-san) chưa?_

---

**Comment 61** — Tatsuya Shintai, Toyota (2026-06-23)  
🔵 _[VLAN Update]_

> _Đang confirm với In-car team về việc tách VLAN10 → EP VLAN + Internal VLAN. Nếu ổn thì TMC sửa spec._

---

**Comment 62** — Tatsuya Shintai, Toyota (2026-06-24)  
🔵 _[SID Filter Clarification]_

> _I/F được định nghĩa trước và có mục đích cụ thể thì được phép. SID Filter không chỉ check collection condition — cần filter **bất kỳ** diag request bất hợp lệ từ EP Container._

---

**Comment 63** — Osamu Hosono, Toyota 2DED (2026-06-25)  
✅ _[Acknowledgment]_

> _Cảm ơn. Sẽ sửa implementation proposal._

---

**Comment 64** — Osamu Hosono (2026-06-26)  
🔍 _[Route Clarification]_

> _Diag từ EP Container → Internal Container đi qua RemoteDiag rồi mới đến OBC — không có direct path EP → OBC. SID Filter áp dụng cho route nào cụ thể?_

---

**Comment 65** — Tatsuya Shintai, Toyota (2026-06-26)  
🔵 _[Route Confirmation]_

> _Theo `EP分離_20260608.pptx` DIAG Flow: EP Remote Diag → Internal Remote Diag KHÔNG phải SID Filter target (trừ SID 0x10/session 0x02). Các request từ EP khác mới là SID Filter target._

---

**Comment 66** — Osamu Hosono (2026-06-26)  
🔍 _[MM Diag Filter Concern]_

> _Gwansu Shin nói MM request cũng cần qua SID Filter? Nhưng MMからは đến Internal VLAN nên không phải filter target đúng không?_

---

**Comment 67** — Tatsuya Shintai, Toyota (2026-06-26)  
🔵 _[VLAN-Based Separation]_

> _OTA Master trong MM/C-DC thuộc internal domain. VLAN phân tách theo EP/Internal → OTA request từ MM đến Internal Container VLAN → bypass SID Filter._

---

**Comment 68** — Osamu Hosono (2026-06-26)  
✅ _[Misunderstanding Resolved]_

> _Xin lỗi về confusion. Gwansu Shin cũng đã hiểu nhầm — đã clarify nội bộ._

---

**Comment 69** — Tatsuya Shintai, Toyota (2026-07-02)  
➡️ _[Scope Split]_

> _Ethernet VLAN separation → chuyển sang MPWSPEC-282._

---

**Comment 70** — Osamu Hosono, Toyota 2DED (2026-07-10)  
🟣 _[Draft Spec Shared]_

> _Chia sẻ draft Remote Diag functional spec cho EP separation: `(Draft)(MPW-EP)24DCM Remote Diagnostics Application Functional Requirement Specification_rdg24-01.07.00.docx`_

---

**Comment 71** — Gwansu Shin, LGE (2026-07-27)  
✅ _[LGE Final Confirmation]_

> _Cảm ơn vì đã chia sẻ draft spec sớm. Hiện tại LGE không có câu hỏi thêm về functional spec._

**Phân tích**: **Comment cuối** — LGE confirm draft spec OK. Ticket sẽ tiếp tục theo dõi các implementation actions.

---

## ✅ PHẦN 5: TÓM TẮT

Ticket MPWSPEC-14 là **Spec QA ticket** yêu cầu LGE xác nhận tính khả thi của **Entry Point (EP) Separation Architecture** trên platform MPW ePF, theo bộ security requirements của Toyota (MLSREQ, MFGREQ). Vấn đề cốt lõi là: làm thế nào phân tách EP Container và Internal Container sao cho On Board Client (xử lý OTA repro) ở internal domain được bảo vệ khỏi tamper từ EP area, trong khi Remote Diag (server-side) vẫn hoạt động bình thường ở EP. Sau nhiều vòng thảo luận, LGE đề xuất **tách Remote Diag thành 2 phần** (EP Remote Diag + Internal Remote Diag) và Toyota đã xác nhận kiến trúc này thỏa mãn EP separation requirements vào 2026-05-21. Ticket hiện đang ở giai đoạn refinement với draft spec Remote Diag đã được TMC chia sẻ và LGE xác nhận không còn câu hỏi (2026-07-27); các subtopic (Ethernet VLAN, DCE container) đã được tách thành MPWSPEC-282 và MPWSPEC-51.

---

## 🔗 PHẦN 6: LINKS & REFERENCES

| Type | Reference |
|------|-----------|
| Ticket URL | https://toyota-11f.rickcloud.jp/jira/browse/MPWSPEC-14 |
| Related Ticket | MPWSPEC-282 — Ethernet VLAN Separation for EP Requirement |
| Related Ticket | MPWSPEC-51 — DCE Container Feasibility |
| Design Guidance | SEC-ePF-MILS-CMN-SEP-GUD2-DOC-a00-00-a (準) |
| Design Guidance | SEC-ePF-MILS-CMN-AC-GUD2-DOC-a00-00-a (準) |
| Architecture PPT | EPContainer_Archi_260520.pptx (LGE — approved architecture) |
| Architecture PPT | EP分離_20260608.pptx (Toyota — confirmed architecture) |
| Remote Diag Spec | (Draft)(MPW-EP)24DCM Remote Diagnostics Application Functional Requirement Specification_rdg24-01.07.00.docx |
| Security Spec | SEC-ePF-MLS-REQ-SPEC-a02-00-a (Multi-Layered Separation) |
| Security Req | MLSREQ_00008~00016 (register, memory, HSM, in-vehicle/out-vehicle interface separation) |
| Security Req | MFGREQ_00006 (SID Filtering: 0x10, 0x11, 0x28, 0x34, 0x85) |
| SPO Folder | https://toyotajp.sharepoint.com/sites/msspo_24dcm_development/MPWSPEC-14 |
