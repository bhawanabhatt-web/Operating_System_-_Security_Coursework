#include <stdio.h>
#include <pthread.h>
#include <unistd.h>

pthread_mutex_t m1, m2;

void* thread_A(void* arg) {
    printf("Thread A: Locking m1...\n");
    pthread_mutex_lock(&m1);
    printf("Thread A: Locked m1. Sleeping 1 sec...\n");
    sleep(1);

    printf("Thread A: Locking m2...\n");
    pthread_mutex_lock(&m2);   // Safe: m1 already held, and order is correct
    printf("Thread A: Locked m2! (success)\n");

    pthread_mutex_unlock(&m2);
    pthread_mutex_unlock(&m1);
    printf("Thread A: Finished.\n");
    return NULL;
}

void* thread_B(void* arg) {
    // Thread B also locks m1 FIRST, then m2
    printf("Thread B: Locking m1...\n");
    pthread_mutex_lock(&m1);
    printf("Thread B: Locked m1. Sleeping 1 sec...\n");
    sleep(1);

    printf("Thread B: Locking m2...\n");
    pthread_mutex_lock(&m2);   // Safe: follows the same order
    printf("Thread B: Locked m2! (success)\n");

    pthread_mutex_unlock(&m2);
    pthread_mutex_unlock(&m1);
    printf("Thread B: Finished.\n");
    return NULL;
}

int main() {
    pthread_t t1, t2;

    pthread_mutex_init(&m1, NULL);
    pthread_mutex_init(&m2, NULL);

    pthread_create(&t1, NULL, thread_A, NULL);
    pthread_create(&t2, NULL, thread_B, NULL);

    pthread_join(t1, NULL);
    pthread_join(t2, NULL);

    pthread_mutex_destroy(&m1);
    pthread_mutex_destroy(&m2);
    printf("Both threads finished successfully — no deadlock!\n");
    return 0;
}
