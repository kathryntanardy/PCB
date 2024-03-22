#include <stdio.h>
#include <string.h>
#include <stdbool.h>
#include <stdlib.h>
#include "list.h"
#include "PCB.h"

// TODO: implement everything with semaphores

List *readyPriority0;
List *readyPriority1;
List *readyPriority2;
List *waitForReply; // waiting on a send operation
List *waitForReceive;

static ProcessControlBlock *initProcess;
static ProcessControlBlock *currentProcess;

static int pidAvailable;

#define MSG_MAX_LENGTH 41
static char * inputMessage[MSG_MAX_LENGTH];

//TODO: procInfo to let know changing process
static void changeRunningProcess(){

    if(List_count(readyPriority0) != 0){
        List_first(readyPriority0);
        currentProcess = List_remove(readyPriority0);
    }else if(List_count(readyPriority1) != 0){
        List_first(readyPriority1);
        currentProcess = List_remove(readyPriority1);
    }else if(List_count(readyPriority2) != 0){
        List_first(readyPriority2);
        currentProcess = List_remove(readyPriority2);
    }else{
        printf("No other ready processes available. Continue executing current Process..\n");
    }

    currentProcess->pcbState = RUNNING;

    if(currentProcess->proc_reply != NULL){
        printf("A reply from process %d: ", currentProcess->messageRepliedFrom);
        printf("%s\n", currentProcess->proc_reply);
        free(currentProcess->proc_reply);
        currentProcess->proc_reply = NULL;
        currentProcess->messageRepliedFrom = -1;
    }
}

static void readyProcess(ProcessControlBlock * process){
    int priority = process->priority;
    process->pcbState = READY;

    if(priority == 0){
        List_append(readyPriority0, process);
    }
    else if (priority == 1){
        List_append(readyPriority1, process);
    }
    else if(priority == 2){
        List_append(readyPriority2, process);
    }
    else{
        printf("Invalid priority input.\n");
    }

    return;
}


static void freePCB(ProcessControlBlock *pItem)
{
    if (pItem->proc_message != NULL)
    {
        free(pItem->proc_message);
    }

    if(pItem->proc_reply != NULL){
        free(pItem->proc_reply);
    }

    free(pItem);
}

static int processComparison(ProcessControlBlock *iter, void *pid)
{
    int value = *((int *)pid);
    if (iter->pid == value)
    {
        return 1;
    }

    return 0;
}



// returns 0 if failed, returns pid of the created process on success
int createProcess(int priority)
{
    int ans = 0;

    if (priority < 0 || priority > 2)
    {
        printf("Process creation failed. Invalid priority input.\n");
        return ans;
    }
    ProcessControlBlock *newPCB = (ProcessControlBlock *)malloc(sizeof(ProcessControlBlock));
    if (newPCB == NULL)
    {
        printf("Process creation failed due to failure of memory allocation for new process.\n");
        return ans;
    }
    newPCB->pid = pidAvailable;
    newPCB->messageFrom = -1;
    newPCB->proc_message = NULL;
    newPCB->pcbState = READY;
    newPCB->messageRepliedFrom = -1;
    newPCB->proc_reply = NULL;
    ans = newPCB->pid;
    pidAvailable++;
    newPCB->priority = priority;

    readyProcess(newPCB);

    printf("Process creation success.");
    return ans;
}



// Copy the currently running process and put it on the ready Q
// corresponding to the original process' priority
// return 0 if fail, return pid of the resulting (new) process
static int fork()
{
    // TODO: check whether copying pcb with semaphores impact anything

    // Attempting to Fork the "init" process should fail
    if (currentProcess->pid == 0)
    {
        printf("Unable to fork init process");
        return 0;
    }

    int ret = 0;
    ProcessControlBlock *newProcess = (ProcessControlBlock *)malloc(sizeof(ProcessControlBlock));
    if (newProcess == NULL)
    {
        printf("Process fork failed due to failure of memory allocation for new process.\n");
        return 0;
    }
    newProcess->pid = pidAvailable;
    pidAvailable++;
    newProcess->messageFrom = currentProcess->messageFrom;
    newProcess->priority = currentProcess->priority;
    newProcess->messageRepliedFrom = currentProcess->messageRepliedFrom;
    newProcess->pcbState = READY;
    newProcess->proc_message = NULL;
    newProcess->messageFrom = -1;
    newProcess->proc_reply = -1;
    newProcess->proc_message = NULL;

    ret = newProcess->pid;
    readyProcess(newProcess);

    printf("Fork success\n");
    return ret;
}

