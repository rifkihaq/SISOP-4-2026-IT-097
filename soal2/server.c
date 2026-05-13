#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <dirent.h>

#define PORT 9000
#define BUFFER_SIZE 1024
#define DB_PATH "./db"

void send_response(int client_sock, const char *message) {
    send(client_sock, message, strlen(message), 0);
}

void create_database(int client_sock, char *dbname) {
    char path[256];

    snprintf(path, sizeof(path), "%s/%s", DB_PATH, dbname);

    if (mkdir(path, 0777) == 0) {
        send_response(client_sock, "DATABASE CREATED\n");
    } else {
        send_response(client_sock, "FAILED CREATE DATABASE\n");
    }
}

void create_table(int client_sock, char *dbname, char *tablename) {
    char path[256];

    snprintf(path, sizeof(path), "%s/%s/%s.csv", DB_PATH, dbname, tablename);

    FILE *fp = fopen(path, "w");

    if (fp) {
        fclose(fp);
        send_response(client_sock, "TABLE CREATED\n");
    } else {
        send_response(client_sock, "FAILED CREATE TABLE\n");
    }
}

void list_database(int client_sock) {
    DIR *dir;
    struct dirent *entry;
    char result[4096] = "";

    dir = opendir(DB_PATH);

    if (!dir) {
        send_response(client_sock, "FAILED OPEN DB\n");
        return;
    }

    while ((entry = readdir(dir)) != NULL) {
        if (entry->d_type == DT_DIR &&
            strcmp(entry->d_name, ".") != 0 &&
            strcmp(entry->d_name, "..") != 0) {

            strcat(result, entry->d_name);
            strcat(result, "\n");
        }
    }

    closedir(dir);

    send_response(client_sock, result);
}

void list_table(int client_sock, char *dbname) {
    char path[256];

    snprintf(path, sizeof(path), "%s/%s", DB_PATH, dbname);

    DIR *dir;
    struct dirent *entry;
    char result[4096] = "";

    dir = opendir(path);

    if (!dir) {
        send_response(client_sock, "DATABASE NOT FOUND\n");
        return;
    }

    while ((entry = readdir(dir)) != NULL) {
        if (entry->d_type == DT_REG) {
            strcat(result, entry->d_name);
            strcat(result, "\n");
        }
    }

    closedir(dir);

    send_response(client_sock, result);
}

int main() {
    int server_fd, client_sock;
    struct sockaddr_in server_addr, client_addr;
    socklen_t client_len = sizeof(client_addr);

    char buffer[BUFFER_SIZE];

    mkdir(DB_PATH, 0777);

    server_fd = socket(AF_INET, SOCK_STREAM, 0);

    if (server_fd < 0) {
        perror("Socket failed");
        exit(EXIT_FAILURE);
    }

    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(PORT);
    server_addr.sin_addr.s_addr = INADDR_ANY;

    if (bind(server_fd,
             (struct sockaddr *)&server_addr,
             sizeof(server_addr)) < 0) {

        perror("Bind failed");
        exit(EXIT_FAILURE);
    }

    listen(server_fd, 5);

    printf("Server listening on port %d...\n", PORT);

    while (1) {

        client_sock = accept(server_fd,
                             (struct sockaddr *)&client_addr,
                             &client_len);

        if (client_sock < 0) {
            perror("Accept failed");
            continue;
        }

        printf("Client connected\n");

        while (1) {

            memset(buffer, 0, BUFFER_SIZE);

            int bytes = recv(client_sock, buffer,
                             BUFFER_SIZE, 0);

            if (bytes <= 0)
                break;

            printf("Command: %s\n", buffer);

            char *cmd = strtok(buffer, " \n");

            if (!cmd)
                continue;

            if (strcmp(cmd, "CREATE") == 0) {

                char *type = strtok(NULL, " \n");

                if (!type)
                    continue;

                if (strcmp(type, "DATABASE") == 0) {

                    char *dbname = strtok(NULL, " \n");

                    if (dbname)
                        create_database(client_sock, dbname);

                } else if (strcmp(type, "TABLE") == 0) {

                    char *dbname = strtok(NULL, " \n");
                    char *tablename = strtok(NULL, " \n");

                    if (dbname && tablename)
                        create_table(client_sock,
                                     dbname,
                                     tablename);
                }

            } else if (strcmp(cmd, "LIST") == 0) {

                char *type = strtok(NULL, " \n");

                if (!type)
                    continue;

                if (strcmp(type, "DATABASE") == 0) {

                    list_database(client_sock);

                } else if (strcmp(type, "TABLE") == 0) {

                    char *dbname = strtok(NULL, " \n");

                    if (dbname)
                        list_table(client_sock, dbname);
                }

            } else {

                send_response(client_sock,
                              "UNKNOWN COMMAND\n");
            }
        }

        close(client_sock);

        printf("Client disconnected\n");
    }

    close(server_fd);

    return 0;
}
