# FTP Client Lab - Part 1 + Part 2

## Cấu trúc thư mục

Đặt file `docker-compose.yml` bạn đã có vào thư mục `ftp_lab`.

```text
ftp_lab/
├── docker-compose.yml
├── ftpdata/
│   └── user/
│       ├── readme.txt
│       ├── hello.txt
│       ├── test.txt
│       └── subdir/
│           └── inside.txt
├── src/
│   └── ftp_client.c
├── Makefile
├── README.md
└── upload.txt
```

## Tạo dữ liệu test

```bash
mkdir -p ftpdata/user/subdir

echo "This is readme from FTP server" > ftpdata/user/readme.txt
echo "Hello FTP" > ftpdata/user/hello.txt
echo "This is a test file" > ftpdata/user/test.txt
echo "This file is inside subdir" > ftpdata/user/subdir/inside.txt

echo "This file will be uploaded by STOR" > upload.txt
```

## Chạy FTP server

```bash
docker compose up -d
```

Nếu Docker Compose bản cũ:

```bash
docker-compose up -d
```

Kiểm tra:

```bash
docker ps
docker port vsftpd
```

## Compile

```bash
make
```

Hoặc:

```bash
gcc -Wall -Wextra -O2 src/ftp_client.c -o ftp_client
```

## Chạy client

```bash
./ftp_client
```

Hoặc chỉ rõ tham số:

```bash
./ftp_client 127.0.0.1 21 user pass
```

## Menu lệnh trong chương trình

```text
list
get readme.txt
put upload.txt
delete upload.txt
cwd subdir
list
pwd
help
quit
```

## Demo đề xuất

```text
list
get readme.txt
put upload.txt
list
delete upload.txt
list
cwd subdir
list
quit
```

Sau lệnh `get readme.txt`, kiểm tra file tải về:

```bash
cat downloaded_readme.txt
```

## Dừng server

```bash
docker compose down
```

## Dọn file build

```bash
make clean
```
