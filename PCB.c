#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "list.h"


List* readyPriority0;
List* readyPriority1;
List* readyPriority2;
List* waitForReply(); //waiting on a send operation
List* waitForReceive();


