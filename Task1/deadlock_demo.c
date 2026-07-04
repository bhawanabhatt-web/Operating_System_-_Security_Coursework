#include <stdio.h>
#include <pthread.h>
#include <unistd.h>   // for sleep()

pthread_mutex_t m1, m2;

void* thread_A(void* arg) {
    printf("Thread A: Trying to lock m1...\n");
    pthread_mutex_lock(&m1);
    printf("Thread A: Locked m1. Sleeping 1 sec...\n");
    sleep(1);

    printf("Thread A: Trying to lock m2...\n");
    pthread_mutex_lock(&m2);   // This will block because Thread B holds m2
    printf("Thread A: Locked m2! (won't reach here)\n");

    pthread_mutex_unlock(&m2);
    pthread_mutex_unlock(&m1);
    return NULL;
}

void* thread_B(void* arg) {
    printf("Thread B: Trying to lock m2...\n");
    pthread_mutex_lock(&m2);
    printf("Thread B: Locked m2. Sleeping 1 sec...\n");
    sleep(1);

    printf("Thread B: Trying to lock m1...\n");
    pthread_mutex_lock(&m1);   // This will block because Thread A holds m1
    printf("Thread B: Locked m1! (won't reach here)\n");

    pthread_mutex_unlock(&m1);
    pthread_mutex_unlock(&m2);
    return NULL;
}

int main() {
    pthread_t t1, t2;

    pthread_mutex_init(&m1, NULL);
    pthread_mutex_init(&m2, NULL);

    pthread_create(&t1, NULL, thread_A, NULL);
    pthread_create(&t2, NULL, thread_B, NULL);

    pthread_join(t1, NULL);  // This will never return (deadlock)
    pthread_join(t2, NULL);

    pthread_mutex_destroy(&m1);
    pthread_mutex_destroy(&m2);
    printf("Program finished (will not print).\n");
    return 0;
}
