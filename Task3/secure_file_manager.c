#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/stat.h>
#include <time.h>

#define MAX_USERS      5
#define MAX_LOGIN_TRY  3
#define BUF_SIZE       512
#define AUDIT_LOG      "audit.log"

typedef struct {
    char username[32];
    unsigned long passHash;   /* djb2 hash of the password, not plaintext */
} User;

/* ---- hardcoded user store (demo only - a real system uses a DB) ---- */
static User users[MAX_USERS];
static int  userCount = 0;

char currentUser[32] = "";

/* ===================== Utility: djb2 hash ===================== */
unsigned long hashPassword(const char *str) {
    unsigned long hash = 5381;
    int c;
    while ((c = *str++))
        hash = ((hash << 5) + hash) + c;   /* hash * 33 + c */
    return hash;
}

void addUser(const char *uname, const char *pass) {
    if (userCount >= MAX_USERS) return;
    strncpy(users[userCount].username, uname, sizeof(users[0].username)-1);
    users[userCount].passHash = hashPassword(pass);
    userCount++;
}

/* ===================== Audit logging ===================== */
void auditLog(const char *action, const char *filename, const char *result) {
    FILE *log = fopen(AUDIT_LOG, "a");
    if (!log) return;
    time_t now = time(NULL);
    char ts[32];
    strftime(ts, sizeof(ts), "%Y-%m-%d %H:%M:%S", localtime(&now));
    fprintf(log, "[%s] user=%s action=%-10s file=%-20s result=%s\n",
            ts, currentUser[0] ? currentUser : "unknown",
            action, filename ? filename : "-", result);
    fclose(log);
}

void viewAuditLog() {
    FILE *log = fopen(AUDIT_LOG, "r");
    if (!log) { printf("No audit log yet.\n"); return; }
    char line[256];
    printf("\n----------------- AUDIT LOG -----------------\n");
    while (fgets(line, sizeof(line), log)) printf("%s", line);
    printf("----------------------------------------------\n");
    fclose(log);
}

/* ===================== Authentication ===================== */
int authenticate() {
    char uname[32], pass[64];
    for (int attempt = 1; attempt <= MAX_LOGIN_TRY; attempt++) {
        printf("Username: ");
        scanf("%31s", uname);
        printf("Password: ");
        scanf("%63s", pass);

        unsigned long h = hashPassword(pass);
        for (int i = 0; i < userCount; i++) {
            if (strcmp(users[i].username, uname) == 0 &&
                users[i].passHash == h) {
                strcpy(currentUser, uname);
                auditLog("LOGIN", NULL, "SUCCESS");
                printf("\nLogin successful. Welcome, %s!\n", uname);
                return 1;
            }
        }
        printf("Invalid credentials. Attempt %d of %d.\n\n",
               attempt, MAX_LOGIN_TRY);
    }
    strcpy(currentUser, uname);
    auditLog("LOGIN", NULL, "FAILED - locked out");
    printf("Too many failed attempts. Exiting for security reasons.\n");
    return 0;
}

/* ===================== File operations ===================== */
void createFile(const char *path) {
    int fd = open(path, O_CREAT | O_WRONLY | O_EXCL, 0600);
    if (fd == -1) {
        perror("create");
        auditLog("CREATE", path, "FAILED");
        return;
    }
    close(fd);
    printf("File '%s' created (default permission: owner read/write only).\n", path);
    auditLog("CREATE", path, "SUCCESS");
}

void writeFile(const char *path) {
    if (access(path, W_OK) != 0) {
        printf("Access denied: no write permission.\n");
        auditLog("WRITE", path, "DENIED");
        return;
    }
    char text[BUF_SIZE];
    printf("Enter text to append: ");
    getchar(); /* consume leftover newline */
    fgets(text, BUF_SIZE, stdin);

    int fd = open(path, O_WRONLY | O_APPEND);
    if (fd == -1) { perror("write"); auditLog("WRITE", path, "FAILED"); return; }
    write(fd, text, strlen(text));
    close(fd);
    printf("Data written successfully.\n");
    auditLog("WRITE", path, "SUCCESS");
}

void readFile(const char *path) {
    if (access(path, R_OK) != 0) {
        printf("Access denied: no read permission.\n");
        auditLog("READ", path, "DENIED");
        return;
    }
    int fd = open(path, O_RDONLY);
    if (fd == -1) { perror("read"); auditLog("READ", path, "FAILED"); return; }

    char buf[BUF_SIZE];
    ssize_t n;
    printf("----- %s -----\n", path);
    while ((n = read(fd, buf, BUF_SIZE - 1)) > 0) {
        buf[n] = '\0';
        printf("%s", buf);
    }
    printf("\n--------------------------\n");
    close(fd);
    auditLog("READ", path, "SUCCESS");
}

void deleteFile(const char *path) {
    if (unlink(path) == 0) {
        printf("File '%s' deleted.\n", path);
        auditLog("DELETE", path, "SUCCESS");
    } else {
        perror("delete");
        auditLog("DELETE", path, "FAILED");
    }
}