// init process cannot be killed or exited unless it is the
// last process in the system (i.e. no processes on any ready queue or blocked queue)
// TODO: which process now gets control of the CPU
// TODO: handle blocked processes (waiting for reply from the process killed)
static void killProcess(int pid)
{
    // TO DO: current process is the process to be killed case
    if (pid == initProcess->pid)
    {
        // If init process is the last process, terminate
        if (List_count(readyPriority0) == 0 &&
            List_count(readyPriority1) == 0 &&
            List_count(readyPriority2) == 0 &&
            List_count(waitForReply) == 0 &&
            List_count(waitForReceive) == 0)
        {
            freePCB(initProcess);
            shutDown();
        }
        else
        {
            printf("Init process cannot be killed unless it is the last process in the system");
            return;
        }
    }
    else
    {
        // case if current Process is the Process to be killed
        if (pid == currentProcess->pid)
        {
            // TODO: check if it has blocked a sender or semaphore
            exitProcess();
            return;
        }

        bool isRemoved = false;
        ProcessControlBlock *searchResult;
        ProcessControlBlock *removedItem;
        int *pidPointer = (int *)malloc(sizeof(int));
        *pidPointer = pid;

        if (List_Count(readyPriority0) != 0)
        {
            List_first(readyPriority0);
            searchResult = List_search(readyPriority0, processComparison, pidPointer);
            if (searchResult != NULL)
            {
                // TODO: check if it has blocked a sender or semaphore
                removedItem = List_remove(readyPriority0);
                isRemoved = true;
            }
        }
        if (List_Count(readyPriority1) != 0 && !isRemoved)
        {
            List_first(readyPriority1);
            searchResult = List_search(readyPriority1, processComparison, pidPointer);
            if (searchResult != NULL)
            {
                // TODO: check if it has blocked a sender or semaphore
                removedItem = List_remove(readyPriority1);
                isRemoved = true;
            }
        }
        if (List_Count(readyPriority2) != 0 && !isRemoved)
        {
            List_first(readyPriority2);
            searchResult = List_search(readyPriority2, processComparison, pidPointer);
            if (searchResult != NULL)
            {
                // TODO: check if it has blocked a sender or semaphore
                removedItem = List_remove(readyPriority2);
                isRemoved = true;
            }
        }
        if (List_count(waitForReply) != 0 && !isRemoved)
        {
            List_first(waitForReply);
            searchResult = List_search(waitForReply, processComparison, pidPointer);
            if (searchResult != NULL)
            {
                // TODO: check if it has blocked a sender or semaphore
                removedItem = List_remove(waitForReply);
                isRemoved = true;
            }
        }
        if (List_count(waitForReceive) != 0 && !isRemoved)
        {
            List_first(waitForReceive);
            searchResult = List_search(waitForReceive, processComparison, pidPointer);
            if (searchResult != NULL)
            {
                // TODO: check if it has blocked a sender or semaphore
                removedItem = List_remove(waitForReceive);
                isRemoved = true;
            }
        }

        if (isRemoved)
        {
            free(removedItem);
            printf("Succesfully removed Process %d\n", pid);
        }
        else
        {
            printf("Kill process not succesfully executed. PID not found.");
        }

        free(pidPointer);
    }

    return;
}

