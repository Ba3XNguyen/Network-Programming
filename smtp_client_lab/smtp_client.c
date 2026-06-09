

#include <arpa/inet.h>
#include <errno.h>
#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>

#define BUFFER_SIZE 4096
#define COMMAND_SIZE 2048
#define LINE_SIZE 1024

static void usage(const char *program_name) {
    fprintf(stderr,
            "Usage: %s <server_ip> <server_port> <sender> <recipient> <subject>\n"
            "Example: %s 127.0.0.1 1025 alice@example.com bob@example.com \"Test email\"\n",
            program_name, program_name);
}

static int connect_to_server(const char *server_ip, int server_port) {
    int sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0) {
        perror("socket");
        return -1;
    }

    struct sockaddr_in server_addr;
    memset(&server_addr, 0, sizeof(server_addr));

    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons((uint16_t)server_port);

    if (inet_pton(AF_INET, server_ip, &server_addr.sin_addr) <= 0) {
        fprintf(stderr, "Invalid IPv4 address: %s\n", server_ip);
        close(sockfd);
        return -1;
    }

    if (connect(sockfd, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0) {
        perror("connect");
        close(sockfd);
        return -1;
    }

    return sockfd;
}

static int send_all(int sockfd, const char *buffer) {
    size_t total_sent = 0;
    size_t length = strlen(buffer);

    while (total_sent < length) {
        ssize_t n = send(sockfd, buffer + total_sent, length - total_sent, 0);
        if (n < 0) {
            perror("send");
            return -1;
        }
        if (n == 0) {
            fprintf(stderr, "send failed: connection closed unexpectedly\n");
            return -1;
        }
        total_sent += (size_t)n;
    }

    return 0;
}

static int recv_response(int sockfd, char *buffer, size_t buffer_size) {
    memset(buffer, 0, buffer_size);

    ssize_t n = recv(sockfd, buffer, buffer_size - 1, 0);
    if (n < 0) {
        perror("recv");
        return -1;
    }

    if (n == 0) {
        fprintf(stderr, "recv failed: server closed the connection\n");
        return -1;
    }

    buffer[n] = '\0';
    printf("S: %s", buffer);

    return 0;
}

static int check_response_code(const char *response, const char *expected_code) {
    if (strncmp(response, expected_code, 3) != 0) {
        fprintf(stderr,
                "Unexpected SMTP response. Expected %s, got: %s\n",
                expected_code,
                response);
        return -1;
    }

    return 0;
}

static int send_smtp_command(int sockfd, const char *command, const char *expected_code) {
    char response[BUFFER_SIZE];

    printf("C: %s", command);

    if (send_all(sockfd, command) < 0) {
        return -1;
    }

    if (recv_response(sockfd, response, sizeof(response)) < 0) {
        return -1;
    }

    if (check_response_code(response, expected_code) < 0) {
        return -1;
    }

    return 0;
}

static int send_email_headers(int sockfd,
                              const char *sender,
                              const char *recipient,
                              const char *subject) {
    char headers[COMMAND_SIZE];

    int written = snprintf(headers,
                           sizeof(headers),
                           "From: %s\r\n"
                           "To: %s\r\n"
                           "Subject: %s\r\n"
                           "\r\n",
                           sender,
                           recipient,
                           subject);

    if (written < 0 || (size_t)written >= sizeof(headers)) {
        fprintf(stderr, "Email headers are too long\n");
        return -1;
    }

    printf("C: [email headers]\n");
    return send_all(sockfd, headers);
}

