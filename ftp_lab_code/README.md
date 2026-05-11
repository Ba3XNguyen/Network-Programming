# FTP Lab - Simple FTP Client in C

## 1. Folder structure

Place your existing `docker-compose.yml` in this folder:

```text
ftp_lab/
├── docker-compose.yml
├── ftpdata/
│   └── user/
│       ├── hello.txt
│       └── test.txt
├── src/
│   └── ftp_client.c
├── Makefile
└── README.md
```

## 2. Prepare FTP data

```bash
mkdir -p ftpdata/user
echo "Hello FTP" > ftpdata/user/hello.txt
echo "This is a test file" > ftpdata/user/test.txt
```

## 3. Start FTP server

```bash
docker compose up -d
```

If your Docker version is older:

```bash
docker-compose up -d
```

Check:

```bash
docker ps
docker port vsftpd
```

## 4. Test FTP server manually

```bash
telnet 127.0.0.1 21
```

Then type:

```text
USER user
PASS pass
QUIT
```

Expected:
- `220`: FTP server ready
- `331`: password required
- `230`: login successful

## 5. Compile

```bash
make
```

Or:

```bash
gcc -Wall -Wextra -O2 src/ftp_client.c -o ftp_client
```

## 6. Run LIST

```bash
./ftp_client
```

Or:

```bash
./ftp_client 127.0.0.1 21 user pass list
```

Expected flow:

```text
220 greeting
USER user
331 password required
PASS pass
230 login successful
PASV
227 passive mode
LIST
directory listing from data connection
226 transfer complete
```

## 7. Optional: download a file with RETR

```bash
./ftp_client 127.0.0.1 21 user pass get hello.txt
```

The downloaded file will be saved as:

```text
downloaded_hello.txt
```

## 8. Clean

```bash
make clean
```

## 9. Stop FTP server

```bash
docker compose down
```
