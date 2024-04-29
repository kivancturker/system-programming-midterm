#ifndef COMMAND_HANDLER_H
#define COMMAND_HANDLER_H

#include "mytypes.h"

void handleCommand(struct Request request, int responseFifoFd);
void handleConnectCommand(struct Request request);
void handleTryConnectCommand(struct Request request);

#endif // COMMAND_HANDLER_H