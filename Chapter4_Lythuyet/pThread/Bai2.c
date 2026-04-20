#include <pthread.h>
#include <stdio.h>

void *print_number(void *arg) {
    int *num = (int *)arg;
    printf("Number: %d\n", *num);
    return NULL;
}

int main() {
    pthread_t tid;
    int n = 42;

    pthread_create(&tid, NULL, print_number, &n);
    pthread_join(tid, NULL);

    return 0;
}