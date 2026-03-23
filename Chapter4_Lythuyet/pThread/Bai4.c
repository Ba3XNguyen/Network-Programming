#include <pthread.h>
#include <stdio.h>

void *print_index(void *arg) {
    int idx = *(int *)arg;
    printf("Thread %d\n", idx);
    return NULL;
}

int main() {
    pthread_t tids[5];
    int indices[5];

    for (int i = 0; i < 5; i++) {
        indices[i] = i;
        pthread_create(&tids[i], NULL, print_index, &indices[i]);
    }

    for (int i = 0; i < 5; i++) {
        pthread_join(tids[i], NULL);
    }

    return 0;
}