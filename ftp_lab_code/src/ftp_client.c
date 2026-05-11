/*
 * Simple FTP Client - Network Programming Lab
 *
 * Features:
 *  - Connect to FTP server via control connection (TCP port 21)
 *  - Login using USER/PASS
 *  - Enter Passive Mode using PASV
 *  - Open data connection
 *  - Send LIST command and print directory listing
 *  - Optional: download a file using RETR <filename>
 *
 * Default server:
 *  - Host: 127.0.0.1
 *  - Port: 21
 *  - Username: user
 *  - Password: pass
 *
 * Compile:
 *  gcc -Wall -Wextra -O2 src/ftp_client.c -o ftp_client
 *
 * Run:
 *  ./ftp_client
 *  ./ftp_client 127.0.0.1 21 user pass
 *  ./ftp_client 127.0.0.1 21 user pass list
 *  ./ftp_client 127.0.0.1 21 user pass get hello.txt
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

static void die(const char *msg) {
    perror(msg);
    exit(EXIT_FAILURE);
}

static int connect_to_host(const char *host, int port) {
    int sockfd;
    struct sockaddr_in server_addr;
    struct hostent *he;

    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0) {
        die("socket");
    }

    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons((uint16_t)port);

    if (inet_pton(AF_INET, host, &server_addr.sin_addr) != 1) {
        he = gethostbyname(host);
        if (he == NULL || he->h_addr_list[0] == NULL) {
            fprintf(stderr, "Cannot resolve host: %s\n", host);
            close(sockfd);
            exit(EXIT_FAILURE);
        }
        memcpy(&server_addr.sin_addr, he->h_addr_list[0], (size_t)he->h_length);
    }

    if (connect(sockfd, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0) {
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
        ssize_t sent = send(sockfd, data + total, len - total, 0);
        if (sent < 0) {
            die("send");
        }
        if (sent == 0) {
            fprintf(stderr, "send returned 0\n");
            exit(EXIT_FAILURE);
        }
        total += (size_t)sent;
    }
}

/*
 * Read FTP response from control connection.
 * This simple version reads once, which is enough for this lab's vsftpd responses.
 */
static int receive_response(int sockfd, char *buffer, size_t size) {
    ssize_t n;

    memset(buffer, 0, size);
    n = recv(sockfd, buffer, size - 1, 0);

    if (n < 0) {
        die("recv");
    }
    if (n == 0) {
        fprintf(stderr, "Server closed control connection\n");
        exit(EXIT_FAILURE);
    }

    buffer[n] = '\0';
    printf("<<< %s", buffer);

    if (n >= 3 && buffer[0] >= '0' && buffer[0] <= '9') {
        char code_str[4];
        memcpy(code_str, buffer, 3);
        code_str[3] = '\0';
        return atoi(code_str);
    }

    return 0;
}

static void send_command(int sockfd, const char *cmd) {
    send_all(sockfd, cmd);
}

static void send_command_fmt(int sockfd, const char *fmt, const char *arg) {
    char command[1024];

    int n = snprintf(command, sizeof(command), fmt, arg);
    if (n < 0 || (size_t)n >= sizeof(command)) {
        fprintf(stderr, "Command too long\n");
        exit(EXIT_FAILURE);
    }

    send_command(sockfd, command);
}

static int parse_pasv_response(const char *response, char *ip, size_t ip_size, int *port) {
    int h1, h2, h3, h4, p1, p2;
    const char *start = strchr(response, '(');

    if (start == NULL) {
        fprintf(stderr, "Cannot parse PASV response: missing '('\n");
        return -1;
    }

    if (sscanf(start, "(%d,%d,%d,%d,%d,%d)", &h1, &h2, &h3, &h4, &p1, &p2) != 6) {
        fprintf(stderr, "Cannot parse PASV response: %s\n", response);
        return -1;
    }

    if (h1 < 0 || h1 > 255 || h2 < 0 || h2 > 255 ||
        h3 < 0 || h3 > 255 || h4 < 0 || h4 > 255 ||
        p1 < 0 || p1 > 255 || p2 < 0 || p2 > 255) {
        fprintf(stderr, "Invalid PASV response numbers\n");
        return -1;
    }

    snprintf(ip, ip_size, "%d.%d.%d.%d", h1, h2, h3, h4);
    *port = p1 * 256 + p2;

    return 0;
}

static int enter_passive_mode(int control_sockfd, char *data_ip, size_t ip_size, int *data_port) {
    char response[BUFFER_SIZE];

    send_command(control_sockfd, "PASV\r\n");
    int code = receive_response(control_sockfd, response, sizeof(response));

    if (code != 227) {
        fprintf(stderr, "Expected 227 after PASV, got %d\n", code);
        return -1;
    }

    if (parse_pasv_response(response, data_ip, ip_size, data_port) != 0) {
        return -1;
    }

    printf("[+] Passive data endpoint: %s:%d\n", data_ip, *data_port);
    return 0;
}

