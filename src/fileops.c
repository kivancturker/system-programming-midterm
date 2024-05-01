#include "fileops.h"
#include "myutil.h"

#include <stdio.h>
#include <stdlib.h>
#include <dirent.h>
#include <string.h>
#include <semaphore.h>

void getAllTheFilenamesInDir(const char* dirName, char* filenames[], int size) {
    DIR *dir;
    struct dirent *entry;
    int count = 0;

    // Open the directory
    if ((dir = opendir(dirName)) == NULL) {
        perror("opendir");
        exit(EXIT_FAILURE);
    }

    // Read each entry in the directory
    while ((entry = readdir(dir)) != NULL) {
        // Skip "." and ".." entries
        if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0) {
            continue;
        }
        // Store filename in the array if there's space
        if (count < size) {
            filenames[count] = strdup(entry->d_name); // Duplicate the filename
            count++;
        } else {
            fprintf(stderr, "Not enough space to store all filenames\n");
            break;
        }
    }

    closedir(dir);
}

int getNumOfFilesInDir(const char* dirName) {
    int numOfFiles = 0;
    DIR *dir;
    struct dirent *entry;

    if ((dir = opendir(dirName)) == NULL) {
        perror("opendir");
        exit(EXIT_FAILURE);
    }

    while ((entry = readdir(dir)) != NULL) {
        if (entry->d_type == DT_REG) {
            numOfFiles++;
        }
    }

    closedir(dir);

    return numOfFiles;
}

int isFileExists(const char* dirName, const char* filename) {
    int numOfFiles = getNumOfFilesInDir(dirName);
    char* filenames[numOfFiles];
    getAllTheFilenamesInDir(dirName, filenames, numOfFiles);
    for (int i = 0; i < numOfFiles; i++) {
        if (strcmp(filenames[i], filename) == 0) {
            return 1;
        }
    }
    return 0;
}

void createSemaphores(const char* dirName) {
    int numOfFiles = getNumOfFilesInDir(dirName);
    char* filenames[numOfFiles];
    getAllTheFilenamesInDir(dirName, filenames, numOfFiles);
    // create a semaphore for each file in the directory
    for (int i = 0; i < numOfFiles; i++) {
        char semaphoreName[MAX_SEMAPHORE_NAME_SIZE];
        getSemaphoreNameByFilename(filenames[i], getpid(), semaphoreName);
        sem_t* semaphore = sem_open(semaphoreName, O_CREAT, 0666, 1);
        if (semaphore == SEM_FAILED) {
            perror("sem_open");
            exit(EXIT_FAILURE);
        }
        sem_close(semaphore);
    }
}

void destroyAllSemaphores(const char* dirName) {
    int numOfFiles = getNumOfFilesInDir(dirName);
    char* filenames[numOfFiles];
    getAllTheFilenamesInDir(dirName, filenames, numOfFiles);
    // destroy a semaphore for each file in the directory
    for (int i = 0; i < numOfFiles; i++) {
        char semaphoreName[MAX_SEMAPHORE_NAME_SIZE];
        getSemaphoreNameByFilename(filenames[i], getpid(), semaphoreName);
        sem_unlink(semaphoreName);
    }
}

char* readLineFromFile(const char* dirName, const char* filename, int lineNum) {
    char* line = NULL;
    char filePath[MAX_FILENAME_SIZE];
    sprintf(filePath, "%s/%s", dirName, filename);
    FILE* file = fopen(filePath, "r");
    if (file == NULL) {
        return NULL;
    }

    size_t len = 0;
    ssize_t read;
    int currentLine = 1;
    while ((read = getline(&line, &len, file)) != -1) {
        if (currentLine == lineNum) {
            break;
        }
        currentLine++;
    }

    fclose(file);
    return line;
}