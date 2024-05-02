#ifndef IPC_H
#define IPC_H

#include "myutil.h"
#include "mytypes.h"


#define REQUEST_FIFO_NAME "/tmp/request_fifo943953"
#define RESPONSE_FIFO_TEMPLATE "/tmp/response_fifo" // Use template to create unique fifo name

void createUniqueResponseFifoName(char *responseFifoName, int clientPid);
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

#endif // IPC_H