// Kill the currently running process
// TODO: which process now gets control of the CPU
// TODO: handle blocked processes (waiting for reply from the process killed)
static void exitProcess()
{
    // initProcess exit
    if (currentProcess->pid == initProcess->pid)
    {
        // If init process is the last process, terminate
        if (List_count(readyPriority0) == 0 &&
            List_count(readyPriority1) == 0 &&
            List_count(readyPriority2) == 0 &&
            List_count(waitForReply) == 0 &&
            List_count(waitForReceive) == 0)
        {
            printf("Exiting init process, shutting down system..");
            freePCB(initProcess);
            shutDown();
        }
        else
        {
            printf("Init process cannot be exited unless it is the last process in the system");
            return;
        }
    }
    else
    { // The case where it is not the init process
        // remove current Process and look for the next running Process in the ready queue
        printf("Exiting current process with PID %d\n.. ",currentProcess->pid);
        freePCB(currentProcess);
        
        changeRunningProcess();
        // TODO: configure what if its blocked in the reply and receive queue

        return;
    }
}

static void quantum()
{

    printf("Quantum reached. Switching processes..\n");
    int currentPriority = currentProcess->priority;
    ProcessControlBlock *expiredProcess = currentProcess;

    changeRunningProcess();

    if(currentProcess->pid != expiredProcess->pid){
        expiredProcess->pcbState = READY;
        readyProcess(expiredProcess);
    }
    
    // TODO: give procInfo here after implemented
}

static void sendMessage(int receiverPID, char *message)
{

    if (receiverPID == currentProcess->pid)
    {
        printf("Unable to send message to itself");
        return;
    }

    int *receiverPidPointer = (int*)malloc(sizeof(int));
    *receiverPidPointer = receiverPID;

    int queue = -1;

    //List_first every queue as List_search only search froom the current pointer
    List_first(readyPriority0);
    List_first(readyPriority1);
    List_first(readyPriority2);
    List_first(waitForReceive);
    List_first(waitForReply);
    // Documentation: Processes blocked to wait for replies won't be able to receive any message

    if (List_search(readyPriority0, processComparison, receiverPidPointer) != NULL)
    {
        queue = 0;
    }
    else if (List_search(readyPriority1, processComparison, receiverPidPointer) != NULL)
    {
        queue = 1;
    }
    else if (List_search(readyPriority2, processComparison, receiverPidPointer) != NULL)
    {
        queue = 2;
    }
    else if (List_search(waitForReceive, processComparison, receiverPidPointer) != NULL)
    {
        queue = -2;
    }
    else if (List_search(waitForReply, processComparison, receiverPidPointer) != NULL)
    {
        queue = -3;
    }

    size_t length;
    ProcessControlBlock *receivingProcess;

    if (queue >= 0 || queue == -2)
    {
        switch (queue)
        {
        case 0:
            receivingProcess = List_curr(readyPriority0);
            break;
        case 1:
            receivingProcess = List_curr(readyPriority1);
            break;
        case 2:
            receivingProcess = List_curr(readyPriority2);
            break;
        case -2:
            // Unblock receiving process and move it to the ready queue
            receivingProcess = List_curr(waitForReceive);
            List_remove(waitForReceive);

            readyProcess(receivingProcess);

            break;
        }

        length = strlen(message);
        receivingProcess->proc_message = malloc(length + 1);
        strncpy(receivingProcess->proc_message, message, length);
        receivingProcess->proc_message[length] = '\0';
        receivingProcess->messageRepliedFrom = currentProcess->pid;

        if (currentProcess->pid == initProcess->pid)
        {
            printf("Init process remains running as it can't be blocked");
        }
        else
        {
            currentProcess->pcbState = BLOCKED;
            List_append(waitForReply, currentProcess);

            changeRunningProcess();
            return;
        }
    }
    else
    {
        if (queue == -3)
        {
            printf("Unable to send message as process is only waiting for a reply\n");
            return;
        }
        else{
            printf("PID not found in the system. Failed to send message.\n");
        }
    }

    free(receiverPidPointer);
    return;
}

static void receiveMessage(){
    if(currentProcess->proc_message == NULL && currentProcess->pid == initProcess->pid){
        printf("No message for init process received. Continue running.. ");
    }
    else if(currentProcess->proc_message == NULL){
        currentProcess->pcbState = BLOCKED;
        List_append(waitForReceive, currentProcess);

        changeRunningProcess();
    }
    else if (currentProcess->proc_message != NULL){
        printf("Received message from Process %d: \n", currentProcess->messageFrom);
        printf("%s\n", currentProcess->proc_message);
        free(currentProcess->proc_message);
        currentProcess->messageFrom = -1;
        currentProcess->proc_message = NULL;
    }

    return;
}


