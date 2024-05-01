#include "fileops.h"

#include <stdio.h>
#include <stdlib.h>
#include <dirent.h>
#include <string.h>

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