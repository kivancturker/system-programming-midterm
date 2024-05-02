#include "myutil.h"
#include "command_handler.h"

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

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

    strncpy(serverArg->dirname, argv[1], sizeof(serverArg->dirname) - 1);

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

enum CommandType getConnectionType(const char *connectionType) {
    if (strcmp(connectionType, "Connect") == 0) {
        return CONNECT;
    } else if (strcmp(connectionType, "tryConnect") == 0) {
        return TRYCONNECT;
    } else {
        fprintf(stderr, "Error: Invalid connection type\n");
        fprintf(stderr, "Usage: Connect/tryConnect\n");
        exit(EXIT_FAILURE);
    }
}

void createDirIfNotExist(const char *dirName) {
    struct stat st;
    if (stat(dirName, &st) == -1) {
        // Directory does not exist, create it
        if (mkdir(dirName, 0777) == -1) {
            perror("mkdir");
            exit(EXIT_FAILURE);
        }
    }
}

void errExit(const char* errMessage) {
    perror(errMessage);
    exit(EXIT_FAILURE);
}

void initConnectionInfos(struct ConnectionInfo connectionInfos[], int numOfClients) {
    for (int i = 0; i < numOfClients; i++) {
        connectionInfos[i].clientPid = -1;
        connectionInfos[i].childNum = i;
    }
}

void closeAllPipesInConnectionInfos(struct ConnectionInfo connectionInfos[], int numOfClients) {
    for (int i = 0; i < numOfClients; i++) {
        if (connectionInfos[i].pipeFds[0] != -1) {
            if (close(connectionInfos[i].pipeFds[0]) == -1) {
                errExit("close pipeFds[0]");
            }
        }
        if (connectionInfos[i].pipeFds[1] != -1) {
            if (close(connectionInfos[i].pipeFds[1]) == -1) {
                errExit("close pipeFds[1]");
            }
        }
    }
}

void removeConnection(struct ConnectionInfo connectionInfos[], int numOfClients, int childNum) {
    connectionInfos[childNum].clientPid = -1;
    // close write end of pipe
    if (close(connectionInfos[childNum].pipeFds[WRITE_END_PIPE]) == -1) {
        errExit("close pipeFds[1]");
    }
}

void addNewConnection(struct ConnectionInfo connectionInfos[], int numOfClients, int childNum, pid_t clientPid) {
    connectionInfos[childNum].clientPid = clientPid;
    if (pipe(connectionInfos[childNum].pipeFds) == -1) {
        errExit("pipe");
    }
}

int findAvailableConnection(struct ConnectionInfo connectionInfos[], int numOfClients) {
    for (int i = 0; i < numOfClients; i++) {
        if (connectionInfos[i].clientPid == -1) {
            return i;
        }
    }
    return -1;
}

int findConnectionIndexByClientPid(struct ConnectionInfo connectionInfos[], int numOfClients, pid_t clientPid) {
    for (int i = 0; i < numOfClients; i++) {
        if (connectionInfos[i].clientPid == clientPid) {
            return i;
        }
    }
    return -1;
}

int findConnectionIndexByChildPid(struct ConnectionInfo connectionInfos[], int numOfClients, pid_t childPid) {
    for (int i = 0; i < numOfClients; i++) {
        if (connectionInfos[i].childPid == childPid) {
            return i;
        }
    }
    return -1;

}

enum CommandType getCommandTypeFromCommandString(const char* command) {
    if (strcmp(command, "Connect") == 0) {
        return CONNECT;
    } else if (strcmp(command, "tryConnect") == 0) {
        return TRYCONNECT;
    } else if (strcmp(command, "list") == 0) {
        return LIST;
    } else if (strcmp(command, "help") == 0) {
        return HELP;
    } else if(strcmp(command, "quit") == 0) {
        return QUIT;
    } else if (strcmp(command, "killServer") == 0) {
        return KILL;
    } else if (strcmp(command, "readF") == 0) {
        return READF;
    } else if (strcmp(command, "writeT") == 0) {
        return WRITET;
    } else if (strcmp(command, "upload") == 0) {
        return UPLOAD;
    } else if (strcmp(command, "download") == 0) {
        return DOWNLOAD;
    } else if (strcmp(command, "archServer") == 0) {
        return ARCHSERVER;
    }
    return UNKNOWN;
}

void getSemaphoreNameByFilename(const char* filename, pid_t parentPID, char* semaphoreName) {
    // Create the semaphore name using filename and parentPID
    snprintf(semaphoreName, MAX_SEMAPHORE_NAME_SIZE, "/%s_%d_sem", filename, parentPID);
}