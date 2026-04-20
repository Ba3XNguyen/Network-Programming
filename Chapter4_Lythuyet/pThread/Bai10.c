#include <pthread.h>
#include <stdio.h>

void *thread_func(void *arg) {
    printf("Luong ket thuc\n");
    pthread_exit(NULL); // kết thúc thread
}

int main() {
    pthread_t tid;

    pthread_create(&tid, NULL, thread_func, NULL);
    pthread_join(tid, NULL);

    return 0;
}