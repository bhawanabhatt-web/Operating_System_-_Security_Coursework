#include <stdio.h>
#include <pthread.h>

void* thread_func(void* arg) {
    int id = *(int*)arg;
    printf("Thread %d running\n", id);
    return NULL;
}

int main() {
    pthread_t threads[4];
    int ids[4] = {1,2,3,4};
    for (int i = 0; i < 4; i++)
        pthread_create(&threads[i], NULL, thread_func, &ids[i]);
    for (int i = 0; i < 4; i++)
        pthread_join(threads[i], NULL);
    return 0;
}
