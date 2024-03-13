#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "list.h"
#include "PCB.h"


List* readyPriority0;
List* readyPriority1;
List* readyPriority2;
List* waitForReply; // waiting on a send operation
List* waitForReceive;


void init();
void shutDown();
