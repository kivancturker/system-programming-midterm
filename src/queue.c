#include "queue.h"

// Initialize the queue
int initQueue(struct Queue* queue) {
    if (queue == NULL) {
        return -1; // Invalid queue pointer
    }
    queue->frontIndex = 0;
    queue->rearIndex = 0;
    queue->size = 0;
    return 0; // Queue initialized successfully
}

// Enqueue a connection request into the queue
int enqueue(struct Queue* queue, struct ConnectionRequest connectionRequest) {
    if (queue == NULL) {
        return -1; // Invalid queue pointer
    }
    if (queue->size >= MAX_QUEUE_SIZE) {
        return -1; // Queue is full
    }
    queue->connectionRequest[queue->rearIndex] = connectionRequest;
    queue->rearIndex = (queue->rearIndex + 1) % MAX_QUEUE_SIZE;
    queue->size++;
    return 0; // Enqueue operation successful
}

// Dequeue a connection request from the queue
int dequeue(struct Queue* queue, struct ConnectionRequest* connectionRequest) {
    if (queue == NULL || connectionRequest == NULL) {
        return -1; // Invalid queue or connectionRequest pointer
    }
    if (queue->size <= 0) {
        return -1; // Queue is empty
    }
    *connectionRequest = queue->connectionRequest[queue->frontIndex];
    queue->frontIndex = (queue->frontIndex + 1) % MAX_QUEUE_SIZE;
    queue->size--;
    return 0; // Dequeue operation successful
}

// Check if the queue is empty
int isQueueEmpty(struct Queue* queue) {
    if (queue == NULL) {
        return -1; // Invalid queue pointer
    }
    return (queue->size == 0);
}
