#ifndef FILEOPS_H
#define FILEOPS_H

#include "mytypes.h"

void getAllTheFilenamesInDir(const char* dirName, char* filenames[], int size);
int getNumOfFilesInDir(const char* dirName);
int isFileExists(const char* dirName, const char* filename);
void createSemaphores(const char* dirName);
void createSemaphoreForGivenFile(const char* dirName, const char* filename);
void destroyAllSemaphores(const char* dirName);


char* readLineFromFile(const char* dirName, const char* filename, int lineNum);
char* readWholeFile(const char* dirName, const char* filename);
void writeLineToFile(const char* dirName, const char* filename, const char* lineToInsert, int lineNum);

#endif // FILEOPS_H