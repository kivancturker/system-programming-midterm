#include "child.h"
#include "ipc.h"
#include "command_handler.h"

#include <string.h>
#include <stdio.h>
#include <fcntl.h>

void childMain(struct ConnectionRequest connectionRequest, int pipeFd) {
    char responseFifoName[MAX_FILENAME_SIZE];
    char serverDir[MAX_FILENAME_SIZE];
    pid_t clientPid = connectionRequest.clientPid;
    strcpy(responseFifoName, connectionRequest.responseFifoName);
    strcpy(serverDir, connectionRequest.serverDir);

    int responseFifoFd = open(responseFifoName, O_WRONLY, 0666);
    if (responseFifoFd == -1) {
        errExit("open response fifo child");
    }
    struct Response response;
    response.status = OK;
    writeResponseToFifo(responseFifoFd, response);
    printf("Client PID %d connected as \"client%d\"\n", clientPid, connectionRequest.clientNum);

    while(1) {
        struct Request request;
        readForwardedRequestFromServer(pipeFd, &request);
        handleCommand(request, responseFifoFd, serverDir);
        if (request.commandType == QUIT || request.commandType == KILL) {
            break;
        }
    }

    if (close(pipeFd) == -1) {
        errExit("close pipe");
    }

    if (close(responseFifoFd) == -1) {
        errExit("close response fifo");
    }
}