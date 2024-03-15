#include <stdio.h>
#include <string.h>
#include <stdbool.h>
#include <stdlib.h>
#include "list.h"
#include "PCB.h"


 //TO DO: implement everything with semaphores 

List* readyPriority0;
List* readyPriority1;
List* readyPriority2;
List* waitForReply; // waiting on a send operation
List* waitForReceive;

static ProcessControlBlock* initProcess;
static ProcessControlBlock* currentProcess;

static int pidAvailable;

static void freePCB(void *pItem){
    //needs to be modified as we still have to free proc_message
    free(pItem);
}

static int processComparison(ProcessControlBlock* iter, void * pid){
    int value = *((int*) pid);
    if(iter->pid == value){
        return 1;
    }

    return 0;
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
    newPCB->proc_message = NULL;
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

    printf("Process creation success.");
    return ans;
}

//Copy the currently running process and put it on the ready Q 
//corresponding to the original process' priority
//return 0 if fail, return pid of the resulting (new) process
static int fork(){
    //TODO: check whether copying pcb with semaphores impact anything

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
    
    //Copy proc_message based on the state of the Current Process' proc_message
    if(currentProcess->proc_message == NULL){
        newProcess->proc_message = NULL;
    }
    else{
        int length = strlen(currentProcess->proc_message);
        newProcess->proc_message = (char*)malloc((length+1)*sizeof(char));
        strcpy(newProcess->proc_message, currentProcess->proc_message);
    }

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
static void killProcess(int pid){
    //TO DO: current process is the process to be killed case
    if(pid == initProcess->pid){
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
            printf("Init process cannot be killed unless it is the last process in the system");
            return;
        }
    }
    else{
        //case if current Process is the Process to be killed
        if(pid == currentProcess->pid){
             //TODO: check if it has blocked a sender or semaphore 
            exitProcess();
            return;
        }

        int priority = currentProcess->priority;
        bool isRemoved = false;
        ProcessControlBlock * searchResult;
        ProcessControlBlock * removedItem;
        int * pidPointer = (int*)malloc(sizeof(int));
        *pidPointer = pid;
        
        if(priority == 0){
            searchResult = List_search(readyPriority0, processComparison, pidPointer); 
            if(searchResult != NULL){
                //TODO: check if it has blocked a sender or semaphore 
                removedItem = List_remove(readyPriority0);
                free(removedItem);
                isRemoved = true;
            }
        }
        if(priority == 1){
            searchResult = List_search(readyPriority1, processComparison, pidPointer); 
            if(searchResult != NULL){
                //TODO: check if it has blocked a sender or semaphore 
                removedItem = List_remove(readyPriority1);
                free(removedItem);
                isRemoved = true;
            }
        }
        if (priority == 2){
            searchResult = List_search(readyPriority2, processComparison, pidPointer); 
            if(searchResult != NULL){
                //TODO: check if it has blocked a sender or semaphore 
                removedItem = List_remove(readyPriority2);
                free(removedItem);
                isRemoved = true;
            }
        }

        if(List_count(waitForReply) != 0){
            searchResult = List_search(waitForReply, processComparison, pidPointer);
            if(searchResult != NULL){
                //TODO: check if it has blocked a sender or semaphore 
                removedItem = List_remove(waitForReply);
                free(removedItem);
                isRemoved = true;
            }
        }
        if(List_count(waitForReceive) != 0){
            searchResult = List_search(waitForReceive, processComparison, pidPointer);
            if(searchResult != NULL){
                //TODO: check if it has blocked a sender or semaphore 
                removedItem = List_remove(waitForReceive);
                free(removedItem);
                isRemoved = true;
            }
        }


        if(isRemoved){
            printf("Succesfully removed Process %d\n", pid);
        } else{
            printf("Kill process not succesfully executed. PID not found.");
        }

        free(pidPointer);
    }

}


//Kill the currently running process
static void exitProcess(){
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
        //remove current Process and look for the next running Process in the ready queue 
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
    pidAvailable = 0;
    initProcess = (ProcessControlBlock*)malloc(sizeof(ProcessControlBlock));
    initProcess->pid = pidAvailable;
    pidAvailable++; //increment for other process PID
    initProcess->priority = 2;
    initProcess->pcbState = RUNNING;
    initProcess->proc_message = NULL;
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
                int priority = -1;
                printf("Enter the priority (0-2, 0 being the highest priority) n for the new PCB: ");
                scanf("%d", &priority);
                if(priority < 0 || priority > 2){
                    printf("Invalid input, enter a number between 0, 1 or 2\n");
                    break;
                }
                createProcess(priority);
                break;
            case 'F':
                fork();
                break;
            case 'K':
                int pid = -1;
                printf("Enter the pid oyou want to remove from the system: ");
                scanf("%d", &pid);

                if(pid > pidAvailable){
                    printf("There's no a process with the typed pid\n");
                    break;
                }

                killProcess(pid);
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

