#include <stdio.h>
#include <unistd.h>
#include <sys/wait.h>

int main() {
    if (fork() == 0) {
        printf("Tien trinh con\n");
        _exit(0);
    } else {
        wait(NULL); // Thu dọn tiến trình con
        printf("Tien trinh cha ket thuc\n");
    }
    return 0;
}