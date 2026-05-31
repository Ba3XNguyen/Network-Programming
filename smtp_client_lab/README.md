# SMTP Client Lab

## 1. Chạy Mailpit bằng Docker

```bash
docker run -d \
  --name mailpit \
  -p 1025:1025 \
  -p 8025:8025 \
  axllent/mailpit
```

Mở giao diện web:

```text
http://localhost:8025
```

## 2. Biên dịch

```bash
make
```

## 3. Chạy chương trình

```bash
./smtp_client 127.0.0.1 1025 alice@example.com bob@example.com "Normal test"
```

Nhập nội dung:

```text
This is a normal test email.
Sent from a C SMTP client.
.
```

## 4. Kiểm thử sai cổng

```bash
./smtp_client 127.0.0.1 9999 alice@example.com bob@example.com "Invalid port test"
```

Chương trình cần in lỗi `connect` và kết thúc hợp lệ.

## 5. Dọn dẹp

```bash
make clean
docker stop mailpit
docker rm mailpit
```
