# POP3 Client Lab

Folder này chứa code mẫu cho bài lab **xây dựng POP3 Client đơn giản bằng C**.

## 1. Cấu trúc thư mục

```text
pop3_lab/
├── src/
│   └── pop3_client.c      # Code POP3 client chính
├── Makefile               # Build chương trình bằng gcc
├── docker-compose.yml     # Chạy Mailpit có SMTP, Web UI, POP3
└── README.md              # Hướng dẫn chạy
```

## 2. Chạy Mailpit

Cách 1: dùng Docker Compose:

```bash
docker compose up -d
```

Cách 2: dùng docker run:

```bash
docker stop mailpit 2>/dev/null || true
docker rm mailpit 2>/dev/null || true

docker run -d \
  --name mailpit \
  -p 1025:1025 \
  -p 8025:8025 \
  -p 1110:1110 \
  -e MP_POP3_AUTH="alice:password" \
  axllent/mailpit
```

Kiểm tra:

```bash
docker ps
```

Mở Web UI:

```text
http://localhost:8025
```

## 3. Build POP3 client

```bash
make
```

Sau khi build xong sẽ có file:

```text
./pop3_client
```

## 4. Tạo email test

Dùng SMTP client của bài trước để gửi email vào Mailpit, ví dụ:

```bash
./smtp_client 127.0.0.1 1025 alice@example.com bob@example.com "First test email"
```

Nhập nội dung, kết thúc bằng dòng chỉ có dấu chấm:

```text
Hello Bob,

This is the first test email.
.
```

Sau đó mở Web UI `http://localhost:8025` để kiểm tra email đã vào Mailpit.

## 5. Chạy POP3 client

Cú pháp:

```bash
./pop3_client <server_ip> <server_port> <username> <password>
```

Ví dụ đúng với lab:

```bash
./pop3_client 127.0.0.1 1110 alice password
```

Menu chương trình:

```text
===== POP3 Client Menu =====
1. Show mailbox status
2. List messages
3. Retrieve a message
4. Quit
Choose an option:
```

## 6. Ý nghĩa các chức năng

- `Show mailbox status`: gửi lệnh `STAT`, server trả về số lượng email và tổng kích thước mailbox.
- `List messages`: gửi lệnh `LIST`, server trả về danh sách email, mỗi email gồm số thứ tự và kích thước.
- `Retrieve a message`: gửi lệnh `RETR n`, server trả về nội dung email thứ `n`.
- `Quit`: gửi lệnh `QUIT`, kết thúc phiên POP3.

## 7. Test nhanh POP3 bằng netcat

```bash
nc localhost 1110
```

Nhập lần lượt:

```text
USER alice
PASS password
STAT
LIST
RETR 1
QUIT
```

## 8. Dọn file build

```bash
make clean
```
