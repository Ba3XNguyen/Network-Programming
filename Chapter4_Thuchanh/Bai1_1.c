#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>

int main() {
    pid_t pid = fork();

    if (pid == 0) {
        // tiến trình con
        printf("Con bat dau, PID = %d\n", getpid());
        sleep(2); // để thấy rõ tiến trình con chạy
        printf("Con ket thuc\n");
        exit(0);
    } 
    else if (pid > 0) {
        // tiến trình cha
        printf("Cha dang ngu, PID = %d\n", getpid());
        sleep(30); // không wait thì tạo zombie
    } 
    else {
        perror("fork loi");
    }

    return 0;
}