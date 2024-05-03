#include "command_handler.h"
#include "ipc.h"
#include "fileops.h"

#include <fcntl.h>
#include <errno.h>
#include <stdio.h>
#include <string.h>
#include <semaphore.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/stat.h>

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
        case WRITET:
            handleWriteTCommand(request, responseFifoFd, serverDir);
            break;
        case UPLOAD:
            handleUploadCommand(request, responseFifoFd, serverDir);
            break;
        case DOWNLOAD:
            handleDownloadCommand(request, responseFifoFd, serverDir);
            break;
        case QUIT:
            handleQuitCommand(request, responseFifoFd, serverDir);
            break;
        case ARCHSERVER:
            handleArchServerCommand(request, responseFifoFd, serverDir);
            break;
        case KILL:
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

void handleTryConnectCommand(struct Request request, struct Queue *serverQueue, const char* serverDir, int availableChildCount) {
    // Check if there is any available child and serverQueue is empty
    if (availableChildCount == 0) {
        // Reject the connection request
        struct Response response;
        response.status = ERROR;
        strcpy(response.payload, "Server Que Full\n");
        int responseFifoFd = 0;
        NO_EINTR(responseFifoFd = open(request.commandArgs, O_WRONLY, 0666));
        if (responseFifoFd == -1) {
            errExit("open response fifo");
        }
        writeResponseToFifo(responseFifoFd, response);
        if (close(responseFifoFd) == -1) {
            errExit("close responseFifoFd");
        }
        return;
    }
    handleConnectCommand(request, serverQueue, serverDir);
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
    for (int i = 0; i < numOfFiles; i++) {
        free(filenames[i]);
    }
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
        strcpy(response.payload, "<No Record Exist in This Line>\n");
    } else {
        strcpy(response.payload, line);
    }
    sem_post(semaphore);
    writeResponseToFifo(responseFifoFd, response);
    free(line);
}

