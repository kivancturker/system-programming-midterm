#include "command_handler.h"
#include "ipc.h"
#include "fileops.h"

#include <fcntl.h>
#include <errno.h>
#include <stdio.h>
#include <string.h>
#include <semaphore.h>

void handleCommand(struct Request request, int responseFifoFd, const char* serverDir) {
    struct Response response;
    response.status = OK;
    switch (request.commandType) {
        case HELP:
            handleHelpCommand(request, responseFifoFd);
            break;
        case LIST:
            handleListCommand(request, responseFifoFd, serverDir);
            break;
        case READF:
            handleReadFCommand(request, responseFifoFd, serverDir);
            break;
        default:
            response.status = ERROR;
            strcpy(response.payload, "Unknown command\n");
            writeResponseToFifo(responseFifoFd, response);
            break;
    }
}

void handleConnectCommand(struct Request request, struct Queue *serverQueue, const char* serverDir) {
    struct ConnectionRequest connectionRequest;
    connectionRequest.clientPid = request.clientPid;
    strcpy(connectionRequest.serverDir, serverDir);
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

void handleListCommand(struct Request request, int responseFifoFd, const char* serverDir) {
    int numOfFiles = getNumOfFilesInDir(serverDir);
    char* filenames[numOfFiles];
    getAllTheFilenamesInDir(serverDir, filenames, numOfFiles);
    struct Response response;
    memset(response.payload, 0, MAX_PAYLOAD_SIZE);
    response.status = OK;
    for (int i = 0; i < numOfFiles; i++) {
        strcat(response.payload, filenames[i]);
        strcat(response.payload, "\n");
    }
    writeResponseToFifo(responseFifoFd, response);
}

void handleReadFCommand(struct Request request, int responseFifoFd, const char* serverDir) {
    struct Response response;
    response.status = OK;
    // First argument is the filename and second is linenum
    char* filename = strtok(request.commandArgs, " ");
    if (filename == NULL) {
        // Handle error: no filename found
        response.status = ERROR;
        strcpy(response.payload, "Argument for filename is invalid\n");
        writeResponseToFifo(responseFifoFd, response);
        return;
    }

    char* lineNumStr = strtok(NULL, " ");
    int lineNum = 0;  
    if (lineNumStr != NULL) { 
        lineNum = atoi(lineNumStr);
    }
    /*
    if (lineNum == 0 && strcmp(lineNumStr, "0") != 0 && lineNumStr != NULL) {
        // Handle error: conversion to integer failed
        response.status = ERROR;
        strcpy(response.payload, "Argument for line# is invalid\n");
        writeResponseToFifo(responseFifoFd, response);
        return;
    }*/
    // Check if the file exists
    if (!isFileExists(serverDir, filename)) {
        response.status = ERROR;
        strcpy(response.payload, "File does not exist\n");
        writeResponseToFifo(responseFifoFd, response);
        return;
    }
    // Check semaphore availability
    char semaphoreName[MAX_SEMAPHORE_NAME_SIZE];
    // Careful you need to use getppid() here, because child process is the one that is going to read the file
    getSemaphoreNameByFilename(filename, getppid(), semaphoreName);
    sem_t* semaphore = sem_open(semaphoreName, O_CREAT, 0666, 1);
    if (semaphore == SEM_FAILED) {
        perror("sem_open");
        exit(EXIT_FAILURE);
    }
    sem_wait(semaphore);
    char* line;
    if (lineNumStr == NULL){
        // Read the whole file
        line = readWholeFile(serverDir, filename);
    }
    else {
        line = readLineFromFile(serverDir, filename, lineNum);
    }
    if (line == NULL) {
        response.status = ERROR;
        strcpy(response.payload, "File does not exist\n");
    } else {
        strcpy(response.payload, line);
    }
    sem_post(semaphore);
    writeResponseToFifo(responseFifoFd, response);
    free(line);
}

// ********************** Response Part **********************

void handleCommandResponseByCommandType(enum CommandType commandType, struct Response response) {
    switch (commandType) {
        case HELP:
            handleHelpResponse(response);
            break;
        case LIST:
            handleListResponse(response);
            break;
        case READF:
            handleReadFResponse(response);
            break;
        default:
            fprintf(stderr, "Invalid command type\n");
            break;
    }
}

void handleErrorResponse(struct Response response) {
    fprintf(stderr, "Error: %s\n", response.payload);
}

void handleHelpResponse(struct Response response) {
    printf("%s\n", response.payload);
}

void handleListResponse(struct Response response) {
    printf("%s\n", response.payload);
}

void handleReadFResponse(struct Response response) {
    printf("%s\n", response.payload);
}