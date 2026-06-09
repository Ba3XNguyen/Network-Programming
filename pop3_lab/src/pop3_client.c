#include <arpa/inet.h>
#include <errno.h>
#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>

#define LINE_SIZE 4096
#define CMD_SIZE 1024

static void die(const char *message) {
    perror(message);
    exit(EXIT_FAILURE);
}

int connect_to_server(const char *server_ip, int server_port) {
    int sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0) {
        die("socket");
    }

    struct sockaddr_in server_addr;
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons((uint16_t)server_port);

    if (inet_pton(AF_INET, server_ip, &server_addr.sin_addr) <= 0) {
        close(sockfd);
        fprintf(stderr, "Invalid server IP address: %s\n", server_ip);
        exit(EXIT_FAILURE);
    }

    if (connect(sockfd, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0) {
        close(sockfd);
        die("connect");
    }

    return sockfd;
}

int send_all(int sockfd, const char *buffer) {
    size_t total_sent = 0;
    size_t length = strlen(buffer);

    while (total_sent < length) {
        ssize_t sent = send(sockfd, buffer + total_sent, length - total_sent, 0);
        if (sent < 0) {
            if (errno == EINTR) {
                continue;
            }
            return -1;
        }
        if (sent == 0) {
            return -1;
        }
        total_sent += (size_t)sent;
    }

    return 0;
}

int recv_line(int sockfd, char *buffer, size_t buffer_size) {
    if (buffer_size == 0) {
        return -1;
    }

    size_t index = 0;
    while (index + 1 < buffer_size) {
        char c;
        ssize_t n = recv(sockfd, &c, 1, 0);
        if (n < 0) {
            if (errno == EINTR) {
                continue;
            }
            return -1;
        }
        if (n == 0) {
            break;
        }

        buffer[index++] = c;
        if (c == '\n') {
            break;
        }
    }

    buffer[index] = '\0';
    return (int)index;
}

int check_pop3_ok(const char *response) {
    return strncmp(response, "+OK", 3) == 0;
}

int send_pop3_command(int sockfd, const char *command, char *response, size_t response_size) {
    if (send_all(sockfd, command) < 0) {
        perror("send");
        return -1;
    }

    if (recv_line(sockfd, response, response_size) <= 0) {
        fprintf(stderr, "Failed to receive response from server.\n");
        return -1;
    }

    printf("S: %s", response);
    return check_pop3_ok(response) ? 0 : -1;
}

int read_multiline_response(int sockfd) {
    char line[LINE_SIZE];

    while (1) {
        int n = recv_line(sockfd, line, sizeof(line));
        if (n <= 0) {
            fprintf(stderr, "Failed while reading multiline response.\n");
            return -1;
        }

        if (strcmp(line, ".\r\n") == 0 || strcmp(line, ".\n") == 0 || strcmp(line, ".") == 0) {
            printf("S: %s", line);
            break;
        }

        printf("S: %s", line);
    }

    return 0;
}

static int read_int_from_stdin(const char *prompt) {
    char input[64];
    printf("%s", prompt);
    fflush(stdout);

    if (fgets(input, sizeof(input), stdin) == NULL) {
        return -1;
    }

    char *endptr = NULL;
    long value = strtol(input, &endptr, 10);
    if (endptr == input) {
        return -1;
    }

    return (int)value;
}

static int send_single_line_command(int sockfd, const char *command) {
    char response[LINE_SIZE];
    printf("C: %s", command);
    return send_pop3_command(sockfd, command, response, sizeof(response));
}

static int send_multiline_command(int sockfd, const char *command) {
    char response[LINE_SIZE];
    printf("C: %s", command);

    if (send_pop3_command(sockfd, command, response, sizeof(response)) < 0) {
        fprintf(stderr, "POP3 command failed: %s", response);
        return -1;
    }

    return read_multiline_response(sockfd);
}

void run_pop3_menu(int sockfd) {
    while (1) {
        printf("\n===== POP3 Client Menu =====\n");
        printf("1. Show mailbox status\n");
        printf("2. List messages\n");
        printf("3. Retrieve a message\n");
        printf("4. Quit\n");

        int choice = read_int_from_stdin("Choose an option: ");
        char command[CMD_SIZE];

        switch (choice) {
            case 1:
                snprintf(command, sizeof(command), "STAT\r\n");
                if (send_single_line_command(sockfd, command) < 0) {
                    fprintf(stderr, "STAT failed.\n");
                }
                break;

            case 2:
                snprintf(command, sizeof(command), "LIST\r\n");
                if (send_multiline_command(sockfd, command) < 0) {
                    fprintf(stderr, "LIST failed.\n");
                }
                break;

            case 3: {
                int message_id = read_int_from_stdin("Message number: ");
                if (message_id <= 0) {
                    fprintf(stderr, "Invalid message number.\n");
                    break;
                }
                snprintf(command, sizeof(command), "RETR %d\r\n", message_id);
                if (send_multiline_command(sockfd, command) < 0) {
                    fprintf(stderr, "RETR failed.\n");
                }
                break;
            }

            case 4:
                snprintf(command, sizeof(command), "QUIT\r\n");
                send_single_line_command(sockfd, command);
                return;

            default:
                fprintf(stderr, "Invalid option. Please choose 1-4.\n");
                break;
        }
    }
}

static void login_pop3(int sockfd, const char *username, const char *password) {
    char response[LINE_SIZE];
    char command[CMD_SIZE];

    if (recv_line(sockfd, response, sizeof(response)) <= 0) {
        fprintf(stderr, "Could not read POP3 greeting.\n");
        close(sockfd);
        exit(EXIT_FAILURE);
    }
    printf("S: %s", response);

    if (!check_pop3_ok(response)) {
        fprintf(stderr, "Server greeting is not +OK.\n");
        close(sockfd);
        exit(EXIT_FAILURE);
    }

    snprintf(command, sizeof(command), "USER %s\r\n", username);
    printf("C: USER %s\n", username);
    if (send_pop3_command(sockfd, command, response, sizeof(response)) < 0) {
        fprintf(stderr, "USER failed: %s", response);
        close(sockfd);
        exit(EXIT_FAILURE);
    }

    snprintf(command, sizeof(command), "PASS %s\r\n", password);
    printf("C: PASS ********\n");
    if (send_pop3_command(sockfd, command, response, sizeof(response)) < 0) {
        fprintf(stderr, "PASS failed: %s", response);
        close(sockfd);
        exit(EXIT_FAILURE);
    }
}

int main(int argc, char *argv[]) {
    if (argc != 5) {
        fprintf(stderr, "Usage: %s <server_ip> <server_port> <username> <password>\n", argv[0]);
        fprintf(stderr, "Example: %s 127.0.0.1 1110 alice password\n", argv[0]);
        return EXIT_FAILURE;
    }

    const char *server_ip = argv[1];
    int server_port = atoi(argv[2]);
    const char *username = argv[3];
    const char *password = argv[4];

    if (server_port <= 0 || server_port > 65535) {
        fprintf(stderr, "Invalid server port: %s\n", argv[2]);
        return EXIT_FAILURE;
    }

    int sockfd = connect_to_server(server_ip, server_port);
    printf("Connected to POP3 server %s:%d\n", server_ip, server_port);

    login_pop3(sockfd, username, password);
    printf("Login successful.\n");

    run_pop3_menu(sockfd);

    close(sockfd);
    printf("Connection closed.\n");
    return EXIT_SUCCESS;
}
