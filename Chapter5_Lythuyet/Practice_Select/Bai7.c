#include <sys/select.h>
#include <stdio.h>

int sockfd1 = 5, sockfd2 = 6;
fd_set read_fds;
FD_ZERO(&read_fds);
FD_SET(sockfd1, &read_fds);
FD_SET(sockfd2, &read_fds);

int maxfd = (sockfd1 > sockfd2) ? sockfd1 : sockfd2;
select(maxfd + 1, &read_fds, NULL, NULL, NULL);

printf("Select theo dõi sockfd1 và sockfd2\n");