#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>

#define PORT 8080
#define MAXLINE 1024

int main() {

    int listenfd, connfd;
    struct sockaddr_in servaddr, cliaddr;
    socklen_t len;
    char buffer[MAXLINE];
    int n;

    // tạo socket
    listenfd = socket(AF_INET, SOCK_STREAM, 0);

    // khởi tạo địa chỉ server
    memset(&servaddr, 0, sizeof(servaddr));
    servaddr.sin_family = AF_INET;
    servaddr.sin_addr.s_addr = htonl(INADDR_ANY);
    servaddr.sin_port = htons(PORT);

    // bind
    bind(listenfd, (struct sockaddr *)&servaddr, sizeof(servaddr));

    // listen
    listen(listenfd, 5);

    printf("Server dang cho ket noi...\n");

    len = sizeof(cliaddr);

    // accept client
    connfd = accept(listenfd, (struct sockaddr *)&cliaddr, &len);

    printf("Client da ket noi\n");

    // read message
    n = read(connfd, buffer, MAXLINE);
    buffer[n] = '\0';

    printf("Nhan tu client: %s\n", buffer);

    // gửi phản hồi
    char reply[] = "Hello from server";
    write(connfd, reply, strlen(reply));

    close(connfd);
    close(listenfd);

    return 0;
}