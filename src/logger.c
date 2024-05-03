#include "logger.h"

#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdlib.h>

void createLogFile() {
    int fd = open(LOG_FILE, O_WRONLY | O_CREAT | O_TRUNC, 0666);
    if (fd == -1) {
        perror("Error opening log file");
        exit(EXIT_FAILURE);
    }
    
    if (close(fd) == -1) {
        perror("Error closing log file");
        exit(EXIT_FAILURE);
    }
}

void writeLog(char *message) {
    FILE *file = fopen(LOG_FILE, "a");
    if (file == NULL) {
        perror("Error opening log file");
        exit(EXIT_FAILURE);
    }
    
    if (fprintf(file, "%s", message) < 0) {
        perror("Error writing to log file");
        exit(EXIT_FAILURE);
    }
    
    if (fclose(file) != 0) {
        perror("Error closing log file");
        exit(EXIT_FAILURE);
    }
}