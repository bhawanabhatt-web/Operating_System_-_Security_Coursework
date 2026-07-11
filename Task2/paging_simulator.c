#include <stdio.h>
#include <stdlib.h>
#define MAX_FRAMES 10
#define MAX_REFS   50
#define PAGE_SIZE_BYTES 4096

typedef struct {
    int frames[MAX_FRAMES];     /* pages currently held in physical memory */
    int loadTime[MAX_FRAMES];
    int lastUsed[MAX_FRAMES];
    int numFrames;
    int filled;                 /* how many frames are currently occupied  */
    int faults;
    int hits;
    int clock;                  /* logical time counter, increases every access */
} MemorySystem;

void initMemory(MemorySystem *m, int numFrames) {
    m->numFrames = numFrames;
    m->filled = 0;
    m->faults = 0;
    m->hits = 0;
    m->clock = 0;
    for (int i = 0; i < numFrames; i++) {
        m->frames[i] = -1;
        m->loadTime[i] = -1;
        m->lastUsed[i] = -1;
    }
}

void printFrames(MemorySystem *m) {
    printf("Frame state : [ ");
    for (int i = 0; i < m->numFrames; i++) {
        if (m->frames[i] == -1) printf(". ");
        else                    printf("%d ", m->frames[i]);
    }
    printf("]\n");
}

/* find a page already resident in memory, -1 if not found */
int findPage(MemorySystem *m, int page) {
    for (int i = 0; i < m->numFrames; i++)
        if (m->frames[i] == page) return i;
    return -1;
}

/* FIFO replacement: evict the frame that was loaded the longest time ago */
int selectFIFOVictim(MemorySystem *m) {
    int victim = 0;
    for (int i = 1; i < m->numFrames; i++)
        if (m->loadTime[i] < m->loadTime[victim]) victim = i;
    return victim;
}

/* LRU replacement: evict the frame whose page was used longest ago */
int selectLRUVictim(MemorySystem *m) {
    int victim = 0;
    for (int i = 1; i < m->numFrames; i++)
        if (m->lastUsed[i] < m->lastUsed[victim]) victim = i;
    return victim;
}
 
void accessPage(MemorySystem *m, int page, const char *algo) {
    m->clock++;
    int idx = findPage(m, page);
 
    if (idx != -1) {
        /* PAGE HIT */
        m->hits++;
        m->lastUsed[idx] = m->clock;   /* LRU bookkeeping */
        printf("Access page %-2d -> HIT   ", page);
    } else {
        /* PAGE FAULT */
        m->faults++;
        int target;
        if (m->filled < m->numFrames) {
            target = m->filled++;                 /* empty frame available */
        } else if (algo[0] == 'F') {               /* FIFO */
            target = selectFIFOVictim(m);
            printf("(evicting page %-2d) ", m->frames[target]);
        } else {                                   /* LRU */
            target = selectLRUVictim(m);
            printf("(evicting page %-2d) ", m->frames[target]);
        }
        m->frames[target] = page;
        m->loadTime[target] = m->clock;
        m->lastUsed[target] = m->clock;
        printf("Access page %-2d -> FAULT ", page);
    }
    printFrames(m);
}
 
void runSimulation(int refs[], int n, int numFrames, const char *algo) {
    MemorySystem m;
    initMemory(&m, numFrames);
 
    printf("\n================================================\n");
    printf(" Algorithm: %s | Frames: %d | Page size: %d bytes\n",
           algo, numFrames, PAGE_SIZE_BYTES);
    printf("================================================\n");
 
    for (int i = 0; i < n; i++)
        accessPage(&m, refs[i], algo);
 
    double hitRatio  = (double)m.hits  / n * 100.0;
    double missRatio = (double)m.faults / n * 100.0;
 
    printf("------------------------------------------------\n");
    printf("Total references : %d\n", n);
    printf("Page hits        : %d\n", m.hits);
    printf("Page faults      : %d\n", m.faults);
    printf("Hit ratio        : %.2f %%\n", hitRatio);
    printf("Miss ratio       : %.2f %%\n", missRatio);
    printf("------------------------------------------------\n");
}
 
int main(int argc, char *argv[]) {
    /* ---------------- Test case 1 : 3 frames ----------------
       Reference string chosen so FIFO and LRU behave differently
       (classic example used to compare replacement algorithms). */
    int refs1[] = {1, 2, 3, 4, 1, 2, 5, 1, 2, 3, 4, 5};
    int n1 = sizeof(refs1) / sizeof(refs1[0]);
    int frames1 = 3;
 
    printf("TEST CASE 1\nReference string: ");
    for (int i = 0; i < n1; i++) printf("%d ", refs1[i]);
    printf("\n");
 
    runSimulation(refs1, n1, frames1, "FIFO");
    runSimulation(refs1, n1, frames1, "LRU");
 
    /* ---------------- Test case 2 : 4 frames ----------------
       Same length reference string but a larger, configurable
       number of physical frames -> demonstrates that increasing
       frames reduces page faults for both algorithms. */
    int refs2[] = {7, 0, 1, 2, 0, 3, 0, 4, 2, 3, 0, 3, 2};
    int n2 = sizeof(refs2) / sizeof(refs2[0]);
    int frames2 = 4;
 
    printf("\nTEST CASE 2\nReference string: ");
    for (int i = 0; i < n2; i++) printf("%d ", refs2[i]);
    printf("\n");
 
    runSimulation(refs2, n2, frames2, "FIFO");
    runSimulation(refs2, n2, frames2, "LRU");
 
    return 0;
}
 
