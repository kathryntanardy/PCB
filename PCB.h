#ifndef PROCESSCB
#define PROCESSCB

#include <stdio.h>
#include <string.h>
#include "list.h"

enum ProcessState {
    RUNNING,
    READY,
    BLOCKED
};

typedef struct ProcessControlBlock_s ProcessControlBlock;
struct ProcessControlBlock_s {
    int pid;
    int priority;
    enum ProcessState pcbState;
    char * message;
};

typedef struct Semaphore_s Semaphore;
struct Semaphore_s {
    int value;
    List* waitingProcesses;
};


void init();
void shutDown();


#endif