static int read_email_body_and_send(int sockfd) {
    char line[LINE_SIZE];

    printf("\nEnter email body. End with a line containing only a single dot: .\n");

    while (fgets(line, sizeof(line), stdin) != NULL) {
        size_t len = strlen(line);

        /*
         * Remove trailing '\n' and optional '\r' so we can normalize every line
         * to SMTP CRLF format.
         */
        while (len > 0 && (line[len - 1] == '\n' || line[len - 1] == '\r')) {
            line[len - 1] = '\0';
            len--;
        }

        /*
         * A line containing only "." terminates DATA mode.
         * The terminator must be sent as "\r\n.\r\n" after the body.
         */
        if (strcmp(line, ".") == 0) {
            break;
        }

        /*
         * Dot-stuffing: if a real email body line begins with '.', SMTP requires
         * the client to send an extra dot so it is not confused with the DATA
         * terminator.
         */
        if (line[0] == '.') {
            if (send_all(sockfd, ".") < 0) {
                return -1;
            }
        }

        if (send_all(sockfd, line) < 0) {
            return -1;
        }

        if (send_all(sockfd, "\r\n") < 0) {
            return -1;
        }
    }

    if (ferror(stdin)) {
        perror("fgets");
        return -1;
    }

    printf("C: .\n");
    return send_all(sockfd, ".\r\n");
}

int main(int argc, char *argv[]) {
    if (argc != 6) {
        usage(argv[0]);
        return EXIT_FAILURE;
    }

    const char *server_ip = argv[1];
    char *endptr = NULL;
    long port_value = strtol(argv[2], &endptr, 10);

    if (*argv[2] == '\0' || *endptr != '\0' || port_value < 1 || port_value > 65535) {
        fprintf(stderr, "Invalid server port: %s\n", argv[2]);
        return EXIT_FAILURE;
    }

    int server_port = (int)port_value;
    const char *sender = argv[3];
    const char *recipient = argv[4];
    const char *subject = argv[5];

    int sockfd = connect_to_server(server_ip, server_port);
    if (sockfd < 0) {
        return EXIT_FAILURE;
    }

    char response[BUFFER_SIZE];
    char command[COMMAND_SIZE];

    if (recv_response(sockfd, response, sizeof(response)) < 0 ||
        check_response_code(response, "220") < 0) {
        close(sockfd);
        return EXIT_FAILURE;
    }

    if (send_smtp_command(sockfd, "HELO localhost\r\n", "250") < 0) {
        close(sockfd);
        return EXIT_FAILURE;
    }

    int written = snprintf(command, sizeof(command), "MAIL FROM:<%s>\r\n", sender);
    if (written < 0 || (size_t)written >= sizeof(command)) {
        fprintf(stderr, "MAIL FROM command is too long\n");
        close(sockfd);
        return EXIT_FAILURE;
    }
    if (send_smtp_command(sockfd, command, "250") < 0) {
        close(sockfd);
        return EXIT_FAILURE;
    }

    written = snprintf(command, sizeof(command), "RCPT TO:<%s>\r\n", recipient);
    if (written < 0 || (size_t)written >= sizeof(command)) {
        fprintf(stderr, "RCPT TO command is too long\n");
        close(sockfd);
        return EXIT_FAILURE;
    }
    if (send_smtp_command(sockfd, command, "250") < 0) {
        close(sockfd);
        return EXIT_FAILURE;
    }

    if (send_smtp_command(sockfd, "DATA\r\n", "354") < 0) {
        close(sockfd);
        return EXIT_FAILURE;
    }

    if (send_email_headers(sockfd, sender, recipient, subject) < 0) {
        close(sockfd);
        return EXIT_FAILURE;
    }

    if (read_email_body_and_send(sockfd) < 0) {
        close(sockfd);
        return EXIT_FAILURE;
    }

    if (recv_response(sockfd, response, sizeof(response)) < 0 ||
        check_response_code(response, "250") < 0) {
        close(sockfd);
        return EXIT_FAILURE;
    }

    if (send_smtp_command(sockfd, "QUIT\r\n", "221") < 0) {
        close(sockfd);
        return EXIT_FAILURE;
    }

    close(sockfd);
    printf("Email sent successfully.\n");

    return EXIT_SUCCESS;
}
