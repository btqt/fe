---
agent: agent
description: "Phân tích JIRA ticket (LGE hoặc Toyota JIRA) và tạo báo cáo kỹ thuật chi tiết bằng tiếng Việt. Dùng khi: phân tích ticket, dịch comment, tóm tắt root cause, tạo file phân tích."
argument-hint: "Paste JIRA URL hoặc issue key (vd: TMCBEV-4040, BEV26MON-1405)"
tools:
  [
    "vscode",
    "execute",
    "read",
    "edit",
    "search",
    "web",
    "agent",
    "lge-jira/*",
    "toyota-jira/*",
    "todo",
  ]
---

# JIRA Ticket Auto-Detection & Analysis Task

**Output Language**: Vietnamese _(thay đổi nếu cần: `English` | `Korean`)_

---

## Quy trình thực thi (tự động, không cần xác nhận)

1. **Detect** JIRA system từ URL/issue key (xem bảng bên dưới)
2. **Fetch** dữ liệu — **bắt buộc 2 lần gọi**:
   - Lần 1: `fields="*all"`, `expand="renderedFields,changelog"` → issue details + changelog
   - Lần 2: `fields="comment"`, `comment_limit=100` → all comments
3. **Generate** phân tích theo Output Format (6 phần)
4. **Save** file `JIRA-{ISSUE_KEY}-analysis-{YYYYMMDD}.md` vào workspace root
5. **Confirm**: `✅ Analysis saved to JIRA-{ISSUE_KEY}-analysis-{YYYYMMDD}.md`

---

## Auto-Detection

| Input                           | Tool                              |
| ------------------------------- | --------------------------------- |
| URL chứa `toyota`               | `mcp_toyota-jira_jira_get_issue`  |
| URL chứa `lge.com`              | `mcp_lge-jira_jira_get_issue`     |
| Key bắt đầu `BEV`, `DCM`        | `mcp_toyota-jira_jira_get_issue`  |
| Key bắt đầu `TMCBEV`, `ICONNIC` | `mcp_lge-jira_jira_get_issue`     |
| Không rõ                        | Hỏi user hoặc mặc định `lge-jira` |

---

## Output Format

> ❌ KHÔNG thêm section nào ngoài 6 phần dưới đây.

### File header

```markdown
---
JIRA System: [LGE JIRA / Toyota JIRA]
Issue Key: [ISSUE_KEY]
Analysis Date: [YYYY-MM-DD]
Language: Vietnamese
---
```

### 📋 PHẦN 1: THÔNG TIN CƠ BẢN

| Field               | Value    |
| ------------------- | -------- |
| JIRA System         |          |
| Ticket ID           |          |
| Tiêu đề             |          |
| Trạng thái          | ✅/❌/⏳ |
| Priority / Severity |          |
| Occurrence Rate     |          |

_Toyota thêm_: Phase, Supplier, Component, Monren Rank (A/B/C)

_Nếu synced_: Linked Ticket đối diện, LGE Status, Toyota Status, Bug Owner (LGE/TMC/T-OEM)

---

### 🎯 PHẦN 2: MÔ TẢ VẤN ĐỀ & PHÂN TÍCH NGUYÊN NHÂN

- **Vấn đề chính**: mô tả bằng Output Language
- **Môi trường**: Platform, SW Version, điều kiện test
- **Các bước tái hiện**
- **Kết quả mong đợi vs thực tế**

_Toyota thêm_: Evaluation Category, Evaluation Environment (Bench/Vehicle), Function

**Root Cause** _(chỉ khi Resolved/Closed)_:

- Nguyên nhân kỹ thuật cụ thể
- Bug Owner: LGE / TMC / T-OEM
- Module liên quan
- _Toyota_: Target of Modification (Code/Spec/Config)

---

### 📈 PHẦN 3: TIMELINE CHI TIẾT

| Date | Time | Event | Action By | Owner | Status |
| ---- | ---- | ----- | --------- | ----- | ------ |

**Icons**: 🆕 Created · ➡️ Reassign · ⬆️ Priority Change · 🔍 Investigate · 🟡 Escalate · 🟢 Solution Found · 🟣 Sync to OEM · 🏆 Resolved

**Tổng kết**: Số lần assign · Tổng ngày · Tổ chức tham gia · Assignment flow

---

### 💬 PHẦN 4: DỊCH VÀ PHÂN TÍCH COMMENT

Mỗi comment:

**Comment N** — Author (Date)  
🔵/🔴/🟡/🟢/🟣/🏆 _[icon phù hợp]_

> _Bản dịch (Output Language)_

**Phân tích**: [nhận xét kỹ thuật ngắn gọn]

**Icons**: 🔴 Problem report · 🔵 Investigation/LGE · 🟡 Escalation/Finding · 🟢 Solution · 🟣 OEM/Toyota · 🏆 Resolution

---

### ✅ PHẦN 5: TÓM TẮT (2-4 câu)

Vấn đề gì → Root cause là gì → Giải pháp → Trạng thái cuối / Fix version

---

### 🔗 PHẦN 6: LINKS & REFERENCES

- Related Tickets, Cross-System Link, Confluence, Test Case ID

---

## Technical Context (Tiger Platform)

**Key Services**: PowerManagerService · ApplicationManagerService · CommunicationManagerService · TimeManagerService · DiagManagerService · RemoteService/RMTS

**Common Issue Categories**: Power Mode (Standby/Sleep) · IPC/Binder callbacks · Race conditions · Resource leaks (wakelock, fd) · CANoe/test configuration (Toyota)

**Toyota-Specific**: Monren Rank A/B/C · Phase CV/PV · Immobilizer · CANoe DoIP/CAN NM signals · ETH PHY Power Control

**Cross-system**: Luôn kiểm tra linked ticket đối diện (LGE ↔ Toyota). Phân biệt rõ bug owner: LGE / TMC / T-OEM.

---

## Language Rules

| Language   | Style                                                                          |
| ---------- | ------------------------------------------------------------------------------ |
| Vietnamese | Giữ technical terms tiếng Anh (Standby, RMTS, DCM); văn phong kỹ thuật lịch sự |
| English    | Professional engineering documentation style                                   |
| Korean     | 존댓말; giữ technical terms tiếng Anh                                          |

> Không bịa nghĩa cho từ viết tắt/thuật ngữ kỹ thuật nếu không rõ trong source.