void handleWriteTCommand(struct Request request, int responseFifoFd, const char* serverDir) {
    struct Response response;
    memset(response.payload, 0, MAX_PAYLOAD_SIZE);
    response.status = OK;
    // First argument is the filename second is optional line number and third is the string to write
    char filename[MAX_FILENAME_SIZE];
    int lineNum = 0;
    char stringToWrite[MAX_PAYLOAD_SIZE];
    char errMessage[256];
    if (parseWriteTCommandArgs(request.commandArgs, filename, &lineNum, stringToWrite, errMessage) == -1) {
        sendErrorResponse(responseFifoFd, errMessage);
        return;
    }
    // Check if the file exists
    int fd = -1;
    if (!isFileExists(serverDir, filename)) {
        // Create the file in the directory
        char filePath[MAX_FILENAME_SIZE];
        snprintf(filePath, MAX_FILENAME_SIZE, "%s/%s", serverDir, filename);
        NO_EINTR(fd = open(filePath, O_CREAT | O_WRONLY, 0666));
        if (fd == -1) {
            perror("open");
            exit(EXIT_FAILURE);
        }
        // I close it because I just wantted to create it
        if (close(fd) == -1) {
            perror("close");
            exit(EXIT_FAILURE);
        }
        // Create the semaphore for the file
        createSemaphoreForGivenFile(serverDir, filename);
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
    writeLineToFile(serverDir, filename, stringToWrite, lineNum);
    sem_post(semaphore);
    strcpy(response.payload, "Write successful\n");
    writeResponseToFifo(responseFifoFd, response);
}

// Return -1 if there is an error, 0 otherwise
int parseWriteTCommandArgs(char* commandArgs, char* filename, int* lineNum, char* stringToWrite, char* errMessage) {
    char commandArgCopy[MAX_PAYLOAD_SIZE];
    strcpy(commandArgCopy, commandArgs);

    strcpy(filename, strtok(commandArgs, " "));
    if (filename == NULL) {
        strcpy(errMessage, "Argument for filename is invalid\n");
        return -1;
    }

    char* lineNumStr = strtok(NULL, " ");
    if (lineNumStr == NULL) {
        strcpy(errMessage, "Argument for line number is invalid\n");
        return -1;
    }

    // Check if the line number is a valid integer
    char* end;
    *lineNum = (int) strtol(lineNumStr, &end, 10);
    if (end == lineNumStr || *end != '\0' || errno == ERANGE) {
        // lineNumStr is not a valid integer
        // Then the argument is writeT <filename> <string>
        strcpy(stringToWrite, strtok(commandArgCopy, " ")); // Just to get rid of the filename part
        strcpy(stringToWrite, strtok(NULL, "")); // Entire srting after the second space
        *lineNum = -1;
    }
    else {
        // lineNumStr is a valid integer
        strcpy(stringToWrite, strtok(NULL, ""));
    }
    return 0;
}

void handleUploadCommand(struct Request request, int responseFifoFd, const char* serverDir) {
    struct Response response;
    memset(response.payload, 0, MAX_PAYLOAD_SIZE);
    response.status = OK;
    
    char* filename = strtok(request.commandArgs, " ");
    if (filename == NULL) {
        sendErrorResponse(responseFifoFd, "Argument for filename is invalid\n");
        return;
    }

    char* fileTransferFifoName = strtok(NULL, " ");
    if (fileTransferFifoName == NULL) {
        sendErrorResponse(responseFifoFd, "An error occured while establishing upload fifo\n");
        return;
    }
    if (mkfifo(fileTransferFifoName, 0666) == -1) {
        errExit("mkfifo fileTransferFifo");
    }
    strcpy(response.payload, filename);
    strcat(response.payload, " ");
    strcat(response.payload, fileTransferFifoName);
    writeResponseToFifo(responseFifoFd, response);

    char filepath[MAX_FILENAME_SIZE];
    sprintf(filepath, "%s/%s", serverDir, filename);
    int bytesTransferred = receiveFile(filepath, fileTransferFifoName);
    if (bytesTransferred == -1) {
        sendErrorResponse(responseFifoFd, "An error occured while receiving file\n");
        return;
    }

    // Create a semaphore for that file
    createSemaphoreForGivenFile(serverDir, filename);

    if (unlink(fileTransferFifoName) == -1) {
        errExit("unlink fileTransferFifo");
    }
}

void handleDownloadCommand(struct Request request, int responseFifoFd, const char* serverDir) {
    struct Response response;
    memset(response.payload, 0, MAX_PAYLOAD_SIZE);
    response.status = OK;
    char* filename = strtok(request.commandArgs, " ");
    if (filename == NULL) {
        sendErrorResponse(responseFifoFd, "Argument for filename is invalid\n");
        return;
    }
    // If file does not exist in the server directory
    if (!isFileExists(serverDir, filename)) {
        sendErrorResponse(responseFifoFd, "File does not exist on the server\n");
        return;
    }
    char* fileTransferFifoName = strtok(NULL, " ");
    if (fileTransferFifoName == NULL) {
        sendErrorResponse(responseFifoFd, "An error occured while establishing download fifo\n");
        return;
    }
    if (mkfifo(fileTransferFifoName, 0666) == -1) {
        errExit("mkfifo fileTransferFifo");
    }
    // Open semaphore
    char semaphoreName[MAX_SEMAPHORE_NAME_SIZE];
    getSemaphoreNameByFilename(filename, getppid(), semaphoreName);
    sem_t* semaphore = sem_open(semaphoreName, O_CREAT, 0666, 1);
    if (semaphore == SEM_FAILED) {
        perror("sem_open");
        exit(EXIT_FAILURE);
    }
    sem_wait(semaphore);

    strcpy(response.payload, filename);
    strcat(response.payload, " ");
    strcat(response.payload, fileTransferFifoName);
    writeResponseToFifo(responseFifoFd, response);

    char filepath[MAX_FILENAME_SIZE];
    sprintf(filepath, "%s/%s", serverDir, filename);
    int bytesTransferred = transferFile(filepath, fileTransferFifoName);
    if (bytesTransferred == -1) {
        sendErrorResponse(responseFifoFd, "An error occured while transferring file\n");
        return;
    }
    if (unlink(fileTransferFifoName) == -1) {
        errExit("unlink fileTransferFifo");
    }
    sem_post(semaphore);
}

void handleQuitCommand(struct Request request, int responseFifoFd, const char* serverDir) {
    struct Response response;
    response.status = OK;
    strcpy(response.payload, "Log file write request granted\nbye...\n");
    writeResponseToFifo(responseFifoFd, response);
}

void handleArchServerCommand(struct Request request, int responseFifoFd, const char* serverDir) {
    struct Response response;
    response.status = OK;
    memset(response.payload, 0, MAX_PAYLOAD_SIZE);
    // Take the arguement from commandArgs filename.tar
    // Then fork exec, to run tar to archive all the files in the server directory
    char* tarname = strtok(request.commandArgs, " ");
    if (tarname == NULL) {
        sendErrorResponse(responseFifoFd, "Argument for filename is invalid\n");
        return;
    }
    char* fileTransferFifoName = strtok(NULL, "");
    if (fileTransferFifoName == NULL) {
        sendErrorResponse(responseFifoFd, "An error occured while establishing archive fifo\n");
        return;
    }
    if (mkfifo(fileTransferFifoName, 0666) == -1) {
        errExit("mkfifo fileTransferFifo");
    }
    // Critical Section - Wait all the file semaphores
    // Get all filenames 
    // open semaphore for each of them
    int numOfFiles = getNumOfFilesInDir(serverDir);
    char* filenames[numOfFiles];
    sem_t *semaphores[numOfFiles];
    // Do not forget to free filenames !!!
    getAllTheFilenamesInDir(serverDir, filenames, numOfFiles);
    // Open all the semaphores
    for (int i = 0; i < numOfFiles; i++) {
        char semaphoreName[MAX_SEMAPHORE_NAME_SIZE];
        getSemaphoreNameByFilename(filenames[i], getppid(), semaphoreName);
        semaphores[i] = sem_open(semaphoreName, O_CREAT, 0666, 1);
        if (semaphores[i] == SEM_FAILED) {
            perror("sem_open");
            exit(EXIT_FAILURE);
        }
    }
    // Wait for all the semaphores
    for (int i = 0; i < numOfFiles; i++) {
        sem_wait(semaphores[i]);
    }

    char tarCommand[MAX_PAYLOAD_SIZE];
    sprintf(tarCommand, "tar -cf %s/%s %s/*", serverDir, tarname, serverDir);
    // Now fork exec to run that tar command not system because system is not async safe
    pid_t pid = fork();
    if (pid == -1) {
        errExit("fork");
    }
    if (pid == 0) {
        execl("/bin/sh", "sh", "-c", tarCommand, (char*) NULL);
        errExit("execl");
    }
    // Wait for the child to finish
    int status = 0;
    int waitResult = 0;
    while ((waitResult = waitpid(pid, &status, 0)) == -1 && errno == EINTR);
    if (waitResult == -1) {
        errExit("waitpid");
    }
    if (WIFEXITED(status) && WEXITSTATUS(status) == 0) {
        strcpy(response.payload, tarname);
        strcat(response.payload, " ");
        strcat(response.payload, fileTransferFifoName);
        writeResponseToFifo(responseFifoFd, response);
    }
    else {
        sendErrorResponse(responseFifoFd, "An error occured while creating archive\n");
    }

    // Transfer the archieve file to the client working directory
    char tarPath[MAX_ARG_SIZE];
    snprintf(tarPath, MAX_ARG_SIZE, "%s/%s", serverDir, tarname);
    int bytesTransferred = transferFile(tarPath, fileTransferFifoName);
    if (bytesTransferred == -1) {
        sendErrorResponse(responseFifoFd, "An error occured while transferring tar file\n");
        return;
    }

    for (int i = 0; i < numOfFiles; i++) {
        sem_post(semaphores[i]);
        sem_close(semaphores[i]);
    }
    for (int i = 0; i < numOfFiles; i++) {
        free(filenames[i]);
    }

    // Remove archieve from server directory
    unlink(tarPath);
    
    if (unlink(fileTransferFifoName) == -1) {
        errExit("unlink fileTransferFifo");
    }
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
        case WRITET:
            handleWriteTResponse(response);
            break;
        case UPLOAD:
            handleUploadResponse(response);
            break;
        case DOWNLOAD:
            handleDownlaodResponse(response);
            break;
        case QUIT:
            handleQuitResponse(response);
            break;
        case ARCHSERVER:
            handleArchServerResponse(response);
            break;
        case KILL:
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

void handleWriteTResponse(struct Response response) {
    printf("%s\n", response.payload);
}

void handleUploadResponse(struct Response response) {
    // Payload is fileTransferFifoName
    char fileTransferFifoName[MAX_FILENAME_SIZE];
    char filename[MAX_FILENAME_SIZE];
    strcpy(filename, strtok(response.payload, " "));
    strcpy(fileTransferFifoName, strtok(NULL, ""));
    // Check if file to transfer exists
    if (!isFileExists(".", filename)) {
        fprintf(stderr, "File to transfer does not exist\n");
        cancelTransfer(fileTransferFifoName);
        fprintf(stderr, "Upload cancelled\n");
        return;
    }
    printf("File transfer request received\n");
    printf("Begining file transfer\n");
    int bytesTransferred = transferFile(filename, fileTransferFifoName);
    if (bytesTransferred == -1) {
        fprintf(stderr, "An error occured while transferring file\n");
    }
    else {
        printf("%d bytes transferred\n", bytesTransferred);
    }
}

void handleDownlaodResponse(struct Response response) {
    // Payload is fileTransferFifoName
    char fileTransferFifoName[MAX_FILENAME_SIZE];
    char filename[MAX_FILENAME_SIZE];
    strcpy(filename, strtok(response.payload, " "));
    strcpy(fileTransferFifoName, strtok(NULL, ""));
    printf("File download request received\n");
    printf("Begining file transfer\n");
    int bytesReceived = receiveFile(filename, fileTransferFifoName);
    if (bytesReceived == -1) {
        fprintf(stderr, "An error occured while receiving file\n");
    }
    else {
        printf("%d bytes transferred\n", bytesReceived);
    }
}

void handleQuitResponse(struct Response response) {
    printf("%s\n", response.payload);
}

void handleArchServerResponse(struct Response response) {
    // Parse response
    char tarname[MAX_FILENAME_SIZE];
    char fileTransferFifoName[MAX_FILENAME_SIZE];
    char* token = strtok(response.payload, " ");
    if (token == NULL) {
        fprintf(stderr, "An error occured while parsing archive response\n");
        return;
    }
    strcpy(tarname, token);
    token = strtok(NULL, "");
    if (token == NULL) {
        fprintf(stderr, "An error occured while parsing archive response\n");
        return;
    }
    strcpy(fileTransferFifoName, token);
    printf("Archive request received\n");
    printf("Begining file transfer\n");
    int bytesReceived = receiveFile(tarname, fileTransferFifoName);
    if (bytesReceived == -1) {
        fprintf(stderr, "An error occured while receiving archive\n");
    }
    else {
        printf("%d bytes transferred\n", bytesReceived);
        printf("Archieve removed from server directory\n");
    }
}