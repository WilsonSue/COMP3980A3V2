#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <string.h>

int main(int argc, char *argv[]) {
    if (argc != 4 || strcmp(argv[1], "-fifo") != 0) {
        write(STDERR_FILENO, "Usage: client -fifo <path> \"command -options\"\n", strlen("Usage: client -fifo <path> \"command -options\"\n"));
        return EXIT_FAILURE;
    }

    char *CMD_FIFO = argv[2];
    char OUT_FIFO[256];
    snprintf(OUT_FIFO, sizeof(OUT_FIFO), "%s_out", CMD_FIFO);

    int cmd_fd = open(CMD_FIFO, O_WRONLY);
    if (cmd_fd == -1) {
        perror("Error opening CMD_FIFO for writing");
        exit(EXIT_FAILURE);
    }

    ssize_t bytesWritten = write(cmd_fd, argv[3], strlen(argv[3]));
    if (bytesWritten < 0) {
        perror("Error writing to CMD_FIFO");
        close(cmd_fd);
        exit(EXIT_FAILURE);
    }
    close(cmd_fd);

    int out_fd = open(OUT_FIFO, O_RDONLY);
    if (out_fd == -1) {
        perror("Error opening OUT_FIFO for reading");
        exit(EXIT_FAILURE);
    }

    char buffer[1024];
    int bytesRead;
    while ((bytesRead = read(out_fd, buffer, sizeof(buffer))) > 0) {
        ssize_t result = write(STDOUT_FILENO, buffer, bytesRead);
        if (result < 0) {
            perror("Error writing to STDOUT");
            close(out_fd);
            exit(EXIT_FAILURE);
        }
    }

    if (bytesRead < 0) {
        perror("Error reading from OUT_FIFO");
    }

    close(out_fd);
    return 0;
}
