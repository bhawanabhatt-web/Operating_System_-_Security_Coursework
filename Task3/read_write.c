#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/stat.h>
#define BUF_SIZE 256

int writeToFile(const char *path, const char *text) {
    /* O_APPEND keeps previous content; O_CREAT makes the file if missing */
    int fd = open(path, O_WRONLY | O_CREAT | O_APPEND, 0600);
    if (fd == -1) { perror("open (write)"); return -1; }

    ssize_t written = write(fd, text, strlen(text));
    close(fd);

    if (written == -1) { perror("write"); return -1; }
    printf("Wrote %ld bytes to %s\n", (long)written, path);
    return 0;
}

int readFromFile(const char *path) {
    /* check read permission explicitly before opening the file */
    if (access(path, R_OK) != 0) {
        printf("Access denied: no read permission for %s\n", path);
        return -1;
    }

    int fd = open(path, O_RDONLY);
    if (fd == -1) { perror("open (read)"); return -1; }

    char buffer[BUF_SIZE];
    ssize_t n;
    printf("----- Contents of %s -----\n", path);
    while ((n = read(fd, buffer, BUF_SIZE - 1)) > 0) {
        buffer[n] = '\0';
        printf("%s", buffer);
    }
    printf("\n---------------------------\n");
    close(fd);
    return 0;
}

int deleteFile(const char *path) {
    if (unlink(path) == 0) {
        printf("File %s deleted.\n", path);
        return 0;
    }
    perror("unlink");
    return -1;
}

int main() {
    const char *path = "notes.txt";
     writeToFile(path, "Line 1: Operating Systems coursework\n");
    writeToFile(path, "Line 2: File read/write demonstration\n");

    readFromFile(path);

    /* remove read permission to show access() blocking a read attempt */
    chmod(path, 0200); /* write-only for owner, no read bit set */
    printf("\nRemoved read permission, attempting read again...\n");
    readFromFile(path);
 
    /* restore permission and clean up */
    chmod(path, 0600);
    deleteFile(path);
 
    return 0;
}

