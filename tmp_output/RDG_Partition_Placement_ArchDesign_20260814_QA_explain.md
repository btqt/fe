Viewed SKILL.md:1-118
Viewed doc.md:1-347

Đoạn văn bản bạn đề cập nằm trong bảng định nghĩa **4 Thuộc tính chất lượng (Quality Attributes - QAs)** tại mục 2.3 của tài liệu [doc.md](file:///d:/00_wip/03_Dev/fe/FA_summary_documents/raw_topics/41%20-%20Design%20of%20secure%20remote%20diagnostic/doc.md#L57-L71). Bảng này được thiết kế để làm tiêu chí định lượng (chỉ số đo lường cụ thể) nhằm so sánh và quyết định nên chọn phương án thiết kế nào ở các phần sau.

Để giúp bạn dễ hiểu hơn, dưới đây là diễn giải chi tiết từng cột và ý nghĩa cụ thể của từng dòng QA:

### 1. Cấu trúc chung của một kịch bản chất lượng (Quality Attribute Scenario)
Thay vì mô tả chung chung như *"Hệ thống phải bảo mật"* hay *"Hệ thống phải chạy nhanh"*, tài liệu kỹ thuật này sử dụng cấu trúc chuẩn hóa để có thể đo lường và kiểm thử được:
*   **Stimulus (Tác nhân kích thích)**: Một sự cố, một hành động tấn công, hoặc một yêu cầu dịch vụ gửi tới hệ thống.
*   **Response (Phản hồi)**: Cách hệ thống phản ứng lại tác nhân đó.
*   **Response Measure (Ngưỡng đo lường)**: Các chỉ số hoặc con số cụ thể (giây, phần trăm, số lần...) để đánh giá xem hệ thống có đạt yêu cầu chất lượng hay không.

---

### 2. Diễn giải chi tiết từng thuộc tính chất lượng (QA)

#### 🛡️ **QA-SEC (Security – Resistance/Integrity): An ninh bảo mật (Khả năng chống chịu & Tính toàn vẹn)**
*   **Ý nghĩa**: Đây là tiêu chí quan trọng nhất định hình thiết kế này (**driver**). Nó kiểm tra xem hệ thống có bảo vệ được các ECU điều khiển xe khi phân vùng bên ngoài bị hack hay không.
*   **Kịch bản (Stimulus → Response)**: Khi phân vùng **EP (Entry Point)** – nơi kết nối trực tiếp với internet bên ngoài qua HTTP/MQTT – bị hacker chiếm quyền điều khiển (compromise) và cố tình giả danh là một client chẩn đoán hợp lệ để gửi các lệnh nạp lại chương trình (reprogramming) xuống mạng CAN của xe nhằm phá hoại.
*   **Chỉ số đo lường cụ thể**:
    *   `Reprogramming SID chạm CAN = 0`: Chặn đứng **100%** các mã lệnh nhạy cảm (UDS Service ID) như `{0x10(02)` (chuyển sang session lập trình), `0x11` (reset ECU), `0x28` (tắt giao tiếp truyền thông), `0x34` (yêu cầu download phần mềm), `0x85` (kiểm tra tính toàn vẹn)` không cho chạm tới đường truyền CAN của xe.
    *   `Số trust boundary độc lập attacker phải phá ≥ 2`: Để gửi được lệnh xuống CAN, kẻ tấn công không thể chỉ chiếm phân vùng EP là xong. Thiết kế bắt buộc kẻ tấn công phải vượt qua ít nhất **2 ranh giới bảo mật độc lập** (ví dụ: ranh giới giữa container EP và IP, và ranh giới kiểm soát bộ lọc bên trong IP).
    *   `Phát hiện tamper filter config = 100%`: Nếu có bất kỳ hành vi thay đổi hay can thiệp trái phép nào vào file cấu hình bộ lọc chẩn đoán (filter config), hệ thống phải phát hiện được ngay lập tức.

---

#### ⚡ **QA-PERF (Performance efficiency): Hiệu năng hệ thống**
*   **Ý nghĩa**: Đo lường tác động của việc chia tách hệ thống thành 2 phân vùng (EP và IP) xem có làm chậm quá trình chẩn đoán xe hay không.
*   **Kịch bản (Stimulus → Response)**: Thực hiện một giao dịch chẩn đoán thông thường hướng CAN (Ví dụ: Từ Center gửi lệnh xuống → qua cổng chẩn đoán AP → đến ECU dưới xe → ECU phản hồi ngược lại).
*   **Chỉ số đo lường cụ thể**:
    *   `Số lần băng EP↔IP boundary trên đường CAN-facing = 0 (ngưỡng chấp nhận ≤ 1)`: Đường truyền dữ liệu chẩn đoán chính từ ứng dụng chẩn đoán xuống CAN tốt nhất là **không đi xuyên qua ranh giới EP/IP lần nào** (tức là ứng dụng chẩn đoán và cổng CAN nằm cùng một vùng IP Container). Mỗi lần đi xuyên ranh giới (cross-boundary) sẽ tốn chi phí đồng bộ dữ liệu và chuyển ngữ cảnh làm tăng độ trễ.
    *   `Độ trễ transport nội AP thêm cho 1 UDS request nhỏ (ReadDID) p95 ≤ 5 ms`: Việc chia tách phân vùng chỉ được phép làm tăng thêm tối đa **5 miligiây** (cho 95% số trường hợp test) đối với lệnh đọc dữ liệu chẩn đoán cơ bản (ReadDID) so với kiến trúc cũ.
    *   `Throughput đủ`: Băng thông truyền dữ liệu phải đủ lớn để đáp ứng nhu cầu thu thập dữ liệu chẩn đoán định kỳ từ xe gửi về Center.

---

#### 🩺 **QA-REL (Reliability – Fault containment): Độ tin cậy (Khoanh vùng lỗi)**
*   **Ý nghĩa**: Đảm bảo lỗi xảy ra ở vùng kém tin cậy hơn (EP) không được ảnh hưởng hoặc làm sập vùng tin cậy (IP).
*   **Kịch bản (Stimulus → Response)**: Một tiến trình chạy ở phân vùng EP (nơi có bề mặt tấn công lớn, phần mềm phức tạp nên dễ bị crash) bị sập/lỗi.
*   **Chỉ số đo lường cụ thể**:
    *   `Fault propagation qua boundary = 0`: Lỗi tuyệt đối không được lan truyền từ EP sang IP. Phân vùng IP phải hoàn toàn độc lập và sống sót bình thường.
    *   `Phát hiện RDP (RemoteDiagProxy) chết (POLLHUP) ≤ 50 ms`: Phía IP phải phát hiện ra tiến trình Proxy ở EP bị crash trong vòng **50ms** nhờ cơ chế giám sát đứt kết nối socket (`POLLHUP`).
    *   `Reconnect RDP p95 ≤ 500 ms`: Khi tiến trình RDP ở EP tự phục hồi, việc kết nối lại giữa EP và IP phải hoàn tất trong vòng **500ms** (cho 95% số trường hợp).
    *   `RDG core luôn ở trạng thái xác định`: Lõi RemoteDiag bên IP không bị treo, lửng lơ hoặc không nhất quán về trạng thái dữ liệu (tránh tình trạng split-brain).

---

#### 🚗 **QA-AVAIL (Availability): Độ khả dụng của hệ thống**
*   **Ý nghĩa**: Giữ cho các dịch vụ chẩn đoán nội bộ và các tính năng điều khiển xe luôn sẵn sàng hoạt động ổn định.
*   **Kịch bản (Stimulus → Response)**: Có 2 tình huống:
    *(a)* Phân vùng EP bị quá tải hoặc bị lỗi;
    *(b)* Phân vùng EP bị chiếm quyền điều khiển và spam liên tục lệnh phá hoại xe.
*   **Chỉ số đo lường cụ thể**:
    *   Với *(a)*: `Duy trì 100% chức năng chẩn đoán nội bộ IP→CAN`: Cho dù EP bị sập hoàn toàn, các công cụ cắm trực tiếp vào xe hoặc các tác vụ chẩn đoán nội bộ khởi phát từ IP đi xuống CAN vẫn phải hoạt động bình thường, không bị phụ thuộc vào EP.
    *   Với *(b)*: `Lệnh trái phép chạm CAN = 0`: Bộ lọc chẩn đoán hoạt động tốt sẽ bảo vệ mạng CAN không bị quá tải hoặc bị reset, đảm bảo tính khả dụng của việc điều khiển xe không bị ảnh hưởng (ví dụ: xe không bị mất phanh hoặc mất lái do ECU bị reset đột ngột khi đang chạy).

---

### Tóm lại
Bảng này thiết lập các **"luật chơi" rất ngặt nghèo bằng các con số**. Ở các phần tiếp theo trong tài liệu thiết kế, các phương án kiến trúc (Alternative 1, 2, 3, 4) sẽ được đưa lên bàn cân xem phương án nào thỏa mãn các chỉ số đo lường trên tốt nhất (đặc biệt là yêu cầu **chặn 100% lệnh reprogramming SID độc hại** của QA-SEC và **0 lần băng ranh giới** của QA-PERF).