#include <stdio.h>
#include <stdlib.h>

typedef struct {
    char name[10];
    int burst;
    int remaining;
} Process;

void round_robin(Process p[], int n, int quantum) {
    int time = 0;
    int done = 0;
    while (done < n) {
        for (int i = 0; i < n; i++) {
            if (p[i].remaining > 0) {
                int exec = (p[i].remaining < quantum) ? p[i].remaining : quantum;
                p[i].remaining -= exec;
                time += exec;
                printf("%s runs for %d ms (total time %d)\n", p[i].name, exec, time);
                if (p[i].remaining == 0) {
                    done++;
                    printf("%s finished at %d ms\n", p[i].name, time);
                }
            }
        }
    }
}

int main() {
    Process processes[] = {{"P1",10,10},{"P2",5,5},{"P3",8,8}};
    int n = 3;
    int quantum = 3;
    round_robin(processes, n, quantum);
    return 0;
}
