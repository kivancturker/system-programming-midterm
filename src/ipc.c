#include "ipc.h"
#include "myutil.h"
#include "command_handler.h"

#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>

void createUniqueRequestFifoName(char *requestFifoName, int clientPid) {
    sprintf(requestFifoName, REQUEST_FIFO_TEMPLATE "%d", clientPid);
}

void createUniqueResponseFifoName(char *responseFifoName, int clientPid) {
    sprintf(responseFifoName, RESPONSE_FIFO_TEMPLATE "%d", clientPid);
}

void createUniqueFileTransferFifoName(char *fileTransferFifoName, int clientPid) {
    sprintf(fileTransferFifoName, FILETRANSFER_FIFO_TEMPLATE "%d", clientPid);
}

int readRequestFromFifo(int fifoFd, struct Request* request) {
    int requestByteSize = getRequestSize();
    int bytesRead = 0;
    char requestBuffer[requestByteSize];
    bytesRead = read(fifoFd, requestBuffer, requestByteSize);
    if (bytesRead == -1 && errno != EINTR) {
        errExit("read");
    }
    if (bytesRead == -1 && errno == EINTR) {
        return 1;
    }
    // Ensure that enough bytes were read
    if (bytesRead < requestByteSize && bytesRead != 0) {
        fprintf(stderr, "Incomplete data read from request fifo\n");
        exit(EXIT_FAILURE);
    }
    int offset = 0;
    memcpy(&request->clientPid, requestBuffer + offset, sizeof(int));
    offset += sizeof(int);
    memcpy(&request->commandType, requestBuffer + offset, sizeof(enum CommandType));
    offset += sizeof(enum CommandType);
    memcpy(&request->commandArgs, requestBuffer + offset, MAX_ARG_SIZE);

    return 0;
}

void writeRequestToFifo(int fifoFd, struct Request request) {
    size_t requestByteSize = getRequestSize();
    char requestBuffer[requestByteSize];
    memset(requestBuffer, 0, requestByteSize);
    int offset = 0;

    // Copy the data of the struct into the buffer
    memcpy(requestBuffer + offset, &request.clientPid, sizeof(int));
    offset += sizeof(int);
    memcpy(requestBuffer + offset, &request.commandType, sizeof(enum CommandType));
    offset += sizeof(enum CommandType);
    memcpy(requestBuffer + offset, request.commandArgs, MAX_ARG_SIZE);

    // Write the buffer to the FIFO
    ssize_t bytesWritten = 0;
    NO_EINTR(bytesWritten = write(fifoFd, requestBuffer, requestByteSize));
    if (bytesWritten == -1) {
        errExit("write");
    }
}

void readResponseFromFifo(int fifoFd, struct Response* response) {
    int responseByteSize = getResponseSize();
    char responseBuffer[responseByteSize];
    int bytesRead = 0;

    // Read data from the FIFO
    NO_EINTR(bytesRead = read(fifoFd, responseBuffer, responseByteSize));
    if (bytesRead == -1) {
        errExit("read");
    }

    // Ensure that enough bytes were read
    if (bytesRead < responseByteSize) {
        fprintf(stderr, "Incomplete data read from response fifo\n");
        exit(EXIT_FAILURE);
    }

    // Extract data from the buffer and populate the struct Response
    int offset = 0;
    memcpy(response->payload, responseBuffer + offset, MAX_PAYLOAD_SIZE);
    offset += MAX_PAYLOAD_SIZE;
    memcpy(&response->status, responseBuffer + offset, sizeof(enum Status));
}

void writeResponseToFifo(int fifoFd, struct Response response) {
    int responseByteSize = getResponseSize();
    char responseBuffer[responseByteSize];
    memset(responseBuffer, 0, responseByteSize);
    int offset = 0;

    // Copy the data of the struct into the buffer
    memcpy(responseBuffer + offset, response.payload, MAX_PAYLOAD_SIZE);
    offset += MAX_PAYLOAD_SIZE;
    memcpy(responseBuffer + offset, &response.status, sizeof(enum Status));

    // Write the buffer to the FIFO
    ssize_t bytesWritten = 0;
    NO_EINTR(bytesWritten = write(fifoFd, responseBuffer, responseByteSize));
    if (bytesWritten == -1) {
        errExit("write");
    }
}


void createFifoIfNotExist(const char* fifoName) {
    if (unlink(fifoName) == -1 && errno != ENOENT) {
        errExit("unlink");
    }
    if (mkfifo(fifoName, 0666) == -1) {
        errExit("mkfifo");
    }
}

