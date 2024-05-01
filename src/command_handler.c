#include "command_handler.h"
#include "ipc.h"

#include <fcntl.h>
#include <errno.h>
#include <stdio.h>
#include <string.h>

void handleCommand(struct Request request, int responseFifoFd) {
    struct Response response;
    response.status = OK;
    switch (request.commandType) {
        case HELP:
            handleHelpCommand(request, responseFifoFd);
            break;
        default:
            response.status = ERROR;
            strcpy(response.payload, "Unknown command\n");
            writeResponseToFifo(responseFifoFd, response);
            break;
    }
}

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

void handleHelpCommand(struct Request request, int responseFifoFd) {
    struct Response response;
    enum CommandType commandType = getCommandTypeFromCommandString(request.commandArgs);
    switch (commandType) {
        case HELP:
            strcpy(response.payload, "help <CommandName>\nDisplay the possible command\n");
            break;
        case LIST:
            strcpy(response.payload, "list\nSend request to display the list of files in the Server directory\n");
            break;
        case READF:
            strcpy(response.payload, "readF <file> <line #>\nDisplay the #th line of the <file>, "
            "returns with an error if <file> does not exist\n");
            break;
        case WRITET:
            strcpy(response.payload, "writeT <file> <line#> <string> \nRequest to write the content of <string> " 
            "to the #th line, if the <line#> not given writes to the end of file. If the file does not exists in " 
            "the Server in the Server directory creates and edits the file at the same time.\n");
            break;
        case UPLOAD:
            strcpy(response.payload, "upload <file>\nUploads the file from the current working directory of client"
            " to the Server directory (beware of cases no file in clients current working directory and file with "
            "the same name on Server side)\n");
            break;
        case DOWNLOAD:
            strcpy(response.payload, "download <file>\nRequest to receive <file> from Servers directory to client side\n");
            break;
        case ARCHSERVER:
            strcpy(response.payload, "archServer<fileName>.tar\nUsing fork exec and tar utilities create a child process"
            " that will collect all the files currently available on the Server side and store them in the <filename>.tar archive\n");
            break;
        case QUIT:
            strcpy(response.payload, "quit\nSend write request to Server side log file and quits\n");
            break;
        case KILL:
            strcpy(response.payload, "killServer\nSends a kill request to Server\n");
            break;
        default:
            strcpy(response.payload, "Available commands are:\nhelp, list, readF, writeT, upload, download, archServer, quit, killServer\n");
            break;
    }
    response.status = OK;
    if (responseFifoFd == -1) {
        errExit("open response fifo");
    }
    writeResponseToFifo(responseFifoFd, response);
}

// ********************** Response Part **********************

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