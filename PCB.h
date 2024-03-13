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
    char * proc_message;
};

typedef struct Semaphore_s Semaphore;
struct Semaphore_s {
    int value;
    List* waitingProcesses;
};

static void freeNode(void *pItem);
void process_init();
void shutDown();
static int createProcess(int priority);


#endif