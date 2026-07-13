#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>
#include <time.h>
#include <stdarg.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include "protocol.h"

typedef struct {
    char username[MAX_USERNAME];
    unsigned long passHash;
} User;

typedef struct {
    int  sockfd;
    char username[MAX_USERNAME];
    int  active;
} Client;

static User   users[MAX_CLIENTS];
static int    userCount = 0;

static Client clients[MAX_CLIENTS];
static pthread_mutex_t clientsLock = PTHREAD_MUTEX_INITIALIZER;

/* ===================== Utility ===================== */
unsigned long hashPassword(const char *str) {
    unsigned long hash = 5381;
    int c;
    while ((c = *str++)) hash = ((hash << 5) + hash) + c;  /* djb2 */
    return hash;
}

void auditLog(const char *fmt, ...) {
    FILE *log = fopen(AUDIT_FILE, "a");
    if (!log) return;
    time_t now = time(NULL);
    char ts[32];
    strftime(ts, sizeof(ts), "%Y-%m-%d %H:%M:%S", localtime(&now));
    fprintf(log, "[%s] ", ts);

    va_list args;
    va_start(args, fmt);
    vfprintf(log, fmt, args);
    va_end(args);
    fprintf(log, "\n");
    fclose(log);
    /* also echo to console so behaviour is visible while testing */
    va_start(args, fmt);
    printf("[%s] ", ts);
    vprintf(fmt, args);
    printf("\n");
    va_end(args);
}

/* create a default credential file the first time the server runs */
void loadUsers() {
    FILE *f = fopen(USERS_FILE, "r");
    if (!f) {
        f = fopen(USERS_FILE, "w");
        fprintf(f, "bhawana:%lu\n",  hashPassword("Bhawana@123"));
        fprintf(f, "sita:%lu\n",  hashPassword("Sita@123"));
        fprintf(f, "gita:%lu\n",    hashPassword("Gita@123"));
        fprintf(f, "ram:%lu\n",  hashPassword("Ram@123"));
        fclose(f);
        f = fopen(USERS_FILE, "r");
    }
    char line[128];
    while (fgets(line, sizeof(line), f) && userCount < MAX_CLIENTS) {
        char *colon = strchr(line, ':');
        if (!colon) continue;                 /* malformed line: skip */
        *colon = '\0';
        strncpy(users[userCount].username, line, MAX_USERNAME - 1);
        users[userCount].passHash = strtoul(colon + 1, NULL, 10);
        userCount++;
    }
    fclose(f);
    printf("Loaded %d user account(s) from %s\n", userCount, USERS_FILE);
}

int authenticate(const char *username, const char *password) {
    unsigned long h = hashPassword(password);
    for (int i = 0; i < userCount; i++)
        if (strcmp(users[i].username, username) == 0 && users[i].passHash == h)
            return 1;
    return 0;
}

/* ===================== Data validation ===================== */
/* rejects empty/too-long fields and characters that would break the
   ':'-delimited, newline-terminated wire protocol */
int isValidField(const char *s, int maxLen) {
    int len = strlen(s);
    if (len == 0 || len >= maxLen) return 0;
    for (int i = 0; i < len; i++)
        if (s[i] == ':' || s[i] == '\n' || s[i] == '\r') return 0;
    return 1;
}

/* ===================== Client list management ===================== */
int addClient(int sockfd, const char *username) {
    pthread_mutex_lock(&clientsLock);
    for (int i = 0; i < MAX_CLIENTS; i++) {
        if (!clients[i].active) {
            clients[i].sockfd = sockfd;
            strncpy(clients[i].username, username, MAX_USERNAME - 1);
            clients[i].active = 1;
            pthread_mutex_unlock(&clientsLock);
            return i;
        }
    }
    pthread_mutex_unlock(&clientsLock);
    return -1;  /* server full */
}

void removeClient(int idx) {
    pthread_mutex_lock(&clientsLock);
    if (idx >= 0 && idx < MAX_CLIENTS) clients[idx].active = 0;
    pthread_mutex_unlock(&clientsLock);
}

void broadcast(const char *senderUser, const char *text, int excludeIdx) {
    char out[BUF_SIZE];
    snprintf(out, sizeof(out), "BCAST:%s:%s\n", senderUser, text);
    pthread_mutex_lock(&clientsLock);
    for (int i = 0; i < MAX_CLIENTS; i++) {
        if (clients[i].active && i != excludeIdx) {
            if (send(clients[i].sockfd, out, strlen(out), 0) == -1)
                perror("send (broadcast)");
        }
    }
    pthread_mutex_unlock(&clientsLock);
}
 
void sendLine(int sockfd, const char *line) {
    if (send(sockfd, line, strlen(line), 0) == -1) perror("send");
}
 
