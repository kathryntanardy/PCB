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

static currentPID; // 0 for init process

static void freeNode(void *pItem){
    free(pItem);
}

void process_init(){

    // ProcessControlBlock initProcess;

    printf("Initiating processes.. \n");
    readyPriority0 = List_create();
    readyPriority1 = List_create();
    readyPriority2 = List_create();
    waitForReply = List_create();
    waitForReceive = List_create();

    printf("Processing init process\n");
    while(1){
        printf("Enter command:  \n");
        char input;
        scanf("%c", &input);
        
        switch(input){
            case 'C':

                break;
            case 'F':

                break;
            case 'K':

                break;
            case 'E':

                break;
            case 'Q':

                break;
            case 'S':

                break;
            case 'R':

                break;
            case 'Y':

                break;
            case 'N':

                break;
            case 'P':

                break;
            case 'V':

                break;
            case 'I':

                break;
            case 'T':

                break;
            default:
                printf("Command not recognized\n");
        }

        break;
    }

    return;
}

void shutDown(){
    printf("Shutting down all Processes...\n");
    
    List_free(readyPriority0, freeNode);
    List_free(readyPriority1, freeNode);
    List_free(readyPriority2, freeNode);
    List_free(waitForReply, freeNode);
    List_free(waitForReceive, freeNode);
    return;
}
