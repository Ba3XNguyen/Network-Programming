#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>

pthread_mutex_t lock = PTHREAD_MUTEX_INITIALIZER;

void *write_file(void *arg) {
    FILE *f = fopen("output.txt", "a");

    for (int i = 0; i < 5; i++) {
        pthread_mutex_lock(&lock);

        fprintf(f, "Thread %lu ghi dong %d\n", pthread_self(), i);

        pthread_mutex_unlock(&lock);
    }

    fclose(f);
    return NULL;
}

int main() {
    pthread_t t1, t2;

    pthread_create(&t1, NULL, write_file, NULL);
    pthread_create(&t2, NULL, write_file, NULL);

    pthread_join(t1, NULL);
    pthread_join(t2, NULL);

    return 0;
}