#include <stdio.h>
#include <unistd.h>
#include <sys/wait.h>

int main() {
    pid_t pid1 = fork();

    if (pid1 == 0) {
        printf("Tien trinh con 1, PID = %d\n", getpid());
    } else {
        pid_t pid2 = fork();

        if (pid2 == 0) {
            printf("Tien trinh con 2, PID = %d\n", getpid());
        } else {
            printf("Tien trinh cha, PID = %d\n", getpid());
            wait(NULL);
            wait(NULL);
        }
    }

    return 0;
}