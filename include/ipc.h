#ifndef IPC_H
#define IPC_H

#include "myutil.h"
#include "mytypes.h"


#define REQUEST_FIFO_TEMPLATE "/tmp/request_fifo943953"
#define RESPONSE_FIFO_TEMPLATE "/tmp/response_fifo" // Use template to create unique fifo name
#define FILETRANSFER_FIFO_TEMPLATE "/tmp/transfer_fifo" // Use template to create unique fifo name

void createUniqueRequestFifoName(char *requestFifoName, int clientPid);
void createUniqueResponseFifoName(char *responseFifoName, int clientPid);
void createUniqueFileTransferFifoName(char *fileTransferFifoName, int clientPid);
int readRequestFromFifo(int fifoFd, struct Request* request);
void writeRequestToFifo(int fifoFd, struct Request request);
void readResponseFromFifo(int fifoFd, struct Response* response);
void writeResponseToFifo(int fifoFd, struct Response response);
void createFifoIfNotExist(const char* fifoName);
int getRequestSize();
int getResponseSize();
void forwardRequestToChild(int childPipeWriteEndFd, struct Request request);
void readForwardedRequestFromServer(int childPipeReadEndFd, struct Request* request);
void sendErrorResponse(int responseFifoFd, const char* errorMessage);
int transferFile(const char* filepath, const char* fileTransferFifoName);
int receiveFile(const char* filepath, const char* fileTransferFifoName);
void cancelTransfer(const char* fileTransferFifoName);

#endif // IPC_H