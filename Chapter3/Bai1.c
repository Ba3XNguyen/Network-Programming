#include <stdio.h>
#include <string.h>
#include <arpa/inet.h>
#include <netinet/in.h>

int main() {

    struct sockaddr_in addr;

    // Reset toàn bộ struct
    memset(&addr, 0, sizeof(addr));

    // Thiết lập địa chỉ IPv4
    addr.sin_family = AF_INET;

    // Thiết lập port (phải dùng htons)
    addr.sin_port = htons(8080);

    // Thiết lập địa chỉ IP
    if (inet_pton(AF_INET, "192.168.1.1", &addr.sin_addr) <= 0) {
        printf("Invalid IP address\n");
        return -1;
    }

    // In thông tin đã khởi tạo
    printf("Sockaddr_in initialized successfully\n");
    printf("Address Family : AF_INET (IPv4)\n");
    printf("Port           : %d\n", ntohs(addr.sin_port));
    printf("IP Address     : 192.168.1.1\n");

    return 0;
}