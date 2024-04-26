#ifndef MYUTIL_H
#define MYUTIL_H

#include <stdlib.h>

#define MAX_FILENAME_SIZE 256

struct ServerArg{
    char filename[MAX_FILENAME_SIZE];
    int numOfClients;
};

struct ClientArg {
    char connectionType[MAX_FILENAME_SIZE];
    pid_t serverPid;
};

enum ConnectionType {
    CONNECT,
    TRYCONNECT
};

void parseServerArgs(int argc, char *argv[], struct ServerArg *serverArg);
void parseClientArgs(int argc, char *argv[], struct ClientArg *clientArg);
enum ConnectionType getConnectionType(const char *connectionType);

#endif // MYUTIL_H