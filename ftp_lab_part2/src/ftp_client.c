
/*
 * FTP Client - Lập trình mạng - Phần 1 + Phần 2
 *
 * Chức năng:
 *  - Kết nối FTP server qua control connection TCP port 21
 *  - Đăng nhập USER/PASS
 *  - PASV + LIST
 *  - PASV + RETR <filename>
 *  - PASV + STOR <filename>
 *  - DELE <filename>
 *  - CWD <dirname>
 *  - Menu dòng lệnh tương tác
 *
 * Compile:
 *  gcc -Wall -Wextra -O2 src/ftp_client.c -o ftp_client
 *
 * Run:
 *  ./ftp_client
 *  ./ftp_client 127.0.0.1 21 user pass
 */

#include <arpa/inet.h>
#include <errno.h>
#include <netdb.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>

#define BUFFER_SIZE 4096
#define LINE_SIZE 1024

static void die(const char *msg) {
    perror(msg);
    exit(EXIT_FAILURE);
}

static void trim_newline(char *s) {
    size_t len = strlen(s);
    while (len > 0 && (s[len - 1] == '\n' || s[len - 1] == '\r')) {
        s[len - 1] = '\0';
        len--;
    }
}

static const char *basename_of_path(const char *path) {
    const char *slash = strrchr(path, '/');
    return slash ? slash + 1 : path;
}

static int connect_to_host(const char *host, int port) {
    int sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0) die("socket");

    struct sockaddr_in addr;
    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_port = htons((uint16_t)port);

    if (inet_pton(AF_INET, host, &addr.sin_addr) != 1) {
        struct hostent *he = gethostbyname(host);
        if (!he || !he->h_addr_list[0]) {
            fprintf(stderr, "Cannot resolve host: %s\n", host);
            close(sockfd);
            exit(EXIT_FAILURE);
        }
        memcpy(&addr.sin_addr, he->h_addr_list[0], (size_t)he->h_length);
    }

    if (connect(sockfd, (struct sockaddr *)&addr, sizeof(addr)) < 0) {
        close(sockfd);
        die("connect");
    }

    return sockfd;
}

static void send_all(int sockfd, const char *data) {
    size_t total = 0;
    size_t len = strlen(data);

    printf(">>> %s", data);

    while (total < len) {
        ssize_t n = send(sockfd, data + total, len - total, 0);
        if (n < 0) die("send");
        if (n == 0) {
            fprintf(stderr, "send returned 0\n");
            exit(EXIT_FAILURE);
        }
        total += (size_t)n;
    }
}

static int recv_response(int sockfd, char *buf, size_t size) {
    memset(buf, 0, size);

    ssize_t n = recv(sockfd, buf, size - 1, 0);
    if (n < 0) die("recv");
    if (n == 0) {
        fprintf(stderr, "Server closed control connection\n");
        exit(EXIT_FAILURE);
    }

    buf[n] = '\0';
    printf("<<< %s", buf);

    if (n >= 3 && buf[0] >= '0' && buf[0] <= '9') {
        char code_str[4];
        memcpy(code_str, buf, 3);
        code_str[3] = '\0';
        return atoi(code_str);
    }

    return 0;
}

static void send_cmd(int sockfd, const char *cmd) {
    send_all(sockfd, cmd);
}

static void send_cmd_arg(int sockfd, const char *fmt, const char *arg) {
    char cmd[LINE_SIZE];
    int n = snprintf(cmd, sizeof(cmd), fmt, arg);

    if (n < 0 || (size_t)n >= sizeof(cmd)) {
        fprintf(stderr, "Command too long\n");
        return;
    }

    send_cmd(sockfd, cmd);
}

static int parse_pasv(const char *response, char *ip, size_t ip_size, int *port) {
    int h1, h2, h3, h4, p1, p2;
    const char *start = strchr(response, '(');

    if (!start) {
        fprintf(stderr, "PASV response parse error: missing '('\n");
        return -1;
    }

    if (sscanf(start, "(%d,%d,%d,%d,%d,%d)", &h1, &h2, &h3, &h4, &p1, &p2) != 6) {
        fprintf(stderr, "PASV response parse error: %s\n", response);
        return -1;
    }

    snprintf(ip, ip_size, "%d.%d.%d.%d", h1, h2, h3, h4);
    *port = p1 * 256 + p2;
    return 0;
}

