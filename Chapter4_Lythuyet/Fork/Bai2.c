#include <stdio.h>
#include <unistd.h>

int main() {
    fork();
    fork();
    printf("PID = %d, PPID = %d\n", getpid(), getppid());
    return 0;
}