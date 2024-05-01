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

void handleCommandResponseByCommandType(enum CommandType commandType, struct Response response);
void handleErrorResponse(struct Response response);
void handleHelpResponse(struct Response response);
void handleListResponse(struct Response response);
void handleReadFResponse(struct Response response);

#endif // COMMAND_HANDLER_H