int getRequestSize() {
    return sizeof(int) + sizeof(enum CommandType) + MAX_ARG_SIZE;
}

int getResponseSize() {
    return MAX_PAYLOAD_SIZE + sizeof(enum Status);
}

void forwardRequestToChild(int childPipeWriteEndFd, struct Request request) {
    int bytesWritten = -1;
    NO_EINTR(bytesWritten = write(childPipeWriteEndFd, &request, sizeof(struct Request)));
    if (bytesWritten == -1) {
        errExit("write");
    }
}

void readForwardedRequestFromServer(int childPipeReadEndFd, struct Request* request) {
    int bytesRead = -1;
    NO_EINTR(bytesRead = read(childPipeReadEndFd, request, sizeof(struct Request)));
    if (bytesRead == -1) {
        errExit("read");
    }
}

void sendErrorResponse(int responseFifoFd, const char* errorMessage) {
    struct Response response;
    memset(&response, 0, sizeof(struct Response));
    response.status = ERROR;
    strncpy(response.payload, errorMessage, MAX_PAYLOAD_SIZE);
    writeResponseToFifo(responseFifoFd, response);
}

int transferFile(const char* filepath, const char* fileTransferFifoName) {
    // Open FIFO for writing
    int fifoFd = open(fileTransferFifoName, O_WRONLY);
    if (fifoFd == -1) {
        perror("open FIFO for writing");
        return -1;
    }

    // Open file for reading
    FILE *file = fopen(filepath, "rb");
    if (file == NULL) {
        perror("open file for reading");
        close(fifoFd);
        return -1;
    }

    // Transfer file data
    ssize_t totalBytesTransferred = 0;
    char buffer[CHUNK_SIZE];
    ssize_t bytesRead;
    while ((bytesRead = fread(buffer, 1, sizeof(buffer), file)) > 0) {
        ssize_t bytesWritten = write(fifoFd, buffer, bytesRead);
        if (bytesWritten == -1) {
            perror("write to FIFO");
            fclose(file);
            close(fifoFd);
            return -1;
        }
        totalBytesTransferred += bytesWritten;
    }
    // Check for errors or end-of-file
    if (ferror(file)) {
        perror("read from file");
        fclose(file);
        close(fifoFd);
        return -1;
    }

    // Close file and FIFO
    fclose(file);
    close(fifoFd);

    // Return total bytes transferred
    return totalBytesTransferred;
}

int receiveFile(const char* filepath, const char* fileTransferFifoName) {
    int totalBytesRead = 0;

    // Open FIFO for reading
    int fifoFd = open(fileTransferFifoName, O_RDONLY);
    if (fifoFd == -1) {
        perror("open FIFO for reading");
        return -1;
    }

    // Create or open file for writing
    FILE *file = fopen(filepath, "wb");
    if (file == NULL) {
        perror("open file for writing");
        close(fifoFd);
        return -1;
    }

    mode_t fileMode = S_IRUSR | S_IWUSR | S_IXUSR | S_IRGRP | S_IWGRP | S_IXGRP | S_IROTH | S_IWOTH | S_IXOTH;
    if (chmod(filepath, fileMode) == -1) {
        perror("set file permissions of received file");
        fclose(file);
        close(fifoFd);
        return -1;
    }

    // Receive file data
    char buffer[CHUNK_SIZE];
    ssize_t bytesRead;
    while ((bytesRead = read(fifoFd, buffer, sizeof(buffer))) > 0) {
        ssize_t bytesWritten = fwrite(buffer, 1, bytesRead, file);
        if (bytesWritten < bytesRead) {
            perror("write to file");
            fclose(file);
            close(fifoFd);
            return -1;
        }
        totalBytesRead += bytesRead;
    }

    // Check for errors or end-of-file
    if (bytesRead == -1) {
        perror("read from FIFO");
        fclose(file);
        close(fifoFd);
        return -1;
    }

    // If transfer was cancelled, delete the file
    if (totalBytesRead == 0) {
        if (unlink(filepath) == -1) {
            errExit("unlink for cancelled transfer");
        }
    }

    // Close file and FIFO
    fclose(file);
    close(fifoFd);

    return totalBytesRead;
}

void cancelTransfer(const char* fileTransferFifoName) {
    int fd = open(fileTransferFifoName, O_WRONLY);
    if (fd == -1) {
        perror("open FIFO for writing");
        return;
    }
    if (close(fd) == -1) {
        perror("close FIFO");
        return;
    }
}