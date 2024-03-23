#ifndef PROCESSCB
#define PROCESSCB

#include <stdio.h>
#include <string.h>
#include "list.h"

enum ProcessState {
    RUNNING,
    READY,
    BLOCKED,
};

typedef struct ProcessControlBlock_s ProcessControlBlock;
struct ProcessControlBlock_s {
    int pid;
    int priority;
    enum ProcessState pcbState;
    int messageFrom; //contains PID of the sender
    char * proc_message;
    int messageRepliedFrom;
    char * proc_reply;
};

typedef struct Semaphore_s Semaphore;
struct Semaphore_s {
    int value;
    List* waitingProcesses;
};


void freePCB(void *pItem);
bool processComparison(void *iter, void *pid);
void process_init();
void shutDown();



#endif