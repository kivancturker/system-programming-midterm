#ifndef MYUTIL_H
#define MYUTIL_H

#include <stdlib.h>
#include <errno.h>
#include "mytypes.h"

#define NO_EINTR(expr) while ((expr) == -1 && errno == EINTR);
#define READ_END_PIPE 0
#define WRITE_END_PIPE 1

void parseServerArgs(int argc, char *argv[], struct ServerArg *serverArg);
void parseClientArgs(int argc, char *argv[], struct ClientArg *clientArg);
enum CommandType getConnectionType(const char *connectionType);
void createDirIfNotExist(const char *dirName);
void errExit(const char* errMessage);
void initConnectionInfos(struct ConnectionInfo connectionInfos[], int numOfClients);
void closeAllPipesInConnectionInfos(struct ConnectionInfo connectionInfos[], int numOfClients);
void removeConnection(struct ConnectionInfo connectionInfos[], int numOfClients, int childNum);
void addNewConnection(struct ConnectionInfo connectionInfos[], int numOfClients, int childNum, pid_t clientPid);
int findAvailableConnection(struct ConnectionInfo connectionInfos[], int numOfClients);
int findConnectionIndexByClientPid(struct ConnectionInfo connectionInfos[], int numOfClients, pid_t clientPid);
int findConnectionIndexByChildPid(struct ConnectionInfo connectionInfos[], int numOfClients, pid_t childPid);
enum CommandType getCommandTypeFromCommandString(const char* command);

#endif // MYUTIL_H