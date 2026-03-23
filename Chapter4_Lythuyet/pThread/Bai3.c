#include <pthread.h>
#include <stdio.h>
#include <unistd.h>

void *thread_func(void *arg) {
    printf("Hello from detached thread!\n");
    return NULL;
}

int main() {
    pthread_t tid;

    pthread_create(&tid, NULL, thread_func, NULL);

    pthread_detach(tid); // tách luồng

    sleep(1); // đợi thread chạy xong

    return 0;
}