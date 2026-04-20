#include <sys/select.h>
#include <stdio.h>
// timeout = NULL: chờ vô hạn
select(sockfd + 1, &read_fds, NULL, NULL, NULL);

// timeout > 0: chờ tối đa 5 giây
struct timeval timeout = {5, 0};
select(sockfd + 1, &read_fds, NULL, NULL, &timeout);