#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <signal.h>
#include <string.h>
#include <sys/wait.h>
#include <semaphore.h>

#include "myutil.h"
#include "ipc.h"
#include "command_handler.h"
#include "mytypes.h"
#include "queue.h"
#include "child.h"
#include "fileops.h"
#include "logger.h"

sig_atomic_t sigIntrCount = 0;
sig_atomic_t sigChildCount = 0;

void sigintHandler(int signal) {
    sigIntrCount++;
}

void sigchildHandler(int signal) {
    sigChildCount++;
}

int main(int argc, char *argv[]) {
    struct ServerArg serverArg;
    parseServerArgs(argc, argv, &serverArg);

    createDirIfNotExist(serverArg.dirname);

    createLogFile();

    // Set handler for sigchild
    struct sigaction sa;
    memset(&sa, 0, sizeof(sa));
    sa.sa_handler = &sigchildHandler;
    if (sigaction(SIGCHLD, &sa, NULL) == -1) {
        errExit("sigaction");
    }

    memset(&sa, 0, sizeof(sa));
    sa.sa_handler = &sigintHandler;
    if (sigaction(SIGINT, &sa, NULL) == -1) {
        errExit("sigaction");
    }

    // Create named semaphore for every file in the directory
    createSemaphores(serverArg.dirname);

    printf("Server Started PID: %d\n", getpid());
    printf("Waiting for clients...\n");

    // Create request fifo
    char requestFifoName[255];
    createUniqueRequestFifoName(requestFifoName, getpid());
    createFifoIfNotExist(requestFifoName);
    int requestFifoFd = open(requestFifoName, O_RDWR, 0666);
    if (requestFifoFd == -1) {
        errExit("open request fifo");
    }

    char starterLogMessage[255];
    snprintf(starterLogMessage, 255, "Server Started PID: %d\n", getpid());
    writeLog(starterLogMessage);
    writeLog("Waiting for clients...\n");

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
    int waitedPid = -1;
    int keepServerRunning = 1;
    while(keepServerRunning) {
        if (sigIntrCount > 0) {
            break;
        }
        if (sigChildCount == 0) {
            isRequestReadInterrupted = readRequestFromFifo(requestFifoFd, &request);
            if (isRequestReadInterrupted) {
                continue;
            }
            switch(request.commandType) {
                case CONNECT:
                    handleConnectCommand(request, &serverQueue, serverArg.dirname);
                    break;
                case TRYCONNECT:
                    handleTryConnectCommand(request, &serverQueue, serverArg.dirname, availableChildCount);
                    break;
                case KILL:
                    handleKillServerCommand(request, connectionInfos, serverArg.numOfClients);
                    keepServerRunning = 0;
                    continue;
                default:
                    connectionInfoIndex = findConnectionIndexByClientPid(connectionInfos, serverArg.numOfClients, request.clientPid);
                    childPipeWriteEndFd = connectionInfos[connectionInfoIndex].pipeFds[WRITE_END_PIPE];
                    forwardRequestToChild(childPipeWriteEndFd, request);
                    break;
            }
        }
        while ((waitedPid = waitpid(-1, NULL, WNOHANG)) > 0) {
            int connectionIndexOfWaitedClient = findConnectionIndexByChildPid(connectionInfos, serverArg.numOfClients, waitedPid);
            if (request.commandType != KILL) {
                char quitLogMessage[255];
                snprintf(quitLogMessage, 255, "client%d disconnected...\n", connectionIndexOfWaitedClient);
                printf("%s", quitLogMessage);
                writeLog(quitLogMessage);
            }
            removeConnection(connectionInfos, serverArg.numOfClients, connectionIndexOfWaitedClient);
            availableChildCount++;
            sigChildCount--;
        }
        if (waitedPid == -1 && errno != ECHILD) {
            errExit("waitpid");
        }
        // If available process exist then connect client
        isClientConnected = findConnectionIndexByClientPid(connectionInfos, serverArg.numOfClients, request.clientPid) == -1;
        while (!isQueueEmpty(&serverQueue) && availableChildCount > 0 && isClientConnected) {
            dequeue(&serverQueue, &connectionRequest);
            availableConnectionIndex = findAvailableConnection(connectionInfos, serverArg.numOfClients);
            if (availableConnectionIndex == -1) {
                errExit("findAvailableConnection");
            }
            connectionRequest.clientNum = availableConnectionIndex;
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
            char connectionLogMessage[255];
            snprintf(connectionLogMessage, 255, "Client PID %d, connected as \"client%d\"\n", connectionRequest.clientPid, availableConnectionIndex);
            writeLog(connectionLogMessage);
            connectionInfos[availableConnectionIndex].childPid = childPid;
            availableChildCount--;
        }
    }

    // Forward kill request to all children
    struct Request killRequest;
    killRequest.commandType = KILL;
    memset(killRequest.commandArgs, 0, MAX_ARG_SIZE);
    for (int i = 0; i < serverArg.numOfClients && request.commandType != KILL; i++) {
        if (connectionInfos[i].childPid != -1) {
            forwardRequestToChild(connectionInfos[i].pipeFds[WRITE_END_PIPE], killRequest);
        }
    }
    // Wait all the remaining children
    while ((waitedPid = waitpid(-1, NULL, 0)) > 0) {
        int connectionIndexOfWaitedClient = findConnectionIndexByChildPid(connectionInfos, serverArg.numOfClients, waitedPid);
        if (request.commandType != KILL) {
            char interruptLogMessage[255];
            snprintf(interruptLogMessage, 255, "client%d disconnected...\n", connectionIndexOfWaitedClient);
            printf("%s", interruptLogMessage);
            writeLog(interruptLogMessage);
        }
        removeConnection(connectionInfos, serverArg.numOfClients, connectionIndexOfWaitedClient);
        availableChildCount++;
        sigChildCount--;
    }

    if (close(requestFifoFd) == -1) {
        errExit("close request fifo");
    }
    unlink(requestFifoName);

    destroyAllSemaphores(serverArg.dirname);
    
    printf("bye\n");
    writeLog("bye\n");
    return 0;
}

