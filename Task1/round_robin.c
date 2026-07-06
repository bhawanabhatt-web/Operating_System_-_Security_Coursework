#include<stdio.h>
#include<stdlib.h>

#define NUM_PROCESSES 5
#define TIME_QUANTUM 4

typedef struct
{
    int pid;
    int burst_time;
    int remaining_time;
} Process;

void round_robin_scheduler(Process processes[], int n, int quantum)
{
    int completed = 0;
    int current_time = 0;

    int completion_time[n];
    int turnaround_time[n];
    int waiting_time[n];

    float total_waiting = 0.0;
    float total_turnaround = 0.0;

    printf("\n=========================================================\n");
    printf("               Round Robin Scheduling\n");
    printf("=========================================================\n");

    printf("\nTime Quantum = %d ms\n\n", quantum);

    while (completed < n)
    {
        for (int i = 0; i < n; i++)
        {
            if (processes[i].remaining_time <= 0)
                continue;

            int execution_time;

            if (processes[i].remaining_time > quantum)
                execution_time = quantum;
            else
                execution_time = processes[i].remaining_time;

            printf("Time %2d -> %2d : Process P%d executes for %d ms\n",
                   current_time,
                   current_time + execution_time,
                   processes[i].pid,
                   execution_time);

            current_time += execution_time;
            processes[i].remaining_time -= execution_time;

            if (processes[i].remaining_time == 0)
            {
                completion_time[i] = current_time;
                turnaround_time[i] = completion_time[i];
                waiting_time[i] = turnaround_time[i] - processes[i].burst_time;

                total_waiting += waiting_time[i];
                total_turnaround += turnaround_time[i];

                printf("Process P%d completed.\n\n",
                       processes[i].pid);

                completed++;
            }
        }
    }

    printf("---------------------------------------------------------------\n");
    printf("%-8s %-8s %-12s %-12s %-10s\n",
           "PID",
           "Burst",
           "Completion",
           "Turnaround",
           "Waiting");

    printf("---------------------------------------------------------------\n");

    for (int i = 0; i < n; i++)
    {
        printf("%-8d %-8d %-12d %-12d %-10d\n",
               processes[i].pid,
               processes[i].burst_time,
               completion_time[i],
               turnaround_time[i],
               waiting_time[i]);
    }

    printf("---------------------------------------------------------------\n");

    printf("\nAverage Waiting Time    : %.2f ms\n",
           total_waiting / n);

    printf("Average Turnaround Time : %.2f ms\n",
           total_turnaround / n);
}
int main(){
Process processes[NUM_PROCESSES] =
{
    {1,12,12},
    {2,7,7},
    {3,9,9},
    {4,5,5},
    {5,11,11}
};

round_robin_scheduler(processes,
                      NUM_PROCESSES,
                      TIME_QUANTUM);

}
