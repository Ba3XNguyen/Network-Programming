#include <pthread.h>
#include <stdio.h>

int global_var = 0;

void *thread_func(void *arg) {
    int local_var = 0;

    global_var++;
    local_var++;

    printf("Thread %lu:\n", pthread_self());
    printf("  Dia chi global_var: %p, Gia tri: %d\n", &global_var, global_var);
    printf("  Dia chi local_var : %p, Gia tri: %d\n", &local_var, local_var);

    return NULL;
}

int main() {
    pthread_t t1, t2;

    pthread_create(&t1, NULL, thread_func, NULL);
    pthread_create(&t2, NULL, thread_func, NULL);

    pthread_join(t1, NULL);
    pthread_join(t2, NULL);

    return 0;
}