void reply(int repliedPID, char * message){
    if (repliedPID == currentProcess->pid)
    {
        printf("Unable to reply message to itself");
        return;
    }

    int *repliedPidPointer = (int*)malloc(sizeof(int));
    *repliedPidPointer = repliedPID;

    ProcessControlBlock * repliedProcess = NULL;
    List_first(waitForReply);
    repliedProcess = List_search(waitForReply, processComparison, repliedPidPointer);
    List_remove(waitForReply);

    if(repliedProcess == NULL){
        printf("No Process %d to reply to.\n", repliedPID);
    }
    else{
        size_t length = strlen(message);
        repliedProcess->proc_reply = malloc(length + 1);
        strncpy(repliedProcess->proc_reply, message, length);
        repliedProcess->proc_reply[length] = '\0';
        repliedProcess->proc_reply = currentProcess->pid;
        
        readyProcess(repliedProcess);
       
    }

    free(repliedPidPointer);
    return;
}


void process_init()
{
    pidAvailable = 0;
    initProcess = (ProcessControlBlock *)malloc(sizeof(ProcessControlBlock));
    initProcess->pid = pidAvailable;
    pidAvailable++; // increment for other process PID
    initProcess->priority = 2;
    initProcess->pcbState = RUNNING;
    initProcess->messageFrom = -1;
    initProcess->proc_message = NULL;
    currentProcess = initProcess;

    printf("Initiating processes.. \n");
    readyPriority0 = List_create();
    readyPriority1 = List_create();
    readyPriority2 = List_create();
    waitForReply = List_create();
    waitForReceive = List_create();

    printf("Processing init process\n");
    while (1)
    {
        printf("Enter command:  \n");
        char input;
        scanf("%c", &input);

        switch (input)
        {
        case 'C':
            int priority = -1;
            printf("Enter the priority (0-2, 0 being the highest priority) n for the new PCB: ");
            scanf("%d", &priority);
            if (priority < 0 || priority > 2)
            {
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
            printf("Enter the pid you want to remove from the system: ");
            scanf("%d", &pid);

            if (pid > pidAvailable)
            {
                printf("There's no a process with such  pid\n");
                break;
            }

            killProcess(pid);
            break;
        case 'E':
            exitProcess();
            break;
        case 'Q':
            quantum();
            break;
        case 'S':
            int pidReceiving;
            printf("Enter the PID you want to send the message to: ");
            scanf("%d", &pidReceiving);

            if (pidReceiving > pidAvailable)
            {
                printf("There's no a process with such  pid\n");
                break;
            }

            printf("\n");
            printf("Please enter message (Max 40 chars): ");
            fgets(inputMessage, MSG_MAX_LENGTH, stdin);
            inputMessage[MSG_MAX_LENGTH - 1] = '\0';
            sendMessage(pidReceiving, inputMessage);
            break;
        case 'R':
            receiveMessage();
            break;
        case 'Y':
            int pidReplied;
            printf("Enter the PID you want to reply a message to: ");
            scanf("%d", &pidReplied);

            if (pidReplied > pidAvailable)
            {
                printf("There's no a process with such  pid\n");
                break;
            }
            printf("\n");
            printf("Please enter reply (Max 40 chars): ");
            fgets(inputMessage, MSG_MAX_LENGTH, stdin);
            inputMessage[MSG_MAX_LENGTH - 1] = '\0';
            sendMessage(pidReplied, inputMessage);
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
            break;
        }
    }

    return;
}

void shutDown()
{
    printf("Shutting down all Processes...\n");

    List_free(readyPriority0, freePCB);
    List_free(readyPriority1, freePCB);
    List_free(readyPriority2, freePCB);
    List_free(waitForReply, freePCB);
    List_free(waitForReceive, freePCB);
    exit(0);
}
