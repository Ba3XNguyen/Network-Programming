#include <pthread.h>
#include <stdio.h>

int counter = 0;

void *increment(void *arg) {
    for (int i = 0; i < 1000; i++) {
        counter++;
    }
    return NULL;
}

int main() {
    pthread_t tid1, tid2;

    pthread_create(&tid1, NULL, increment, NULL);
    pthread_create(&tid2, NULL, increment, NULL);

    pthread_join(tid1, NULL);
    pthread_join(tid2, NULL);

    printf("Counter: %d\n", counter);

    return 0;
}