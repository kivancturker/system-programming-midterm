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

void sigintHandler(int signal) {
    sigIntrCount++;
}

int main(int argc, char *argv[]) {
    struct ClientArg clientArg;
    parseClientArgs(argc, argv, &clientArg);
    enum CommandType connectionType = getConnectionType(clientArg.connectionType); // Later put this on the request
    
    struct sigaction sa;
    memset(&sa, 0, sizeof(sa));
    sa.sa_handler = &sigintHandler;
    if (sigaction(SIGINT, &sa, NULL) == -1) {
        errExit("sigaction");
    }

    char responseFifoName[MAX_FILENAME_SIZE];
    createUniqueResponseFifoName(responseFifoName, getpid());

     // Establish by creating fifo
    int requestFifoFd = open(REQUEST_FIFO_NAME, O_WRONLY, 0666);
    if (requestFifoFd == -1) {
        errExit("open requets fifo");
    }

    // Setup the request for connection
    struct Request request;
    request.clientPid = getpid();
    request.commandType = connectionType;
    strcpy(request.commandArgs, responseFifoName);

    // Send the connection request    
    writeRequestToFifo(requestFifoFd, request);

    createFifoIfNotExist(responseFifoName);
    int responseFifoFd = open(responseFifoName, O_RDONLY, 0666);
    if (responseFifoFd == -1) {
        errExit("open response fifo");
    }
    // Wait for server to send connection acceptance response
    printf("Waiting for Que...\n");
    struct Response response;
    readResponseFromFifo(responseFifoFd, &response);
    if (response.status == ERROR) {
        printf("Connection Rejected\n");
        return 1;
    }
    printf("Connection Established\n");

    enum CommandType commandType;
    char command[255];
    char fileTransferFifoName[MAX_FILENAME_SIZE];
    while(1) {
        printf("\nEnter command: ");
        fgets(command, 255, stdin);
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
        if (commandType == UPLOAD || commandType == DOWNLOAD) {
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
    }

    return 0;
}