#ifndef FILEOPS_H
#define FILEOPS_H

#include "mytypes.h"

void getAllTheFilenamesInDir(const char* dirName, char* filenames[], int size);
int getNumOfFilesInDir(const char* dirName);
int isFileExists(const char* dirName, const char* filename);
void createSemaphores(const char* dirName);
void destroyAllSemaphores(const char* dirName);


char* readLineFromFile(const char* dirName, const char* filename, int lineNum);

#endif // FILEOPS_H