#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include "models.h"
//#define PORT 8080
#define BUFFER_SIZE 1024

int main(int argc, char *argv[]) {
    int PORT = atoi(argv[1]);
    int sock;
    struct sockaddr_in serv_addr;
    char buffer[BUFFER_SIZE];
    sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock < 0) {
        perror("Socket creation failed");
        exit(EXIT_FAILURE);
    }
    memset(&serv_addr, 0, sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(PORT);
    serv_addr.sin_addr.s_addr = inet_addr("127.0.0.1");

    if (connect(sock, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0) {
        perror("Connection failed");
        close(sock);
        exit(EXIT_FAILURE);
    }
    //Auth flow
    //read welcome message
    read(sock, buffer, BUFFER_SIZE);
    printf("%s", buffer);
    memset(buffer, 0, sizeof(buffer));
    //send user_id
    int user_id;
    scanf("%d", &user_id);
    write(sock, &user_id, sizeof(user_id));
    //read password prompt
    read(sock, buffer, BUFFER_SIZE);
    printf("%s", buffer);
    memset(buffer, 0, sizeof(buffer));
    //send password
    char password[200];
    scanf(" %[^\n]", password);
    write(sock, password, strlen(password));
    //read auth status
    read(sock, buffer, BUFFER_SIZE);
    printf("%s", buffer);
    if (strstr(buffer, "Error") != NULL) {
        close(sock);
        exit(EXIT_FAILURE);
    }
    while (1) {
        printf("CLIENT_READING\n");
        memset(&buffer, 0, sizeof(buffer));
        // Read the message structure
        ssize_t bytes_read = read(sock, &buffer, sizeof(buffer));
        if (bytes_read < 0) {
            perror("Read failed");
            break; // Exit the loop on read failure
        }
        printf("%s", buffer);
        memset(&buffer, 0, sizeof(buffer));
        printf("CLIENT_WRITING\n");
        scanf(" %[^\n]", buffer);
        write(sock, &buffer, sizeof(buffer));
    }

    // Close socket
    close(sock);

    return 0;
}
