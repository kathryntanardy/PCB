#ifndef PROCESSCB
#define PROCESSCB

#include <stdio.h>
#include <string.h>

enum processState {
    RUNNING,
    READY,
    BLOCKED
};

struct processControlBlock {
    int pid;
    int priority;
    enum processState pcbState;
    char * message;
};


void init();
void shutDown();


#endif