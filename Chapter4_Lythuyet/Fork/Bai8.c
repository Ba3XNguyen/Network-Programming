#include <stdio.h>
#include <unistd.h>
#include <sys/wait.h>
#include <stdlib.h>

int main() {
    for (int i = 0; i < 5; i++) {
        if (fork() == 0) {
            printf("Tien trinh con %d, PID = %d\n", i, getpid());
            _exit(0);
        }
    }

    // Cha đợi tất cả con
    for (int i = 0; i < 5; i++) {
        wait(NULL);
    }

    printf("Tien trinh cha ket thuc\n");
    return 0;
}