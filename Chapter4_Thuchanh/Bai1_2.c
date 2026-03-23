#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <sys/wait.h>

int main() {
    pid_t pid = fork();

    if (pid == 0) {
        // tiến trình con
        printf("Con bat dau, PID = %d\n", getpid());
        sleep(2);
        printf("Con ket thuc\n");
        exit(0);
    } 
    else if (pid > 0) {
        // tiến trình cha
        printf("Cha dang cho, PID = %d\n", getpid());

        // Thu gom tiến trình zombie
        wait(NULL);  
        printf("Tien trinh cha da thu gom tien trinh zombie.\n");

        sleep(5); //
    } 
    else {
        perror("fork loi");
    }

    return 0;
}