#include <arpa/inet.h>
#include <errno.h>
#include <netinet/in.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <unistd.h>

#define PORT 8080
#define BUFFER_SIZE 4096
#define ROOT_DIR "."

void reap_children(int sig) {
    (void)sig;
    while (waitpid(-1, NULL, WNOHANG) > 0);
}

int send_all(int sock, const void *buf, size_t len) {
    const char *p = buf;
    size_t sent = 0;

    while (sent < len) {
        ssize_t n = send(sock, p + sent, len - sent, 0);
        if (n <= 0) return -1;
        sent += n;
    }
    return 0;
}

const char *get_mime_type(const char *path) {
    const char *ext = strrchr(path, '.');

    if (!ext) return "application/octet-stream";
    if (strcmp(ext, ".html") == 0) return "text/html";
    if (strcmp(ext, ".css") == 0) return "text/css";
    if (strcmp(ext, ".jpg") == 0 || strcmp(ext, ".jpeg") == 0) return "image/jpeg";
    if (strcmp(ext, ".png") == 0) return "image/png";
    if (strcmp(ext, ".txt") == 0) return "text/plain";

    return "application/octet-stream";
}

void send_error(int client, int code, const char *text) {
    char body[512];
    char header[1024];

    int body_len = snprintf(body, sizeof(body),
        "<html><body><h1>%d %s</h1></body></html>", code, text);

    int header_len = snprintf(header, sizeof(header),
        "HTTP/1.1 %d %s\r\n"
        "Content-Type: text/html\r\n"
        "Content-Length: %d\r\n"
        "Connection: close\r\n"
        "\r\n",
        code, text, body_len);

    send_all(client, header, header_len);
    send_all(client, body, body_len);
}

void build_path(const char *uri, char *path, size_t path_size) {
    if (strcmp(uri, "/") == 0) {
        snprintf(path, path_size, "%s/index.html", ROOT_DIR);
    } else {
        snprintf(path, path_size, "%s%s", ROOT_DIR, uri);
    }
}

void send_file(int client, const char *path, int send_body) {
    FILE *fp = fopen(path, "rb");
    if (!fp) {
        send_error(client, 404, "Not Found");
        return;
    }

    struct stat st;
    if (stat(path, &st) < 0) {
        fclose(fp);
        send_error(client, 404, "Not Found");
        return;
    }

    char header[1024];
    int header_len = snprintf(header, sizeof(header),
        "HTTP/1.1 200 OK\r\n"
        "Content-Type: %s\r\n"
        "Content-Length: %ld\r\n"
        "Connection: close\r\n"
        "\r\n",
        get_mime_type(path), st.st_size);

    send_all(client, header, header_len);

    if (!send_body) {
        fclose(fp);
        return;
    }

    char buffer[BUFFER_SIZE];
    size_t n;

    while ((n = fread(buffer, 1, sizeof(buffer), fp)) > 0) {
        send_all(client, buffer, n);
    }

    fclose(fp);
}

char *get_body(char *request) {
    char *body = strstr(request, "\r\n\r\n");
    if (body == NULL) return NULL;
    return body + 4;
}

void handle_get(int client, const char *uri) {
    char path[2048];
    build_path(uri, path, sizeof(path));
    send_file(client, path, 1);
}

void handle_head(int client, const char *uri) {
    char path[2048];
    build_path(uri, path, sizeof(path));
    send_file(client, path, 0);
}

void handle_post(int client, char *request) {
    char *body = get_body(request);

    if (body == NULL) {
        body = "";
    }

    printf("POST body: %s\n", body);

    FILE *fp = fopen("post_data.txt", "a");
    if (fp) {
        fprintf(fp, "%s\n", body);
        fclose(fp);
    }

    const char *html =
        "<html>"
        "<body>"
        "<h1>POST received</h1>"
        "<p>Data has been saved to post_data.txt</p>"
        "</body>"
        "</html>";

    char header[1024];
    int header_len = snprintf(header, sizeof(header),
        "HTTP/1.1 200 OK\r\n"
        "Content-Type: text/html\r\n"
        "Content-Length: %ld\r\n"
        "Connection: close\r\n"
        "\r\n",
        strlen(html));

    send_all(client, header, header_len);
    send_all(client, html, strlen(html));
}

