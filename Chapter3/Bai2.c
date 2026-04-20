#include <stdio.h>
#include <arpa/inet.h>

int main() {

    char ip_str[100];
    struct in_addr addr;
    char ip_result[INET_ADDRSTRLEN];

    // Nhập IP từ người dùng
    printf("Nhap dia chi IP: ");
    scanf("%s", ip_str);

    // Chuyển từ string -> binary
    if (inet_pton(AF_INET, ip_str, &addr) <= 0) {
        printf("Dia chi IP khong hop le\n");
        return -1;
    }

    printf("Chuyen sang dang nhi phan thanh cong\n");

    // Chuyển từ binary -> string
    if (inet_ntop(AF_INET, &addr, ip_result, INET_ADDRSTRLEN) == NULL) {
        printf("Khong the chuyen doi lai sang string\n");
        return -1;
    }

    printf("Dia chi IP sau khi chuyen lai: %s\n", ip_result);

    return 0;
}