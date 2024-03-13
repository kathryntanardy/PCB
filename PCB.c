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

static int currentProcessedPID; // 0 for init process
static int pidAvailable = 0;

static void freeNode(void *pItem){
    //needs to be modified as we still have to free proc_message
    free(pItem);
}

void process_init(){

    ProcessControlBlock* initProcess = (ProcessControlBlock*)malloc(sizeof(ProcessControlBlock));
    initProcess->pid = pidAvailable;
    pidAvailable++; //increment for other process PID
    initProcess->priority = 2;
    initProcess->pcbState = RUNNING;

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

//returns 0 if failed, returns pid of the created process on success
static int createProcess(int priority){
    int ans = 0;

    if(priority < 0 || priority > 2){
        printf("Process creation failed. Invalid priority input.\n");
        return ans;
    }
    ProcessControlBlock* newPCB = (ProcessControlBlock*)malloc(sizeof(ProcessControlBlock));
    newPCB->pid = pidAvailable;
    ans = newPCB->pid;
    pidAvailable++;

    newPCB->priority = priority;
    
    switch(priority){
        case 0:
            List_append(readyPriority0, newPCB);
            break;
        case 1:
            List_append(readyPriority1, newPCB);
            break;
        case 2:
            List_append(readyPriority2, newPCB);
            break;
        default:
            printf("Process creation failed. Invalid priority input.\n");
            return ans;
    }
    
    return ans;
}