void handle_put(int client, const char *uri, char *request) {
    char *body = get_body(request);

    if (body == NULL) {
        body = "";
    }

    printf("PUT resource: %s\n", uri);
    printf("PUT body: %s\n", body);

    FILE *fp = fopen("put_data.txt", "a");
    if (fp) {
        fprintf(fp, "Resource: %s\nBody: %s\n\n", uri, body);
        fclose(fp);
    }

    const char *html =
        "<html><body><h1>PUT received</h1></body></html>";

    char header[1024];
    int header_len = snprintf(header, sizeof(header),
        "HTTP/1.1 200 OK\r\n"
        "Content-Type: text/html\r\n"
        "Content-Length: %ld\r\n"
        "Connection: close\r\n"
        "\r\n",
        strlen(html));

    send_all(client, header, header_len);
    send_all(client, html, strlen(html));
}

void handle_delete(int client, const char *uri) {
    printf("DELETE resource: %s\n", uri);

    const char *html =
        "<html><body><h1>DELETE received</h1></body></html>";

    char header[1024];
    int header_len = snprintf(header, sizeof(header),
        "HTTP/1.1 200 OK\r\n"
        "Content-Type: text/html\r\n"
        "Content-Length: %ld\r\n"
        "Connection: close\r\n"
        "\r\n",
        strlen(html));

    send_all(client, header, header_len);
    send_all(client, html, strlen(html));
}

void handle_client(int client) {
    char request[BUFFER_SIZE + 1];

    ssize_t n = recv(client, request, BUFFER_SIZE, 0);
    if (n <= 0) {
        close(client);
        return;
    }

    request[n] = '\0';

    char method[16], uri[1024], version[32];

    if (sscanf(request, "%15s %1023s %31s", method, uri, version) != 3) {
        send_error(client, 400, "Bad Request");
        close(client);
        return;
    }

    printf("Request: %s %s %s\n", method, uri, version);

    if (strstr(uri, "..") != NULL) {
        send_error(client, 403, "Forbidden");
        close(client);
        return;
    }

    if (strcmp(method, "GET") == 0) {
        handle_get(client, uri);
    } else if (strcmp(method, "HEAD") == 0) {
        handle_head(client, uri);
    } else if (strcmp(method, "POST") == 0) {
        handle_post(client, request);
    } else if (strcmp(method, "PUT") == 0) {
        handle_put(client, uri, request);
    } else if (strcmp(method, "DELETE") == 0) {
        handle_delete(client, uri);
    } else {
        send_error(client, 405, "Method Not Allowed");
    }

    close(client);
}

int main() {
    int server_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (server_fd < 0) {
        perror("socket");
        return 1;
    }

    int opt = 1;
    setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

    struct sockaddr_in server_addr;
    memset(&server_addr, 0, sizeof(server_addr));

    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;
    server_addr.sin_port = htons(PORT);

    if (bind(server_fd, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0) {
        perror("bind");
        close(server_fd);
        return 1;
    }

    if (listen(server_fd, 10) < 0) {
        perror("listen");
        close(server_fd);
        return 1;
    }

    signal(SIGCHLD, reap_children);

    printf("HTTP server running at http://localhost:%d/\n", PORT);

    while (1) {
        int client = accept(server_fd, NULL, NULL);
        if (client < 0) {
            if (errno == EINTR) continue;
            perror("accept");
            continue;
        }

        pid_t pid = fork();

        if (pid == 0) {
            close(server_fd);
            handle_client(client);
            exit(0);
        } else if (pid > 0) {
            close(client);
        } else {
            perror("fork");
            close(client);
        }
    }

    close(server_fd);
    return 0;
}