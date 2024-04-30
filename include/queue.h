#ifndef QUEUE_H
#define QUEUE_H

#include "mytypes.h"

#define MAX_QUEUE_SIZE 255

struct Queue {
    int frontIndex;
    int rearIndex;
    int size;
    struct ConnectionRequest connectionRequest[MAX_QUEUE_SIZE];
};

int initQueue(struct Queue* queue);
int enqueue(struct Queue* queue, struct ConnectionRequest connectionRequest);
int dequeue(struct Queue* queue, struct ConnectionRequest* connectionRequest);
int isQueueEmpty(struct Queue* queue);

#endif // QUEUE_H