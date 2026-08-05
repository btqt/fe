---
description: "Sử dụng khi tạo hoặc chỉnh sửa tệp `config.yaml` hoặc các tệp trong thư mục `config/*.yaml`. Quy định về cấu trúc, bảo mật thông tin nhạy cảm và quản lý biến môi trường."
applyTo: "**/config.yaml, config/**/*.yaml"
---

# Quy tắc cấu hình cho `config.yaml`

Tệp hướng dẫn này chỉ áp dụng cho việc tạo và chỉnh sửa `config.yaml` và các tệp cấu hình YAML liên quan.

## 1. Mục đích

`config.yaml` phải chứa:

- Các tham số phụ thuộc vào môi trường (environment-dependent parameters).
- Các cài đặt có thể điều chỉnh khi chạy (runtime-adjustable settings).

**KHÔNG ĐƯỢC** chứa:

- Logic nghiệp vụ (business logic).
- Mã thực thi (executable code).
- Thông tin nhạy cảm (secrets) được ghi cứng.

## 2. Vị trí tệp

- Dự án nhỏ: đặt `config.yaml` tại thư mục gốc (root).
- Dự án trung bình hoặc có cấu trúc: sử dụng:
  ```
  config/
    base.yaml
    dev.yaml
    prod.yaml
  ```
- Tùy chọn: `config/local.yaml` (phải được đưa vào `.gitignore`).

## 3. Quy tắc cấu trúc

- Luôn nhóm cấu hình theo lĩnh vực (domain):
  - `app`, `server`, `database`, `logging`, `features`, `ai`.
- Tránh cấu trúc khóa phẳng (flat key structures).
- Sử dụng quy ước đặt tên nhất quán (ưu tiên `snake_case`).
- Duy trì cấu trúc YAML phân cấp sạch sẽ.

✅ **Tốt:**

```yaml
server:
  host: 0.0.0.0
  port: 8000

database:
  host: ${DB_HOST}
  port: 5432
```

❌ **Tránh:**

```yaml
server_port: 8000
db_host: localhost
```

## 4. Quản lý bí mật (Secrets Management)

- **TUYỆT ĐỐI KHÔNG** ghi cứng (hardcode) các giá trị nhạy cảm (mật khẩu, API keys, tokens).
- Luôn sử dụng tham chiếu biến môi trường:

```yaml
database:
  password: ${DB_PASSWORD}
```

## 5. Giá trị mặc định & Xác thực

- Cung cấp các giá trị mặc định an toàn và hợp lý nếu có thể.
- Phân biệt rõ ràng các trường bắt buộc (required) và tùy chọn (optional).
- Ứng dụng phải dừng ngay lập tức (fail fast) nếu thiếu cấu hình bắt buộc.

## 6. Chiến lược ghi đè môi trường

- Hỗ trợ ghi đè theo từng môi trường (`dev`, `staging`, `prod`).
- Cho phép các biến môi trường ghi đè giá trị trong YAML.
- Giữ `local.yaml` nằm ngoài hệ thống quản lý phiên bản (version control).
