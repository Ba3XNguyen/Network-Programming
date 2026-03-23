#include <stdio.h>
#include <unistd.h>

int main() {
    pid_t pid = fork();

    if (pid == 0) {
        printf("Tien trinh con\n");
    } else if (pid > 0) {
        printf("Tien trinh cha\n");
    } else {
        perror("fork that bai");
    }

    return 0;
}