static void receive_data_to_stdout(int data_sockfd) {
    char buffer[BUFFER_SIZE];
    ssize_t n;

    printf("\n===== DATA CONNECTION OUTPUT =====\n");

    while ((n = recv(data_sockfd, buffer, sizeof(buffer) - 1, 0)) > 0) {
        buffer[n] = '\0';
        printf("%s", buffer);
    }

    if (n < 0) {
        die("recv data");
    }

    printf("===== END DATA =====\n\n");
}

static void receive_data_to_file(int data_sockfd, const char *filename) {
    char buffer[BUFFER_SIZE];
    ssize_t n;
    FILE *fp = fopen(filename, "wb");

    if (fp == NULL) {
        die("fopen output file");
    }

    while ((n = recv(data_sockfd, buffer, sizeof(buffer), 0)) > 0) {
        if (fwrite(buffer, 1, (size_t)n, fp) != (size_t)n) {
            fclose(fp);
            fprintf(stderr, "Failed to write output file\n");
            exit(EXIT_FAILURE);
        }
    }

    if (n < 0) {
        fclose(fp);
        die("recv file data");
    }

    fclose(fp);
    printf("[+] Saved file to: %s\n", filename);
}

static void ftp_login(int control_sockfd, const char *username, const char *password) {
    char response[BUFFER_SIZE];
    int code;

    code = receive_response(control_sockfd, response, sizeof(response));
    if (code != 220) {
        fprintf(stderr, "Warning: expected 220 greeting, got %d\n", code);
    }

    send_command_fmt(control_sockfd, "USER %s\r\n", username);
    code = receive_response(control_sockfd, response, sizeof(response));

    if (code == 331) {
        send_command_fmt(control_sockfd, "PASS %s\r\n", password);
        code = receive_response(control_sockfd, response, sizeof(response));
    }

    if (code != 230) {
        fprintf(stderr, "Login failed or unexpected reply code: %d\n", code);
        exit(EXIT_FAILURE);
    }

    printf("[+] Login successful\n");
}

static void ftp_list(int control_sockfd) {
    char response[BUFFER_SIZE];
    char data_ip[64];
    int data_port;

    if (enter_passive_mode(control_sockfd, data_ip, sizeof(data_ip), &data_port) != 0) {
        exit(EXIT_FAILURE);
    }

    int data_sockfd = connect_to_host(data_ip, data_port);

    send_command(control_sockfd, "LIST\r\n");
    receive_response(control_sockfd, response, sizeof(response));

    receive_data_to_stdout(data_sockfd);
    close(data_sockfd);

    receive_response(control_sockfd, response, sizeof(response));
}

static void ftp_retr(int control_sockfd, const char *remote_filename) {
    char response[BUFFER_SIZE];
    char data_ip[64];
    int data_port;
    char local_filename[512];

    if (enter_passive_mode(control_sockfd, data_ip, sizeof(data_ip), &data_port) != 0) {
        exit(EXIT_FAILURE);
    }

    int data_sockfd = connect_to_host(data_ip, data_port);

    send_command_fmt(control_sockfd, "RETR %s\r\n", remote_filename);
    int code = receive_response(control_sockfd, response, sizeof(response));

    if (code >= 400) {
        close(data_sockfd);
        fprintf(stderr, "RETR failed with code %d\n", code);
        return;
    }

    snprintf(local_filename, sizeof(local_filename), "downloaded_%s", remote_filename);
    receive_data_to_file(data_sockfd, local_filename);
    close(data_sockfd);

    receive_response(control_sockfd, response, sizeof(response));
}

static void print_usage(const char *program) {
    printf("Usage:\n");
    printf("  %s\n", program);
    printf("  %s <host> <port> <username> <password>\n", program);
    printf("  %s <host> <port> <username> <password> list\n", program);
    printf("  %s <host> <port> <username> <password> get <filename>\n", program);
    printf("\nDefault:\n");
    printf("  host=127.0.0.1 port=21 username=user password=pass action=list\n");
}

int main(int argc, char *argv[]) {
    const char *host = "127.0.0.1";
    int port = 21;
    const char *username = "user";
    const char *password = "pass";
    const char *action = "list";
    const char *filename = NULL;

    char response[BUFFER_SIZE];

    if (argc == 2 && strcmp(argv[1], "-h") == 0) {
        print_usage(argv[0]);
        return 0;
    }

    if (argc >= 5) {
        host = argv[1];
        port = atoi(argv[2]);
        username = argv[3];
        password = argv[4];
    }

    if (argc >= 6) {
        action = argv[5];
    }

    if (argc >= 7) {
        filename = argv[6];
    }

    printf("[+] Connecting to FTP server %s:%d\n", host, port);
    int control_sockfd = connect_to_host(host, port);

    ftp_login(control_sockfd, username, password);

    if (strcmp(action, "list") == 0) {
        ftp_list(control_sockfd);
    } else if (strcmp(action, "get") == 0) {
        if (filename == NULL) {
            fprintf(stderr, "Missing filename for get action\n");
            close(control_sockfd);
            return EXIT_FAILURE;
        }
        ftp_retr(control_sockfd, filename);
    } else {
        fprintf(stderr, "Unknown action: %s\n", action);
        print_usage(argv[0]);
    }

    send_command(control_sockfd, "QUIT\r\n");
    receive_response(control_sockfd, response, sizeof(response));

    close(control_sockfd);
    return 0;
}
