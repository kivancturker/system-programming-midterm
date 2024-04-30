#include "command_handler.h"
#include "ipc.h"

#include <fcntl.h>
#include <errno.h>
#include <stdio.h>
#include <string.h>

void handleConnectCommand(struct Request request, struct Queue *serverQueue) {
    struct ConnectionRequest connectionRequest;
    connectionRequest.clientPid = request.clientPid;
    strcpy(connectionRequest.responseFifoName, request.commandArgs);
    int isEnqueued = enqueue(serverQueue, connectionRequest);
    if (isEnqueued == -1) {
        fprintf(stderr, "serverQueue enqueue for connect");
        exit(EXIT_FAILURE);
    }
}

void handleTryConnectCommand(struct Request request) {
    int responseFifoFd = 0;
    while((responseFifoFd = open(request.commandArgs, O_WRONLY, 0666)) == -1 && errno == ENOENT);
    if (responseFifoFd == -1) {
        errExit("open response for connect");
    }
    struct Response connectionResponse;
    connectionResponse.status = ERROR;
    writeResponseToFifo(responseFifoFd, connectionResponse);
    if(close(responseFifoFd) == -1) {
        errExit("close response fifo");
    }
    // Later remove this
    printf("Client PID %d connected\n", request.clientPid);
}

void handleCommandResponseByCommandType(enum CommandType commandType, struct Response response) {
    switch (commandType) {
        case HELP:
            handleHelpResponse(response);
            break;
        // Add cases for other command types here
        default:
            fprintf(stderr, "Invalid command type\n");
            break;
    }
}

void handleHelpResponse(struct Response response) {
    printf("%s\n", response.payload);
}

void handleCommand(struct Request request, int responseFifoFd) {
    struct Response response;
    response.status = OK;
    switch (request.commandType) {
        case HELP:
            response.status = OK;
            strcpy(response.payload, "HELP: Display this help message\n");
            break;
        default:
            response.status = ERROR;
            strcpy(response.payload, "Unknown command\n");
            break;
    }
    writeResponseToFifo(responseFifoFd, response);
}