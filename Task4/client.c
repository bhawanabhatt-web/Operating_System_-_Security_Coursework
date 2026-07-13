#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include "protocol.h"

static int sockfd;
static volatile int running = 1;

/* background thread: print whatever the server sends until it stops */
void *receiveLoop(void *arg) {
    (void)arg;
    char buf[BUF_SIZE];
    while (running) {
        ssize_t n = recv(sockfd, buf, BUF_SIZE - 1, 0);
        if (n <= 0) {
            if (running) printf("\n[Disconnected from server]\n");
            running = 0;
            break;
        }
        buf[n] = '\0';

        char type[16] = "", user[MAX_USERNAME] = "", text[BUF_SIZE] = "";
        if (sscanf(buf, "%15[^:]:%31[^:]:%1023[^\n]", type, user, text) == 3
            && strcmp(type, "BCAST") == 0) {
            printf("\n[%s]: %s\n> ", user, text);
            fflush(stdout);
        } else if (strncmp(buf, "ERR:", 4) == 0) {
            printf("\n[Server error] %s", buf + 4);
        } else {
            printf("\n%s", buf);
        }
    }
    return NULL;
}
 
int main(int argc, char *argv[]) {
    const char *ip   = (argc > 1) ? argv[1] : "127.0.0.1";
    int port          = (argc > 2) ? atoi(argv[2]) : SERVER_PORT;
 
    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd == -1) { perror("socket"); return 1; }
 
    struct sockaddr_in serverAddr;
    memset(&serverAddr, 0, sizeof(serverAddr));
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_port = htons(port);
    if (inet_pton(AF_INET, ip, &serverAddr.sin_addr) <= 0) {
        fprintf(stderr, "Invalid server address: %s\n", ip);
        return 1;
    }
 
    if (connect(sockfd, (struct sockaddr *)&serverAddr, sizeof(serverAddr)) == -1) {
        perror("connect");
        return 1;
    }
    printf("Connected to chat server at %s:%d\n", ip, port);
 
    char username[MAX_USERNAME], password[MAX_PASSWORD];
    printf("Username: ");
    if (!fgets(username, sizeof(username), stdin)) return 1;
    username[strcspn(username, "\n")] = '\0';
    printf("Password: ");
    if (!fgets(password, sizeof(password), stdin)) return 1;
    password[strcspn(password, "\n")] = '\0';
 
    char authMsg[BUF_SIZE];
    snprintf(authMsg, sizeof(authMsg), "AUTH:%s:%s\n", username, password);
    if (send(sockfd, authMsg, strlen(authMsg), 0) == -1) {
        perror("send (auth)"); close(sockfd); return 1;
    }
 
    char resp[BUF_SIZE];
    ssize_t n = recv(sockfd, resp, BUF_SIZE - 1, 0);
    if (n <= 0) { printf("Server closed the connection.\n"); close(sockfd); return 1; }
    resp[n] = '\0';
 
    if (strncmp(resp, "AUTH_OK", 7) != 0) {
        printf("Login failed: %s", resp);
        close(sockfd);
        return 1;
    }
    printf("Login successful! Type a message and press Enter to chat.\n");
    printf("Type /quit to disconnect.\n> ");
    fflush(stdout);
 
    pthread_t recvThread;
    pthread_create(&recvThread, NULL, receiveLoop, NULL);
 
    char line[BUF_SIZE - MAX_USERNAME - 16];
    while (running && fgets(line, sizeof(line), stdin)) {
        line[strcspn(line, "\n")] = '\0';
        if (strlen(line) == 0) { printf("> "); fflush(stdout); continue; }
 
        if (strcmp(line, "/quit") == 0) {
            send(sockfd, "QUIT\n", 5, 0);
            break;
        }
 
        char out[BUF_SIZE];
        snprintf(out, sizeof(out), "MSG:%s:%s\n", username, line);
        if (send(sockfd, out, strlen(out), 0) == -1) {
            perror("send (message)");
            break;
        }
        printf("> ");
        fflush(stdout);
    }
 
    /* stdin reached EOF (useful for scripted testing): give the
       receive thread a short window to print any late broadcasts
       from other concurrently connected clients before quitting */
    if (running) sleep(2);
 
    running = 0;
    shutdown(sockfd, SHUT_RDWR);
    close(sockfd);
    pthread_join(recvThread, NULL);
    printf("Disconnected.\n");
    return 0;
}
