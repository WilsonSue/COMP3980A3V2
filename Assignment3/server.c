#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <string.h>
#include <sys/wait.h>  // For wait()

#define MAX_ARGS 10
#define MAX_PATH_LENGTH 256
#define MAX_DIRECTORIES 256

static char** parseCommand(const char *command_with_args);
void executeCommand(char *args[], char *directories[]);

int main(int argc, char *argv[]) {
    if (argc != 3 || strcmp(argv[1], "-fifo") != 0) {
        fprintf(stderr, "Usage: server -fifo <path>\n");
        return EXIT_FAILURE;
    }

    char *CMD_FIFO = argv[2];
    char OUT_FIFO[256];
    snprintf(OUT_FIFO, sizeof(OUT_FIFO), "%s_out", CMD_FIFO);

    // Create the FIFOs
    if (mkfifo(CMD_FIFO, 0666) == -1) {
        perror("Error creating CMD_FIFO");
        return EXIT_FAILURE;
    }

    if (mkfifo(OUT_FIFO, 0666) == -1) {
        perror("Error creating OUT_FIFO");
        return EXIT_FAILURE;
    }

    int cmd_fd = open(CMD_FIFO, O_RDONLY);
    if (cmd_fd == -1) {
        perror("Error opening CMD_FIFO");
        exit(EXIT_FAILURE);
    }

    int out_fd = open(OUT_FIFO, O_WRONLY);
    if (out_fd == -1) {
        perror("Error opening OUT_FIFO");
        exit(EXIT_FAILURE);
    }

    char command[256];
    int bytesRead = read(cmd_fd, command, sizeof(command) - 1);
    if (bytesRead <= 0) {
        perror("Failed to read command");
        exit(EXIT_FAILURE);
    }
    command[bytesRead] = '\0';

    // Parse the command
    char **args = parseCommand(command);
    if (!args) {
        fprintf(stderr, "Error parsing command\n");
        exit(EXIT_FAILURE);
    }

    // Redirect stdout to OUT_FIFO
    dup2(out_fd, STDOUT_FILENO);

    pid_t pid = fork();
    if (pid < 0) {
        perror("Error forking");
        exit(EXIT_FAILURE);
    } else if (pid == 0) { // Child process
        char *path = getenv("PATH");
        if (!path) {
            fprintf(stderr, "PATH environment variable not found\n");
            exit(EXIT_FAILURE);
        }

        char *path_copy = strdup(path);
        if (!path_copy) {
            perror("Error duplicating PATH string");
            exit(EXIT_FAILURE);
        }

        char *dir = strtok(path_copy, ":");
        char *search_directories[MAX_DIRECTORIES];
        int directory_count = 0;

        while (dir != NULL && directory_count < MAX_DIRECTORIES - 1) {
            search_directories[directory_count++] = dir;
            dir = strtok(NULL, ":");
        }
        search_directories[directory_count] = NULL;

        executeCommand(args, search_directories);
        free(path_copy);
    } else { // Parent process
        wait(NULL);  // Wait for the child process to finish
    }

    close(cmd_fd);
    close(out_fd);
    unlink(CMD_FIFO);
    unlink(OUT_FIFO);

    return 0;
}

char** parseCommand(const char *command_with_args) {
    static char *args[MAX_ARGS];
    int arg_count = 0;
    char *command_with_args_copy = strdup(command_with_args);

    if (!command_with_args_copy) {
        perror("strdup");
        return NULL;
    }

    char *token = strtok(command_with_args_copy, " ");
    while (token != NULL && arg_count < MAX_ARGS) {
        args[arg_count++] = token;
        token = strtok(NULL, " ");
    }
    args[arg_count] = NULL;

    return args;
}

void executeCommand(char *args[], char *directories[]) {
    if (!args || !args[0]) {
        fprintf(stderr, "Invalid arguments provided.\n");
        exit(EXIT_FAILURE);
    }

    for (int i = 0; directories[i] != NULL; i++) {
        char full_path[MAX_PATH_LENGTH];
        snprintf(full_path, sizeof(full_path), "%s/%s", directories[i], args[0]);

        if (access(full_path, X_OK) == 0) {
            execv(full_path, args);
        }
    }

    fprintf(stderr, "Error: %s not found in specified directories\n", args[0]);
    exit(EXIT_FAILURE);
}
