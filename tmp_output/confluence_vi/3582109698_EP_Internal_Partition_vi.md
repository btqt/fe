# 02. EP / Internal Partition (Container)

> Nguồn: Confluence DCMNTCY — trang 3582109698. Bản dịch tiếng Việt; giữ nguyên bảng và thuật ngữ kỹ thuật tiếng Anh.
>
> Link gốc: http://collab.lge.com/main/spaces/DCMNTCY/pages/3582109698/02.+EP+Internal+Partition+Container

## Requirement (Yêu cầu)

| Req#1                                                                                                                                                | Req#2                                                                                                                               |
| ---------------------------------------------------------------------------------------------------------------------------------------------------- | ----------------------------------------------------------------------------------------------------------------------------------- |
| 1. Phạm vi bị compromise phải được giới hạn trong EP Partition và không được lan sang bên trong (tới các internal partition của in-vehicle CAN bus). | 2. Ngay cả khi một SoC/VM bị compromise ở phía EP cố gắng mạo danh một DIAG client và gửi SID vào internal CAN, nó phải bị discard. |

## SW Block Diagram — Next DCM (đang cập nhật)

### Latest Design (Thiết kế mới nhất)

- (5/19) Phản ánh yêu cầu của khách hàng tại cuộc họp Nagoya: tách app RemoteDiag theo Server/MM SID path.
- (5/19) Để xử lý riêng biệt các request của những app RemoteDiag đã tách, OnBoardClient **không tách** mà chỉ đặt bên trong Internal Container.
- (6/25) Đưa **toàn bộ RemoteDiag** vào Internal Container; tuy nhiên, để giao tiếp với các module nằm ở EP, bổ sung module **RemoteDiagProxy**.

### Alternative Design #5

### Alternative Design #4

### Alternative Design #1

### Alternative Design #2

### Alternative Design #3

_(Các phương án thiết kế thay thế — sơ đồ đính kèm trong trang gốc.)_

## High-Level Design

### Danh sách các hạng mục thiết kế cần cân nhắc

| #   | Design Topic (Chủ đề thiết kế)                        | Ghi chú                                                               |
| --- | ----------------------------------------------------- | --------------------------------------------------------------------- |
| 1   | Booting (boot sequence, container start bằng systemd) |                                                                       |
| 2   | Isolation giữa các container                          |                                                                       |
| 3   | Kernel H/W node (time/rtc/network/power)              |                                                                       |
| 4   | Truyền message và data giữa các module RemoteDiag     |                                                                       |
| 5   | Mã hóa/giải mã message truyền                         |                                                                       |
| 6   | ECU Authentication (CHAP Authentication)              |                                                                       |
| 7   | Key Update                                            |                                                                       |
| 8   | Truyền message và data giữa CommMgr và CommMgrProxy   |                                                                       |
| 9   | SW Update                                             |                                                                       |
| 10  | Power State Transition (Normal, STANDBY, STOP)        |                                                                       |
| 11  | Fail-Safe ở góc độ container                          |                                                                       |
| 12  | Resource Quotas                                       | Rủi ro: EP Container chiếm 100% CPU hoặc RAM, làm đói Diag Container. |
| 13  | Logging & Audit                                       |                                                                       |

## Khác (기타)

### Filtering

| SID  | Mô tả                                                 | OBC | DiagMgr |
| ---- | ----------------------------------------------------- | --- | ------- |
| 0x10 | DiagnosticSessionControl — programming session (0x02) | O   | O       |
| 0x11 | ECUReset                                              | X   | O       |
| 0x28 | Communication Control                                 | X   | O       |
| 0x34 | Request Download                                      | O   | O       |
| 0x85 | ControlDTCSetting                                     | X   | O       |
