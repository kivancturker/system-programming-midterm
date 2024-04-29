#ifndef MYUTIL_H
#define MYUTIL_H

#include <stdlib.h>
#include <errno.h>
#include "mytypes.h"

#define NO_EINTR(expr) while ((expr) == -1 && errno == EINTR);

void parseServerArgs(int argc, char *argv[], struct ServerArg *serverArg);
void parseClientArgs(int argc, char *argv[], struct ClientArg *clientArg);
enum CommandType getConnectionType(const char *connectionType);
void createDirIfNotExist(const char *dirName);
void errExit(const char* errMessage);

#endif // MYUTIL_H