/* ===================== Per-client thread ===================== */
void *handleClient(void *arg) {
    int sockfd = *(int *)arg;
    free(arg);
    int clientIdx = -1;
    char username[MAX_USERNAME] = "";
    char buf[BUF_SIZE];
 
    /* ---- Step 1: authentication ---- */
    ssize_t n = recv(sockfd, buf, BUF_SIZE - 1, 0);
    if (n <= 0) { close(sockfd); return NULL; }
    buf[n] = '\0';
    buf[strcspn(buf, "\n")] = '\0';
 
    char type[16] = "", uname[MAX_USERNAME] = "", pass[MAX_PASSWORD] = "";
    sscanf(buf, "%15[^:]:%31[^:]:%63[^\n]", type, uname, pass);
 
    if (strcmp(type, "AUTH") != 0 || !isValidField(uname, MAX_USERNAME) ||
        !isValidField(pass, MAX_PASSWORD)) {
        sendLine(sockfd, "ERR:400:Malformed authentication request\n");
        auditLog("REJECTED malformed AUTH from socket %d", sockfd);
        close(sockfd);
        return NULL;
    }
 
    if (!authenticate(uname, pass)) {
        sendLine(sockfd, "AUTH_FAIL:invalid username or password\n");
        auditLog("LOGIN FAILED user=%s", uname);
        close(sockfd);
        return NULL;
    }
 
    strncpy(username, uname, MAX_USERNAME - 1);
    clientIdx = addClient(sockfd, username);
    if (clientIdx == -1) {
        sendLine(sockfd, "ERR:503:Server full, try again later\n");
        auditLog("REJECTED user=%s server full", username);
        close(sockfd);
        return NULL;
    }
 
    char ok[BUF_SIZE];
    snprintf(ok, sizeof(ok), "AUTH_OK:%s\n", username);
    sendLine(sockfd, ok);
    auditLog("LOGIN SUCCESS user=%s (client slot %d)", username, clientIdx);
 
    char joinMsg[BUF_SIZE];
    snprintf(joinMsg, sizeof(joinMsg), "%s has joined the chat", username);
    broadcast("SERVER", joinMsg, clientIdx);
 
    /* ---- Step 2: message loop ---- */
    while (1) {
        n = recv(sockfd, buf, BUF_SIZE - 1, 0);
        if (n == 0) {
            auditLog("DISCONNECT user=%s (connection closed)", username);
            break;
        }
        if (n < 0) {
            perror("recv");
            auditLog("ERROR recv failed for user=%s", username);
            break;
        }
        buf[n] = '\0';
        buf[strcspn(buf, "\n")] = '\0';
 
        if (strcmp(buf, "QUIT") == 0) {
            auditLog("QUIT user=%s", username);
            break;
        }
 
        char msgType[16] = "", msgUser[MAX_USERNAME] = "", text[BUF_SIZE] = "";
        int fields = sscanf(buf, "%15[^:]:%31[^:]:%1023[^\n]", msgType, msgUser, text);
 
        if (fields != 3 || strcmp(msgType, "MSG") != 0 ||
            !isValidField(text, BUF_SIZE)) {
            sendLine(sockfd, "ERR:422:Malformed or invalid message\n");
            auditLog("INVALID message from user=%s rejected", username);
            continue;
        }
 
        auditLog("MSG user=%s text=\"%s\"", username, text);
        broadcast(username, text, clientIdx);
    }
 
    /* ---- Step 3: cleanup ---- */
    removeClient(clientIdx);
    close(sockfd);
    char leaveMsg[BUF_SIZE];
    snprintf(leaveMsg, sizeof(leaveMsg), "%s has left the chat", username);
    broadcast("SERVER", leaveMsg, -1);
    return NULL;
}
 
int main() {
    loadUsers();
 
    int serverFd = socket(AF_INET, SOCK_STREAM, 0);
    if (serverFd == -1) { perror("socket"); exit(1); }
 
    int opt = 1;
    setsockopt(serverFd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));
 
    struct sockaddr_in addr;
    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = INADDR_ANY;
    addr.sin_port = htons(SERVER_PORT);
 
    if (bind(serverFd, (struct sockaddr *)&addr, sizeof(addr)) == -1) {
        perror("bind");
        exit(1);
    }
    if (listen(serverFd, MAX_CLIENTS) == -1) {
        perror("listen");
        exit(1);
    }
 
    printf("Chat server listening on port %d ...\n", SERVER_PORT);
    auditLog("SERVER STARTED on port %d", SERVER_PORT);
 
    while (1) {
        struct sockaddr_in clientAddr;
        socklen_t clientLen = sizeof(clientAddr);
        int clientFd = accept(serverFd, (struct sockaddr *)&clientAddr, &clientLen);
        if (clientFd == -1) { perror("accept"); continue; }
 
        char ip[INET_ADDRSTRLEN];
        inet_ntop(AF_INET, &clientAddr.sin_addr, ip, sizeof(ip));
        auditLog("CONNECTION from %s:%d (socket %d)", ip,
                  ntohs(clientAddr.sin_port), clientFd);
 
        int *fdPtr = malloc(sizeof(int));
        *fdPtr = clientFd;
 
        pthread_t tid;
        if (pthread_create(&tid, NULL, handleClient, fdPtr) != 0) {
            perror("pthread_create");
            close(clientFd);
            free(fdPtr);
            continue;
        }
        pthread_detach(tid);   /* thread cleans up its own resources on exit */
    }
 
    close(serverFd);
    return 0;
}
