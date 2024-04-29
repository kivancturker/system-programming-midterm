#ifndef MYTYPES_H
#define MYTYPES_H

#include <unistd.h>

#define MAX_ARG_SIZE 256
#define MAX_PAYLOAD_SIZE 1024
#define MAX_FILENAME_SIZE 256

enum CommandType {
    CONNECT,
    TRYCONNECT,
    LIST
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


#endif // MYTYPES_H