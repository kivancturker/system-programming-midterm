#include "command_handler.h"
#include "ipc.h"

#include <fcntl.h>
#include <errno.h>
#include <stdio.h>

void handleConnectCommand(struct Request request) {
    // First send connection accepted response
    int responseFifoFd = 0;
    while((responseFifoFd = open(request.commandArgs, O_WRONLY, 0666)) == -1 && errno == ENOENT);
    if (responseFifoFd == -1) {
        errExit("open response for connect");
    }
    struct Response connectionResponse;
    connectionResponse.status = OK;
    writeResponseToFifo(responseFifoFd, connectionResponse);
    if(close(responseFifoFd) == -1) {
        errExit("close response fifo");
    }
    // Later remove this
    printf("Client PID %d connected\n", request.clientPid);
}

void handleTryConnectCommand(struct Request request) {

}