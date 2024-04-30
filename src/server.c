#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <signal.h>
#include <string.h>

#include "myutil.h"
#include "ipc.h"
#include "command_handler.h"
#include "mytypes.h"
#include "queue.h"
#include "child.h"

sig_atomic_t sigIntrCount = 0;

void sigintHandler(int signal) {
    sigIntrCount++;
}

int main(int argc, char *argv[]) {
    struct ServerArg serverArg;
    parseServerArgs(argc, argv, &serverArg);
    // Disable for now
    // createDirIfNotExist(serverArg.filename);

    // Set handler for sigchild
    struct sigaction sa;
    memset(&sa, 0, sizeof(sa));
    sa.sa_handler = &sigintHandler;
    if (sigaction(SIGINT, &sa, NULL) == -1) {
        errExit("sigaction");
    }

    printf("Server Started PID: %d\n", getpid());
    printf("Waiting for clients...\n");

    // Create request fifo
    createFifoIfNotExist(REQUEST_FIFO_NAME);
    int requestFifoFd = open(REQUEST_FIFO_NAME, O_RDWR, 0666);
    if (requestFifoFd == -1) {
        errExit("open request fifo");
    }

    struct Request request;
    int isRequestReadInterrupted = 0;
    int availableChildCount = serverArg.numOfClients;
    // Create a server queue
    struct Queue serverQueue;
    initQueue(&serverQueue);
    struct ConnectionRequest connectionRequest;
    pid_t childPid = -1;
    struct ConnectionInfo connectionInfos[serverArg.numOfClients];
    initConnectionInfos(connectionInfos, serverArg.numOfClients);
    int availableConnectionIndex = -1;
    int connectionInfoIndex = -1;
    int childPipeWriteEndFd = -1;
    int isClientConnected = 0;
    while(1) {
        if (sigIntrCount > 0) {
            break;
        }
        isRequestReadInterrupted = readRequestFromFifo(requestFifoFd, &request);
        if (isRequestReadInterrupted) {
            continue;
        }
        switch(request.commandType) {
            case CONNECT:
                handleConnectCommand(request, &serverQueue);
                break;
            case TRYCONNECT:
                break;
            case LIST:
                break;
            default:
                connectionInfoIndex = findConnectionIndexByClientPid(connectionInfos, serverArg.numOfClients, request.clientPid);
                childPipeWriteEndFd = connectionInfos[connectionInfoIndex].pipeFds[WRITE_END_PIPE];
                forwardRequestToChild(childPipeWriteEndFd, request);
                break;
        }
        // If available process exist then connect client
        isClientConnected = findConnectionIndexByClientPid(connectionInfos, serverArg.numOfClients, request.clientPid) == -1;
        if (!isQueueEmpty(&serverQueue) && availableChildCount > 0 && isClientConnected) {
            dequeue(&serverQueue, &connectionRequest);
            availableConnectionIndex = findAvailableConnection(connectionInfos, serverArg.numOfClients);
            if (availableConnectionIndex == -1) {
                errExit("findAvailableConnection");
            }
            addNewConnection(connectionInfos, serverArg.numOfClients, availableConnectionIndex, connectionRequest.clientPid);
            childPid = fork();
            if (childPid == -1) {
                errExit("fork");
            }
            if (childPid == 0) {
                if (close(connectionInfos->pipeFds[WRITE_END_PIPE]) == -1) {
                    errExit("close pipeFds[1]");
                }
                childMain(connectionRequest, connectionInfos->pipeFds[READ_END_PIPE]);
                exit(EXIT_SUCCESS);
            }
            if (close(connectionInfos->pipeFds[READ_END_PIPE]) == -1) {
                errExit("close pipeFds[0]");
            }
            availableChildCount--;
        }
    }

    if (close(requestFifoFd) == -1) {
        errExit("close request fifo");
    }

    return 0;
}

