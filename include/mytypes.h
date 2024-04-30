#ifndef MYTYPES_H
#define MYTYPES_H

#include <unistd.h>

#define MAX_ARG_SIZE 256
#define MAX_PAYLOAD_SIZE 1024
#define MAX_FILENAME_SIZE 256

enum CommandType {
    CONNECT,
    TRYCONNECT,
    LIST,
    HELP,
    QUIT,
    UNKNOWN
};

enum Status {
    OK,
    ON_PROGRESS,
    ERROR
};

struct Request {
    int clientPid;
    enum CommandType commandType;
    char commandArgs[MAX_ARG_SIZE];
};

struct Response {
    char payload[MAX_PAYLOAD_SIZE];
    enum Status status;
};

struct ServerArg{
    char dirname[MAX_FILENAME_SIZE];
    int numOfClients;
};

struct ClientArg {
    char connectionType[MAX_FILENAME_SIZE];
    pid_t serverPid;
};

struct ConnectionRequest {
    pid_t clientPid;
    int clientNum;
    char responseFifoName[MAX_FILENAME_SIZE];
};

struct ConnectionInfo {
    pid_t clientPid;
    pid_t childPid;
    int childNum;
    int pipeFds[2];
};

#endif // MYTYPES_H