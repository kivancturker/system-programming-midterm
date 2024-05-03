#ifndef COMMAND_HANDLER_H
#define COMMAND_HANDLER_H

#include "mytypes.h"
#include "queue.h"

void handleCommand(struct Request request, int responseFifoFd, const char* serverDir);
void handleConnectCommand(struct Request request, struct Queue *serverQueue, const char* serverDir);
void handleTryConnectCommand(struct Request request);
void handleHelpCommand(struct Request request, int responseFifoFd);
void handleListCommand(struct Request request, int responseFifoFd, const char* serverDir);
void handleReadFCommand(struct Request request, int responseFifoFd, const char* serverDir);
void handleWriteTCommand(struct Request request, int responseFifoFd, const char* serverDir);
int parseWriteTCommandArgs(char* commandArgs, char* filename, int* lineNum, char* stringToWrite, char* errMessage);
void handleUploadCommand(struct Request request, int responseFifoFd, const char* serverDir);
void handleDownloadCommand(struct Request request, int responseFifoFd, const char* serverDir);
void handleQuitCommand(struct Request request, int responseFifoFd, const char* serverDir);

void handleCommandResponseByCommandType(enum CommandType commandType, struct Response response);
void handleErrorResponse(struct Response response);
void handleHelpResponse(struct Response response);
void handleListResponse(struct Response response);
void handleReadFResponse(struct Response response);
void handleWriteTResponse(struct Response response);
void handleUploadResponse(struct Response response);
void handleDownlaodResponse(struct Response response);
void handleQuitResponse(struct Response response);

#endif // COMMAND_HANDLER_H