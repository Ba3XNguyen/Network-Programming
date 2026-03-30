#include <sys/select.h>
#include <stdio.h>

fd_set read_fds;
int sockfd = 5;

FD_ZERO(&read_fds);
FD_SET(sockfd, &read_fds);

// Chờ vô hạn cho sockfd sẵn sàng đọc
select(sockfd + 1, &read_fds, NULL, NULL, NULL);

printf("Select trả về, socket có thể đọc\n");