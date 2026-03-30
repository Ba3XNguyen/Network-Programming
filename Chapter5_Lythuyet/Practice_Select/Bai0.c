#include <stdio.h>
#include <sys/select.h>

int main() {

    fd_set fdset;

    FD_ZERO(&fdset);

    FD_SET(3, &fdset); // set bit 3
    FD_SET(4, &fdset); // set bit 4

    printf("fds_bits[0] = %ld\n", fdset.fds_bits[0]);

    for (int i = sizeof(fdset.fds_bits[0])*8 - 1; i >= 0; i--) {
        printf("%d", (fdset.fds_bits[0] >> i) & 1);
    }

    printf("\n");

    return 0;
}