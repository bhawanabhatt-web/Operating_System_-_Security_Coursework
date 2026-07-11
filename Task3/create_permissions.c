#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/stat.h>
#include <errno.h>

/* convert a stat mode into a human readable "rwxrwxrwx" string */
void modeToString(mode_t mode, char *out) {
    const char perms[9] = { 'r','w','x','r','w','x','r','w','x' };
    mode_t bits[9] = {
        S_IRUSR, S_IWUSR, S_IXUSR,
        S_IRGRP, S_IWGRP, S_IXGRP,
        S_IROTH, S_IWOTH, S_IXOTH
    };
    for (int i = 0; i < 9; i++)
        out[i] = (mode & bits[i]) ? perms[i] : '-';
    out[9] = '\0';
}

void printPermissions(const char *path) {
    struct stat st;
    if (stat(path, &st) != 0) { perror("stat"); return; }
    char str[10];
    modeToString(st.st_mode, str);
    printf("Permissions of %-20s : %s (octal %o)\n",
           path, str, st.st_mode & 0777);
}

int main() {
    const char *path = "secure_test.txt";

    /* Step 1: create the file (fails safely if it already exists) */
    int fd = open(path, O_CREAT | O_WRONLY | O_EXCL, 0600);
    if (fd == -1) {
        if (errno == EEXIST)
            printf("File already exists, continuing with existing file.\n");
        else
            perror("open failed");
    } else {
        write(fd, "Confidential coursework data\n", 30);
        close(fd);
        printf("File '%s' created successfully.\n", path);
    }

    printPermissions(path);

    /* Step 2: owner = read/write, group = read only, others = no access */
    if (chmod(path, S_IRUSR | S_IWUSR | S_IRGRP) == 0)
        printf("\nPermissions updated: owner=rw, group=r, others=none\n");
    else
        perror("chmod failed");

    printPermissions(path);

    /* Step 3: show a second example - a shared/executable script file */
    const char *script = "shared_script.sh";
    fd = open(script, O_CREAT | O_WRONLY | O_TRUNC, 0644);
    if (fd != -1) {
        write(fd, "#!/bin/bash\necho Report generated\n", 35);
        close(fd);
    }
    /* owner=rwx, group=r-x, others=r-x : typical for a shared script */
    chmod(script, S_IRUSR | S_IWUSR | S_IXUSR |
                  S_IRGRP | S_IXGRP |
                  S_IROTH | S_IXOTH);
    printf("\n");
    printPermissions(script);

    return 0;
}
