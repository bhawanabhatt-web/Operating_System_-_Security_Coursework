#include <stdio.h>
#define NUM_FRAMES 4   /* physical memory can only hold 4 pages at once */
int main() {
    /* Step 1: empty array representing 4 physical memory frames.*/
    int frames[NUM_FRAMES] = {-1, -1, -1, -1};

    /* Step 2: another array holding the page reference string -
       the sequence of logical pages the CPU requests. */
    int pages[] = {1, 2, 3, 4, 5, 2};
    int n = sizeof(pages) / sizeof(pages[0]);
    int front = 0;
    int loaded = 0;

    printf("===== Loading pages into empty frames =====\n");
    for (int i = 0; i < n; i++) {
        int page = pages[i];

        /* check if page is already resident (hit) */
        int hit = 0;
        for (int f = 0; f < NUM_FRAMES; f++) {
            if (frames[f] == page) { hit = 1; break; }
        }

        if (hit) {
            printf("Page %d -> already in memory (HIT)\n", page);
            continue;
        }

        if (loaded < NUM_FRAMES) {
            /* Step 3: array still has empty slots, just load the value */
            int *slot = &frames[loaded];
            *slot = page;                  /* Step 4: dereference to store */
            printf("Page %d -> loaded into frame %d\n", page, loaded);
            loaded++;
        } else {
            /* Step 5: array is full - use FIFO to evict the oldest page */
            int *slot = &frames[front];       /* pointer to oldest frame  */
            printf("Frame table FULL. Evicting page %d from frame %d "
                   "(FIFO)\n", *slot, front);
            *slot = page;
            printf("Page %d -> loaded into frame %d\n", page, front);
            front = (front + 1) % NUM_FRAMES;
        }
        /* show the current state of the frame buffer */
        printf("Buffer state: [ ");
        for (int f = 0; f < NUM_FRAMES; f++) printf("%d ", frames[f]);
        printf("]\n\n");
    }

    return 0;
}
