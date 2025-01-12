#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <string.h>
#include <signal.h>

#include "myutil.h"
#include "ipc.h"
#include "command_handler.h"
#include "mytypes.h"

sig_atomic_t sigIntrCount = 0;
sig_atomic_t sigTermCount = 0;

void sigintHandler(int signal) {
    sigIntrCount++;
}

void sigtermHandler(int signal) {
    sigTermCount++;
}

int main(int argc, char *argv[]) {
    struct ClientArg clientArg;
    parseClientArgs(argc, argv, &clientArg);
    enum CommandType connectionType = getConnectionType(clientArg.connectionType); // Later put this on the request
    
    // Check if process exist with given pid
    int isProcessExist = kill(clientArg.serverPid, 0) == 0;
    if (!isProcessExist) {
        fprintf(stderr, "Server with pid %d does not exist\n", clientArg.serverPid);
        return 1;
    }
    struct sigaction sa;
    memset(&sa, 0, sizeof(sa));
    sa.sa_handler = &sigintHandler;
    if (sigaction(SIGINT, &sa, NULL) == -1) {
        errExit("sigaction");
    }

    memset(&sa, 0, sizeof(sa));
    sa.sa_handler = &sigtermHandler;
    if (sigaction(SIGTERM, &sa, NULL) == -1) {
        errExit("sigaction");
    }

    char responseFifoName[MAX_FILENAME_SIZE];
    createUniqueResponseFifoName(responseFifoName, getpid());

     // Establish by creating fifo
    char requestFifoName[255];
    createUniqueRequestFifoName(requestFifoName, clientArg.serverPid);
    int requestFifoFd = open(requestFifoName, O_WRONLY, 0666);
    if (requestFifoFd == -1 && errno == ENOENT) {
        fprintf(stderr, "Server with pid %d does not exist\n", clientArg.serverPid);
        return 1;
    }
    if (requestFifoFd == -1) {
        errExit("open requets fifo");
    }

    // Setup the request for connection
    struct Request request;
    request.clientPid = getpid();
    request.commandType = connectionType;
    strcpy(request.commandArgs, responseFifoName);

    createFifoIfNotExist(responseFifoName);

    // Send the connection request    
    writeRequestToFifo(requestFifoFd, request);

    int responseFifoFd = open(responseFifoName, O_RDONLY, 0666);
    if (responseFifoFd == -1) {
        errExit("open response fifo");
    }

    // Wait for server to send connection acceptance response
    printf("Waiting for Que...\n");
    struct Response response;
    readResponseFromFifo(responseFifoFd, &response);
    if (response.status == ERROR) {
        printf("Connection Rejected. Queue is Full\n");
        if (close(requestFifoFd) == -1) {
            errExit("close request fifo");
        }
        if (close(responseFifoFd) == -1) {
            errExit("close response fifo");
        }
        unlink(responseFifoName);
        return 1;
    }
    printf("Connection Established\n");

    enum CommandType commandType;
    char command[255];
    char fileTransferFifoName[MAX_FILENAME_SIZE];
    while(1) {
        printf("\nEnter command: ");
        fgets(command, 255, stdin);
        if (sigTermCount > 0) {
            printf("Exiting...\n");
            break;
        }
        if (sigIntrCount > 0) {
            printf("Exiting...\n");
            request.clientPid = getpid();
            request.commandType = QUIT;
            writeRequestToFifo(requestFifoFd, request);
            break;
        }
        command[strlen(command) - 1] = '\0'; // Remove newline
        
        char *commandTypePart = strtok(command, " ");
        char *commandArgPart = strtok(NULL, "");

        commandType = getCommandTypeFromCommandString(commandTypePart);
        if (commandType == UNKNOWN) {
            printf("Unknown command\n");
            continue;
        }
        if (commandType == UPLOAD || commandType == DOWNLOAD || commandType == ARCHSERVER) {
            createUniqueFileTransferFifoName(fileTransferFifoName, getpid());
            strcat(commandArgPart, " ");
            strcat(commandArgPart, fileTransferFifoName);
        }
        request.clientPid = getpid();
        request.commandType = commandType;
        // Add the command arguments to the request
        if (commandArgPart != NULL) {
            strncpy(request.commandArgs, commandArgPart, sizeof(request.commandArgs) - 1);
        }
        else {
            request.commandArgs[0] = '\0';
        }

        writeRequestToFifo(requestFifoFd, request);
        readResponseFromFifo(responseFifoFd, &response);
        if (response.status == OK) {
            handleCommandResponseByCommandType(commandType, response);
        }
        else {
            handleErrorResponse(response);
        }
        if (commandType == QUIT || commandType == KILL) {
            break;
        }
    }

    if (close(requestFifoFd) == -1) {
        errExit("close request fifo");
    }
    
    if (close(responseFifoFd) == -1) {
        errExit("close response fifo");
    }
    unlink(responseFifoName);

    return 0;
}