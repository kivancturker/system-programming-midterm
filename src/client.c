#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <string.h>

#include "myutil.h"
#include "ipc.h"
#include "command_handler.h"
#include "mytypes.h"

int main(int argc, char *argv[]) {
    struct ClientArg clientArg;
    parseClientArgs(argc, argv, &clientArg);
    enum CommandType connectionType = getConnectionType(clientArg.connectionType); // Later put this on the request

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

    return 0;
}