#include "child.h"
#include "ipc.h"
#include "command_handler.h"

#include <string.h>
#include <stdio.h>
#include <fcntl.h>

void childMain(struct ConnectionRequest connectionRequest, int pipeFd) {
    char responseFifoName[MAX_FILENAME_SIZE];
    pid_t clientPid = connectionRequest.clientPid;
    strcpy(responseFifoName, connectionRequest.responseFifoName);

    int responseFifoFd = open(responseFifoName, O_WRONLY, 0666);
    if (responseFifoFd == -1) {
        errExit("open response fifo");
    }
    struct Response response;
    response.status = OK;
    writeResponseToFifo(responseFifoFd, response);
    printf("Client PID %d connected\n", clientPid);

    while(1) {
        struct Request request;
        readForwardedRequestFromServer(pipeFd, &request);
        handleCommand(request, responseFifoFd);
    }

    if (close(pipeFd) == -1) {
        errExit("close pipe");
    }

    if (close(responseFifoFd) == -1) {
        errExit("close response fifo");
    }
}