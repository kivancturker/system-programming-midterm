#include "ipc.h"
#include "myutil.h"
#include "command_handler.h"

#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>

void createUniqueResponseFifoName(char *responseFifoName, int clientPid) {
    sprintf(responseFifoName, RESPONSE_FIFO_TEMPLATE "%d", clientPid);
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
        fprintf(stderr, "Incomplete data read from fifo\n");
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
        fprintf(stderr, "Incomplete data read from fifo\n");
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