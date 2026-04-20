#include <sys/select.h>
#include <stdio.h>

fd_set read_fds;
int sockfd = 5; // giả sử socket đã tạo

FD_ZERO(&read_fds);    // Khởi tạo fd_set rỗng
FD_SET(sockfd, &read_fds); // Thêm sockfd
FD_CLR(sockfd, &read_fds); // Xóa sockfd khỏi tập

printf("Đã xóa sockfd khỏi fd_set\n");