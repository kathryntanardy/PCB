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

static ProcessControlBlock* initProcess;
static ProcessControlBlock* currentProcess;

static int pidAvailable = 0;

static void freePCB(void *pItem){
    //needs to be modified as we still have to free proc_message
    free(pItem);
}

//returns 0 if failed, returns pid of the created process on success
int createProcess(int priority){
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

    prinf("Process creation success.");
    return ans;
}

//Copy the currently running process and put it on the ready Q 
//corresponding to the original process' priority
//return 0 if fail, return pid of the resulting (new) process
static int fork(){

    //Attempting to Fork the "init" process should fail
    if(currentProcess->pid == 0){
        printf("Unable to fork init process");
        return 0;
    }


    int ret = 0;
    ProcessControlBlock* newProcess = (ProcessControlBlock*)malloc(sizeof(ProcessControlBlock));
    newProcess->pid = pidAvailable;
    pidAvailable++;
    newProcess->priority = currentProcess->priority;
    newProcess->pcbState = READY;
    int priority = newProcess->priority;
    ret = newProcess->pid;
    switch(priority){
        case 0:
            List_append(readyPriority0, newProcess);
            break;
        case 1:
            List_append(readyPriority1, newProcess);
            break;
        case 2:
            List_append(readyPriority2, newProcess);
            break;
        default:
            printf("Process creation failed. Invalid priority input.\n");
            return ret;
    }

    printf("Fork success");
    return ret;
}

//init process cannot be killed or exited unless it is the 
//last process in the system (i.e. no processes on any ready queue or blocked queue)
static void kill(int pid){
    //TO DO: current process is the process to be killed case

}

//Kill the currently running process
static void exit(){
    //initProcess exit
    if(currentProcess->pid == initProcess->pid){

        //If init process is the last process, terminate
        if(List_count(readyPriority0) == 0 &&
        List_count(readyPriority1) == 0 &&
        List_count(readyPriority2) == 0 &&
        List_count(waitForReply) == 0 &&
        List_count(waitForReceive) == 0){
            freePCB(initProcess);
            shutDown();
        }
        else{
            printf("Init process cannot be exited unless it is the last process in the system");
            return;
        }
    }
    else{ //The case where it is not the init process
        if(List_count(readyPriority0) != 0){
            freePCB(currentProcess);
            List_first(readyPriority0);
            currentProcess = List_remove(readyPriority0);
        }
        else if(List_count(readyPriority1) != 0){
            freePCB(currentProcess);
            List_first(readyPriority1);
            currentProcess = List_remove(readyPriority1);
        }
        else if(List_count(readyPriority2) != 0){
            freePCB(currentProcess);
            List_first(readyPriority2);
            currentProcess = List_remove(readyPriority2);
        }
        //TODO: configure what if its blocked in the reply and receive queue

        return;
    }
}


void process_init(){

    initProcess = (ProcessControlBlock*)malloc(sizeof(ProcessControlBlock));
    initProcess->pid = pidAvailable;
    pidAvailable++; //increment for other process PID
    initProcess->priority = 2;
    initProcess->pcbState = RUNNING;
    currentProcess = initProcess;

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
                int priority = 3;
                while(priority < 0 || priority > 2){
                    printf("Enter the priority (0-2, 0 being the highest priority) n for the new PCB: ");
                    scanf("%d", &priority);

                    if(priority < 0 || priority > 2){
                        printf("Invalid input, enter a number between 0, 1 or 2\n");
                    }
                }

                createProcess(priority);
                break;
            case 'F':
                fork();
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
    
    freePCB(initProcess); //TO DO: make sure this is in the correct order, init might still be in ready Queue 
    List_free(readyPriority0, freePCB);
    List_free(readyPriority1, freePCB);
    List_free(readyPriority2, freePCB);
    List_free(waitForReply, freePCB);
    List_free(waitForReceive, freePCB);
    exit(0);
}

