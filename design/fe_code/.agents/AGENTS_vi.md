# Superpowers cho Antigravity

Bạn có siêu năng lực.

Profile này chuyển đổi các quy trình làm việc (workflows) của Superpowers cho Antigravity với cơ chế thực thi **strict single-flow**.

## Quy tắc cốt lõi (Core Rules)

1. Ưu tiên các skills cục bộ tại `.agent/skills/<skill-name>/SKILL.md`.
2. Thực hiện từng tác vụ cốt lõi một tại một thời điểm với `task_boundary`.
3. Chỉ sử dụng `browser_subagent` cho các tác vụ tự động hóa trình duyệt (browser automation).
4. Theo dõi tiến độ checklist trong `<project-root>/docs/plans/task.md` (live tracker dạng bảng).
5. Giữ các thay đổi nằm trong phạm vi tác vụ được yêu cầu và xác thực (verify) trước khi xác nhận hoàn thành (completion claims).

## Hợp đồng chuyển đổi công cụ (Tool Translation Contract)

Khi các skills nguồn tham chiếu đến tên công cụ cũ (legacy tool names), hãy sử dụng các công cụ tương đương trong Antigravity:

- Tên assistant/platform cũ -> `Antigravity`
- Công cụ `Task` -> `browser_subagent` cho các tác vụ trình duyệt, nếu không thì sử dụng `task_boundary` tuần tự
- Công cụ `Skill` -> `view_file ~/.gemini/skills/<skill-name>/SKILL.md` (hoặc project-local `.agent/skills/<skill-name>/SKILL.md`)
- `TodoWrite` -> cập nhật danh sách tác vụ trong `<project-root>/docs/plans/task.md`
- Các thao tác file -> `view_file`, `write_to_file`, `replace_file_content`, `multi_replace_file_content`
- Liệt kê thư mục -> `list_dir`
- Cấu trúc mã nguồn -> `view_file_outline`, `view_code_item`
- Tìm kiếm -> `grep_search`, `find_by_name`
- Shell -> `run_command`
- Web fetch -> `read_url_content`
- Web search -> `search_web`
- Tạo ảnh -> `generate_image`
- Giao tiếp với người dùng trong quá trình thực hiện -> `notify_user`
- Các công cụ MCP -> họ công cụ `mcp_*`

## Nạp Skill (Skill Loading)

- Ưu tiên 1: các project skills tại `.agents/skills`.
- Ưu tiên 2: các user skills tại `~/.gemini/skills`.
- Nếu cả hai đều tồn tại, project-local skills sẽ được ưu tiên cho profile này.
- Các tài nguyên tùy chọn (optional parity assets) có thể tồn tại tại `.agents/workflows/*` và `.agents/agents/*` dưới dạng các entrypoint shims hoặc reference profiles.
- Các tài nguyên này không thay đổi yêu cầu thực thi **strict single-flow** quy định trong file này.

## Mô hình thực thi Single-Flow (Single-Flow Execution Model)

- Không điều phối (dispatch) nhiều coding agents song song.
- Chia nhỏ các công việc lớn thành các bước rõ ràng, có thứ tự.
- Luôn chỉ có duy nhất một tác vụ ở trạng thái active tại một thời điểm trong `<project-root>/docs/plans/task.md`.
- Nếu yêu cầu làm việc với trình duyệt, hãy tách biệt nó trong một bước browser chuyên biệt.

## Kỷ luật xác thực (Verification Discipline)

Trước khi xác nhận một tác vụ đã xong:

1. Chạy các lệnh xác thực (verification commands) liên quan.
2. Xác nhận mã thoát (exit status) và kết quả đầu ra chính (key output).
3. Cập nhật `<project-root>/docs/plans/task.md`.
4. Báo cáo bằng chứng (evidence), sau đó mới xác nhận hoàn thành.
