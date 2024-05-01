#ifndef COMMAND_HANDLER_H
#define COMMAND_HANDLER_H

#include "mytypes.h"
#include "queue.h"

void handleCommand(struct Request request, int responseFifoFd);
void handleConnectCommand(struct Request request, struct Queue *serverQueue);
void handleTryConnectCommand(struct Request request);
void handleHelpCommand(struct Request request, int responseFifoFd);

void handleCommandResponseByCommandType(enum CommandType commandType, struct Response response);
void handleHelpResponse(struct Response response);

#endif // COMMAND_HANDLER_H