#include <stdio.h>

#include "myutil.h"

int main(int argc, char *argv[]) {
    struct ClientArg clientArg;
    parseClientArgs(argc, argv, &clientArg);
    enum ConnectionType connectionType = getConnectionType(clientArg.connectionType); // Later put this on the request

    return 0;
}