#include <stdio.h>
#include <unistd.h>

int main() {
    if (fork() != 0) {
        printf("Tien trinh cha\n");
    } else {
        printf("Tien trinh con\n");
    }
    return 0;
}