static int enter_pasv(int control_sock, char *data_ip, size_t ip_size, int *data_port) {
    char resp[BUFFER_SIZE];

    send_cmd(control_sock, "PASV\r\n");
    int code = recv_response(control_sock, resp, sizeof(resp));

    if (code != 227) {
        fprintf(stderr, "Expected 227 for PASV, got %d\n", code);
        return -1;
    }

    if (parse_pasv(resp, data_ip, ip_size, data_port) != 0) {
        return -1;
    }

    printf("[+] Data connection endpoint: %s:%d\n", data_ip, *data_port);
    return 0;
}

static void ftp_login(int control_sock, const char *user, const char *pass) {
    char resp[BUFFER_SIZE];

    int code = recv_response(control_sock, resp, sizeof(resp));
    if (code != 220) {
        fprintf(stderr, "Warning: expected 220 greeting, got %d\n", code);
    }

    send_cmd_arg(control_sock, "USER %s\r\n", user);
    code = recv_response(control_sock, resp, sizeof(resp));

    if (code == 331) {
        send_cmd_arg(control_sock, "PASS %s\r\n", pass);
        code = recv_response(control_sock, resp, sizeof(resp));
    }

    if (code != 230) {
        fprintf(stderr, "Login failed, code=%d\n", code);
        exit(EXIT_FAILURE);
    }

    printf("[+] Login successful\n");
}

static void ftp_list(int control_sock) {
    char resp[BUFFER_SIZE], ip[64];
    int port;

    if (enter_pasv(control_sock, ip, sizeof(ip), &port) != 0) return;

    int data_sock = connect_to_host(ip, port);

    send_cmd(control_sock, "LIST\r\n");
    recv_response(control_sock, resp, sizeof(resp));

    printf("\n===== LIST DATA =====\n");
    while (1) {
        char buf[BUFFER_SIZE];
        ssize_t n = recv(data_sock, buf, sizeof(buf) - 1, 0);
        if (n < 0) die("recv LIST data");
        if (n == 0) break;
        buf[n] = '\0';
        printf("%s", buf);
    }
    printf("===== END LIST =====\n\n");

    close(data_sock);
    recv_response(control_sock, resp, sizeof(resp));
}

static void ftp_retr(int control_sock, const char *filename) {
    char resp[BUFFER_SIZE], ip[64], outname[LINE_SIZE];
    int port;

    if (!filename || strlen(filename) == 0) {
        fprintf(stderr, "Usage: get <remote_file>\n");
        return;
    }

    if (enter_pasv(control_sock, ip, sizeof(ip), &port) != 0) return;

    int data_sock = connect_to_host(ip, port);

    send_cmd_arg(control_sock, "RETR %s\r\n", filename);
    int code = recv_response(control_sock, resp, sizeof(resp));

    if (code >= 400) {
        fprintf(stderr, "RETR failed, code=%d\n", code);
        close(data_sock);
        return;
    }

    snprintf(outname, sizeof(outname), "downloaded_%s", basename_of_path(filename));
    FILE *fp = fopen(outname, "wb");
    if (!fp) {
        perror("fopen output");
        close(data_sock);
        return;
    }

    while (1) {
        char buf[BUFFER_SIZE];
        ssize_t n = recv(data_sock, buf, sizeof(buf), 0);
        if (n < 0) {
            perror("recv RETR data");
            break;
        }
        if (n == 0) break;
        fwrite(buf, 1, (size_t)n, fp);
    }

    fclose(fp);
    close(data_sock);

    printf("[+] Downloaded to local file: %s\n", outname);
    recv_response(control_sock, resp, sizeof(resp));
}

static void ftp_stor(int control_sock, const char *filename) {
    char resp[BUFFER_SIZE], ip[64];
    int port;

    if (!filename || strlen(filename) == 0) {
        fprintf(stderr, "Usage: put <local_file>\n");
        return;
    }

    FILE *fp = fopen(filename, "rb");
    if (!fp) {
        perror("Cannot open local file");
        return;
    }

    if (enter_pasv(control_sock, ip, sizeof(ip), &port) != 0) {
        fclose(fp);
        return;
    }

    int data_sock = connect_to_host(ip, port);

    send_cmd_arg(control_sock, "STOR %s\r\n", basename_of_path(filename));
    int code = recv_response(control_sock, resp, sizeof(resp));

    if (code >= 400) {
        fprintf(stderr, "STOR failed, code=%d\n", code);
        fclose(fp);
        close(data_sock);
        return;
    }

    while (1) {
        char buf[BUFFER_SIZE];
        size_t n = fread(buf, 1, sizeof(buf), fp);
        if (n > 0) {
            size_t sent_total = 0;
            while (sent_total < n) {
                ssize_t sent = send(data_sock, buf + sent_total, n - sent_total, 0);
                if (sent < 0) {
                    perror("send STOR data");
                    fclose(fp);
                    close(data_sock);
                    return;
                }
                sent_total += (size_t)sent;
            }
        }

        if (n < sizeof(buf)) {
            if (ferror(fp)) perror("fread");
            break;
        }
    }

    fclose(fp);
    close(data_sock);

    printf("[+] Uploaded local file: %s\n", filename);
    recv_response(control_sock, resp, sizeof(resp));
}

