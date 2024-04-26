#include "myutil.h"

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

void parseServerArgs(int argc, char *argv[], struct ServerArg *serverArg) {
    if (argc < 3) {
        fprintf(stderr, "Usage: %s <dirname> <max. #ofClients>\n", argv[0]);
        exit(EXIT_FAILURE);
    }

    // Check if serverArg is not NULL
    if (serverArg == NULL) {
        fprintf(stderr, "Error: serverArg is NULL\n");
        exit(EXIT_FAILURE);
    }

    strncpy(serverArg->filename, argv[1], sizeof(serverArg->filename) - 1);

    serverArg->numOfClients = atoi(argv[2]);
}

void parseClientArgs(int argc, char *argv[], struct ClientArg *clientArg) {
    if (argc < 3) {
        fprintf(stderr, "Usage: %s <Connect/tryConnect> <serverPid>\n", argv[0]);
        exit(EXIT_FAILURE);
    }

    // Check if clientArg is not NULL
    if (clientArg == NULL) {
        fprintf(stderr, "Error: clientArg is NULL\n");
        exit(EXIT_FAILURE);
    }

    strncpy(clientArg->connectionType, argv[1], sizeof(clientArg->connectionType) - 1);

    clientArg->serverPid = atoi(argv[2]);
}

enum ConnectionType getConnectionType(const char *connectionType) {
    if (strcmp(connectionType, "Connect") == 0) {
        return CONNECT;
    } else if (strcmp(connectionType, "tryConnect") == 0) {
        return TRYCONNECT;
    } else {
        fprintf(stderr, "Error: Invalid connection type\n");
        exit(EXIT_FAILURE);
    }
}