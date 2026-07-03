// fork_demo.c
#include <stdio.h>
#include <unistd.h>
#include <sys/wait.h>

int main() {
    pid_t pid = fork();

    if (pid == 0) {
        printf("Child process: PID = %d\n", getpid());
    } else if (pid > 0) {
        wait(NULL);
        printf("Parent process: PID = %d, child PID = %d\n", getpid(), pid);
    } else {
        perror("fork failed");
    }
    return 0;
}
