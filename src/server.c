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

sig_atomic_t sigIntrCount = 0;
sig_atomic_t sigChldCount = 0;

void sigchldHandler(int signal) {
    sigChldCount++;
}

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
    sa.sa_handler = &sigchldHandler;
    if (sigaction(SIGCHLD, &sa, NULL) == -1) {
        errExit("sigaction");
    }

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
    while(1) {
        if (sigIntrCount > 0) {
            printf("Interrupted\n");
            break;
        }
        isRequestReadInterrupted = readRequestFromFifo(requestFifoFd, &request);
        if (isRequestReadInterrupted) {
            continue;
        }
        switch(request.commandType) {
            case CONNECT:
                handleConnectCommand(request);
                break;
            case TRYCONNECT:
                break;
            case LIST:
                break;
        }
    }

    if (close(requestFifoFd) == -1) {
        errExit("close request fifo");
    }

    return 0;
}