static void ftp_delete(int control_sock, const char *filename) {
    char resp[BUFFER_SIZE];

    if (!filename || strlen(filename) == 0) {
        fprintf(stderr, "Usage: delete <remote_file>\n");
        return;
    }

    send_cmd_arg(control_sock, "DELE %s\r\n", filename);
    recv_response(control_sock, resp, sizeof(resp));
}

static void ftp_cwd(int control_sock, const char *dirname) {
    char resp[BUFFER_SIZE];

    if (!dirname || strlen(dirname) == 0) {
        fprintf(stderr, "Usage: cwd <remote_directory>\n");
        return;
    }

    send_cmd_arg(control_sock, "CWD %s\r\n", dirname);
    recv_response(control_sock, resp, sizeof(resp));
}

static void ftp_pwd(int control_sock) {
    char resp[BUFFER_SIZE];
    send_cmd(control_sock, "PWD\r\n");
    recv_response(control_sock, resp, sizeof(resp));
}

static void print_help(void) {
    printf("\nCommands:\n");
    printf("  list / ls                 List files on server\n");
    printf("  get <remote_file>         Download file from server\n");
    printf("  put <local_file>          Upload file to server\n");
    printf("  delete <remote_file>      Delete file on server\n");
    printf("  cwd <remote_dir>          Change remote working directory\n");
    printf("  pwd                       Show remote working directory\n");
    printf("  help                      Show this help\n");
    printf("  quit                      Exit\n\n");
}

static void interactive_loop(int control_sock) {
    char line[LINE_SIZE];

    print_help();

    while (1) {
        printf("ftp-client> ");
        fflush(stdout);

        if (!fgets(line, sizeof(line), stdin)) {
            printf("\n");
            break;
        }

        trim_newline(line);
        if (strlen(line) == 0) continue;

        char *cmd = strtok(line, " ");
        char *arg = strtok(NULL, "");

        if (!cmd) continue;

        if (strcmp(cmd, "list") == 0 || strcmp(cmd, "ls") == 0) {
            ftp_list(control_sock);
        } else if (strcmp(cmd, "get") == 0 || strcmp(cmd, "retr") == 0) {
            ftp_retr(control_sock, arg);
        } else if (strcmp(cmd, "put") == 0 || strcmp(cmd, "stor") == 0) {
            ftp_stor(control_sock, arg);
        } else if (strcmp(cmd, "delete") == 0 || strcmp(cmd, "dele") == 0 || strcmp(cmd, "rm") == 0) {
            ftp_delete(control_sock, arg);
        } else if (strcmp(cmd, "cwd") == 0 || strcmp(cmd, "cd") == 0) {
            ftp_cwd(control_sock, arg);
        } else if (strcmp(cmd, "pwd") == 0) {
            ftp_pwd(control_sock);
        } else if (strcmp(cmd, "help") == 0) {
            print_help();
        } else if (strcmp(cmd, "quit") == 0 || strcmp(cmd, "exit") == 0) {
            break;
        } else {
            fprintf(stderr, "Unknown command: %s\n", cmd);
            print_help();
        }
    }
}

int main(int argc, char *argv[]) {
    const char *host = "127.0.0.1";
    int port = 21;
    const char *user = "user";
    const char *pass = "pass";
    char resp[BUFFER_SIZE];

    if (argc == 5) {
        host = argv[1];
        port = atoi(argv[2]);
        user = argv[3];
        pass = argv[4];
    } else if (argc != 1) {
        fprintf(stderr, "Usage: %s [host port username password]\n", argv[0]);
        return EXIT_FAILURE;
    }

    printf("[+] Connecting to FTP server %s:%d\n", host, port);
    int control_sock = connect_to_host(host, port);

    ftp_login(control_sock, user, pass);
    interactive_loop(control_sock);

    send_cmd(control_sock, "QUIT\r\n");
    recv_response(control_sock, resp, sizeof(resp));

    close(control_sock);
    printf("[+] Bye\n");
    return 0;
}
