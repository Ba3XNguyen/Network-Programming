#include <sys/select.h>
#include <stdio.h>

fd_set read_fds;
int sockfd = 5;
struct timeval timeout;

FD_ZERO(&read_fds);
FD_SET(sockfd, &read_fds);

timeout.tv_sec = 5;  // timeout 5 giây
timeout.tv_usec = 0;

select(sockfd + 1, &read_fds, NULL, NULL, &timeout);

printf("Select chờ tối đa 5 giây\n");