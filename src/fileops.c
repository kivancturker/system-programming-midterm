#include "fileops.h"
#include "myutil.h"

#include <stdio.h>
#include <stdlib.h>
#include <dirent.h>
#include <string.h>
#include <semaphore.h>
#include <fcntl.h>

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
        if (strcmp(entry->d_name, ".") != 0 && strcmp(entry->d_name, "..") != 0) {
            numOfFiles++;
        }
        /* // Old code
        if (entry->d_type == DT_REG) {
            numOfFiles++;
        }
        */
    }

    closedir(dir);

    return numOfFiles;
}

int isFileExists(const char* dirName, const char* filename) {
    int numOfFiles = getNumOfFilesInDir(dirName);
    char* filenames[numOfFiles];
    getAllTheFilenamesInDir(dirName, filenames, numOfFiles);
    int fileExists = 0;
    for (int i = 0; i < numOfFiles; i++) {
        if (strcmp(filenames[i], filename) == 0) {
            fileExists = 1;
        }
        free(filenames[i]);
    }
    return fileExists;
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
        free(filenames[i]);
    }
}

void createSemaphoreForGivenFile(const char* dirName, const char* filename) {
    char semaphoreName[MAX_SEMAPHORE_NAME_SIZE];
    getSemaphoreNameByFilename(filename, getpid(), semaphoreName);
    sem_t* semaphore = sem_open(semaphoreName, O_CREAT, 0666, 1);
    if (semaphore == SEM_FAILED) {
        perror("sem_open");
        exit(EXIT_FAILURE);
    }
    sem_close(semaphore);
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
        free(filenames[i]);
    }
}

char* readLineFromFile(const char* dirName, const char* filename, int lineNum) {
    if (lineNum < 1) {
        return NULL;
    }
    char* line = NULL;
    char filePath[MAX_FILENAME_SIZE];
    snprintf(filePath, MAX_FILENAME_SIZE, "%s/%s", dirName, filename);
    FILE* file = fopen(filePath, "r");
    if (file == NULL) {
        return NULL;
    }

    size_t len = 0;
    ssize_t read;
    int currentLine = 1;
    while ((read = getline(&line, &len, file)) != -1) {
        if (currentLine == lineNum) {
            fclose(file);
            return line;
        }
        currentLine++;
        free(line);
        line = NULL;
    }

    fclose(file);
    return NULL;
}

char* readWholeFile(const char* dirName, const char* filename) {
    char* content = NULL; // String to hold the file content
    char filePath[MAX_FILENAME_SIZE];
    sprintf(filePath, "%s/%s", dirName, filename);
    FILE* file = fopen(filePath, "r");
    if (file == NULL) {
        return NULL;
    }

    // Allocate initial memory for the content string
    size_t capacity = 1024; // Initial capacity
    content = (char*)malloc(capacity);
    if (content == NULL) {
        fclose(file);
        return NULL;
    }
    content[0] = '\0'; // Initialize the string

    char* line = NULL;
    size_t len = 0;
    ssize_t read;
    while ((read = getline(&line, &len, file)) != -1) {
        // Resize the content string if needed
        if (strlen(content) + read >= capacity) {
            capacity *= 2; // Double the capacity
            char* temp = realloc(content, capacity);
            if (temp == NULL) {
                free(content);
                fclose(file);
                return NULL;
            }
            content = temp;
        }
        // Append the line to the content string
        strcat(content, line);
    }
    free(line); // Free the memory allocated by getline

    fclose(file);
    return content;
}

// If lineNum -1 then append the line at the end for the file
void writeLineToFile(const char* dirName, const char* filename, const char* lineToInsert, int lineNum) {
    if (lineToInsert == NULL) {
        return;
    }
    char* tempFilename = "tmpX8yQ";
    char filePath[MAX_FILENAME_SIZE];
    char tempFilePath[MAX_FILENAME_SIZE];
    snprintf(filePath, MAX_FILENAME_SIZE, "%s/%s", dirName, filename);
    snprintf(tempFilePath, MAX_FILENAME_SIZE, "%s/%s", dirName, tempFilename);
    FILE* file = fopen(filePath, "r");
    FILE* tempFile = fopen(tempFilePath, "w");
    if (file == NULL || tempFile == NULL) {
        perror("writeLineToFile fopen");
        return;
    }

    char* line = NULL;
    size_t len = 0;
    ssize_t read;
    int currentLine = 1;
    int appended = 0; // Flag to check if line is appended
    while ((read = getline(&line, &len, file)) != -1) {
        if (lineNum == -1 && read == 1 && line[0] == '\n') {
            // Skip empty lines at the end of the file
            free(line);
            line = NULL;
            continue;
        }
        if (currentLine == lineNum) {
            fprintf(tempFile, "%s\n", lineToInsert);
            appended = 1;
        }
        fprintf(tempFile, "%s", line);
        currentLine++;
        free(line);
        line = NULL;
    }
    if (line != NULL) {
        free(line);
        line = NULL;
    }

    // Append the line at the end if lineNum is -1 and not already appended
    char* prevLine = NULL;
    if (lineNum == -1 && !appended) {
        // If the previous line doesn't have a newline character, add one
        prevLine = readLineFromFile(dirName, filename, currentLine - 1);
        if (prevLine != NULL) {
            if (prevLine[strlen(prevLine) - 1] != '\n') {
                fprintf(tempFile, "\n");
            }
        }
        fprintf(tempFile, "%s\n", lineToInsert);
        if (prevLine != NULL) {
            free(prevLine);
        }
    }

    fclose(file);
    fclose(tempFile);
    remove(filePath);
    rename(tempFilePath, filePath);
}