/* ===================== Permission management ===================== */
void modeToString(mode_t mode, char *out) {
    const char p[9] = {'r','w','x','r','w','x','r','w','x'};
    mode_t bits[9] = { S_IRUSR,S_IWUSR,S_IXUSR,S_IRGRP,S_IWGRP,S_IXGRP,
                       S_IROTH,S_IWOTH,S_IXOTH };
    for (int i = 0; i < 9; i++) out[i] = (mode & bits[i]) ? p[i] : '-';
    out[9] = '\0';
}

void setPermissions(const char *path) {
    struct stat st;
    if (stat(path, &st) != 0) { perror("stat"); return; }
    char cur[10]; modeToString(st.st_mode, cur);
    printf("Current permissions: %s\n", cur);

    int r_o,w_o,x_o, r_g,w_g,x_g, r_ot,w_ot,x_ot;
    printf("Enter owner permissions  (r w x as 1/0): ");
    scanf("%d %d %d", &r_o, &w_o, &x_o);
    printf("Enter group permissions  (r w x as 1/0): ");
    scanf("%d %d %d", &r_g, &w_g, &x_g);
    printf("Enter others permissions (r w x as 1/0): ");
    scanf("%d %d %d", &r_ot, &w_ot, &x_ot);

    mode_t mode = 0;
    if (r_o)  mode |= S_IRUSR;
    if (w_o)  mode |= S_IWUSR;
    if (x_o)  mode |= S_IXUSR;
    if (r_g)  mode |= S_IRGRP;
    if (w_g)  mode |= S_IWGRP;
    if (x_g)  mode |= S_IXGRP;
    if (r_ot) mode |= S_IROTH;
    if (w_ot) mode |= S_IWOTH;
    if (x_ot) mode |= S_IXOTH;

    if (chmod(path, mode) == 0) {
        modeToString(mode, cur);
        printf("Permissions updated to: %s (octal %o)\n", cur, mode);
        auditLog("CHMOD", path, "SUCCESS");
    } else {
        perror("chmod");
        auditLog("CHMOD", path, "FAILED");
    }
}

/* ===================== XOR encryption / decryption ===================== */
void xorProcessFile(const char *inPath, const char *outPath, const char *key) {
    FILE *in = fopen(inPath, "rb");
    if (!in) { perror("fopen (input)"); auditLog("ENCRYPT/DECRYPT", inPath, "FAILED"); return; }
    FILE *out = fopen(outPath, "wb");
    if (!out) { perror("fopen (output)"); fclose(in); return; }

    int klen = strlen(key);
    int c, i = 0;
    while ((c = fgetc(in)) != EOF) {
        fputc(c ^ key[i % klen], out);
        i++;
    }
    fclose(in);
    fclose(out);
}

void encryptFile(const char *path) {
    if (access(path, R_OK) != 0) {
        printf("Access denied: cannot read file to encrypt.\n");
        return;
    }
    char key[64];
    printf("Enter encryption key: ");
    scanf("%63s", key);
    char outPath[300];
    snprintf(outPath, sizeof(outPath), "%s.enc", path);
    xorProcessFile(path, outPath, key);
    printf("Encrypted file written to '%s'\n", outPath);
    auditLog("ENCRYPT", path, "SUCCESS");
}

void decryptFile(const char *path) {
    if (access(path, R_OK) != 0) {
        printf("Access denied: cannot read encrypted file.\n");
        return;
    }
    char key[64];
    printf("Enter decryption key: ");
    scanf("%63s", key);
    char outPath[300];
    snprintf(outPath, sizeof(outPath), "%s.dec", path);
    xorProcessFile(path, outPath, key);
    printf("Decrypted file written to '%s'\n", outPath);
    auditLog("DECRYPT", path, "SUCCESS");
}

/* ===================== Menu ===================== */
void printMenu() {
    printf("\n========== SECURE FILE MANAGER ==========\n");
    printf("1. Create file\n");
    printf("2. Write to file\n");
    printf("3. Read file\n");
    printf("4. Delete file\n");
    printf("5. Set file permissions\n");
    printf("6. Encrypt file\n");
    printf("7. Decrypt file\n");
    printf("8. View audit log\n");
    printf("9. Logout / Exit\n");
    printf("==========================================\n");
    printf("Choice: ");
}

int main() {
    /* demo users - in a real system these would come from a secured,
       salted credential store, not source code */

    addUser("bhawana", "Bhawana@123");

    printf("===== SECURE FILE MANAGEMENT SYSTEM =====\n");
    if (!authenticate()) return 1;

    int choice;
    char path[256];
    do {
        printMenu();
        scanf("%d", &choice);
        if (choice >= 1 && choice <= 7) {
            printf("Enter filename: ");
            scanf("%255s", path);
        }
        switch (choice) {
            case 1: createFile(path); break;
            case 2: writeFile(path); break;
            case 3: readFile(path); break;
            case 4: deleteFile(path); break;
            case 5: setPermissions(path); break;
            case 6: encryptFile(path); break;
            case 7: decryptFile(path); break;
            case 8: viewAuditLog(); break;
            case 9: printf("Logging out. Goodbye, %s.\n", currentUser);
                    auditLog("LOGOUT", NULL, "SUCCESS"); break;
            default: printf("Invalid choice.\n");
        }
    } while (choice != 9);

    return 0;
}

