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
List * initQueue;
List *waitForReceive;

static ProcessControlBlock *initProcess;
static ProcessControlBlock *currentProcess;

static Semaphore semaphores [] = {{-1,NULL}, {-1,NULL}, {-1,NULL}, {-1,NULL}, {-1,NULL}};

static int pidAvailable;

#define MSG_MAX_LENGTH 41
static char inputMessage[MSG_MAX_LENGTH];

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
        List_first(initQueue);
        currentProcess = List_remove(initQueue);
    }

    currentProcess->pcbState = RUNNING;
    printf("Process %d is now running.\n", currentProcess->pid);

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
    else if(priority == 3){
        List_append(initQueue, process);
    }
    else{
        printf("Invalid process' priority\n");
    }

    return;
}


void freePCB(void *pItem)
{

    ProcessControlBlock* item = (ProcessControlBlock*)pItem;

    if (item->proc_message != NULL)
    {
        free(item->proc_message);
    }

    if(item->proc_reply != NULL){
        free(item->proc_reply);
    }

    free(pItem);
}

bool processComparison(void *iter, void *pid)
{
    ProcessControlBlock * iterPCB = (ProcessControlBlock*)iter;
    int value = *((int *)pid);
    if (iterPCB->pid == value)
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

    if(currentProcess->pid == initProcess->pid){
        readyProcess(currentProcess);
        changeRunningProcess();
    }

    printf("Process creation success.\n");
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
        printf("Unable to fork init process\n");
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
    newProcess->priority = currentProcess->priority;
    newProcess->pcbState = READY;
    newProcess->proc_message = NULL;
    newProcess->messageFrom = -1;
    newProcess->messageRepliedFrom = -1;
    newProcess->proc_reply = NULL;

    ret = newProcess->pid;
    readyProcess(newProcess);

    printf("Fork success\n");
    return ret;
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
            shutDown();
        }
        else
        {
            printf("Init process cannot be exited unless it is the last process in the system\n");
            return;
        }
    }
    else
    { // The case where it is not the init process
        // remove current Process and look for the next running Process in the ready queue
        printf("Exiting current process with PID %d..\n ",currentProcess->pid);
        freePCB(currentProcess);
        
        changeRunningProcess();
        // TODO: configure what if its blocked in the reply and receive queue

        return;
    }
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

        if (List_count(readyPriority0) != 0)
        {
            List_first(readyPriority0);
            searchResult = List_search(readyPriority0, processComparison, pidPointer);
            if (searchResult != NULL)
            {
                removedItem = List_remove(readyPriority0);
                isRemoved = true;
            }
        }
        if (List_count(readyPriority1) != 0 && !isRemoved)
        {
            List_first(readyPriority1);
            searchResult = List_search(readyPriority1, processComparison, pidPointer);
            if (searchResult != NULL)
            {
                removedItem = List_remove(readyPriority1);
                isRemoved = true;
            }
        }
        if (List_count(readyPriority2) != 0 && !isRemoved)
        {
            List_first(readyPriority2);
            searchResult = List_search(readyPriority2, processComparison, pidPointer);
            if (searchResult != NULL)
            {
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
                removedItem = List_remove(waitForReceive);
                isRemoved = true;
            }
        }

        if (isRemoved)
        {
            free(removedItem);
            printf("Succesfully removed Process %d from the system.\n", pid);
        }
        else
        {
            printf("Kill process not succesfully executed. PID not found.\n");
        }

        free(pidPointer);
    }

    return;
}

static void quantum()
{

    printf("Quantum reached. Switching processes..\n");
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
    List_first(initQueue);
    // Documentation: Processes blocked to wait for replies won't be able to receive any message
    
    
    if (List_search(initQueue, processComparison, receiverPidPointer) != NULL)
    {
        queue = -4;
    }
    else if (List_search(readyPriority0, processComparison, receiverPidPointer) != NULL)
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

    if (queue >= 0 || queue == -2 || queue == -4)
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
        case -4:
            receivingProcess = List_curr(initQueue);
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
        receivingProcess->messageFrom = currentProcess->pid;

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

        printf("No message received. Blocking until it receives one.. \n");
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
        repliedProcess->messageRepliedFrom = currentProcess->pid;
        
        readyProcess(repliedProcess);
       
    }

    free(repliedPidPointer);
    return;
}

static void iterateQueue(List * pcbList){
    int num = List_count(pcbList);
    
    if(num == 0){
        printf("Empty.\n\n");
        return;
    }

    List_first(pcbList);
    ProcessControlBlock * iter;
    for(int i = 0; i < num; i++){
        iter = List_curr(pcbList);
        printf("%d", iter->pid);

        if(i == num - 1){
            printf(".\n");
        }else{
            printf(", ");
        }
        List_next(pcbList);
    }

    printf("\n");
    return;
}


void shutDown()
{
    printf("Shutting down all Processes...\n");
    freePCB(currentProcess);
    List_free(readyPriority0, freePCB);
    List_free(readyPriority1, freePCB);
    List_free(readyPriority2, freePCB);
    List_free(waitForReply, freePCB);
    List_free(waitForReceive, freePCB);
    List_free(initQueue, freePCB);
    for (int i = 0; i < 5; i++)
    {
        if (semaphores[i].waitingProcesses != NULL)
        {
            List_free(semaphores[i].waitingProcesses, freePCB);
        }
    }
    exit(0);
}

int newSemaphore(int semID, int initValue)
{
    if (semID < 0 || semID > 4)
    {
        printf("Invalid semaphore ID. Please enter a number between 0 and 4\n");
        return -1;
    }
    if (initValue < 0)
    {
        printf("Invalid initial value. Please enter a number greater than or equal to 0\n");
        return -1;
    }
    if (semaphores[semID].value != -1)
    {
        printf("Semaphore with ID %d already exists\n", semID);
        return -1;
    }
    semaphores[semID].value = initValue;
    semaphores[semID].waitingProcesses = List_create();
    printf("Semaphore with ID %d and initial value %d created\n", semID, initValue);
    return 0;
}

int semaphoreP(int semID)
{
    if (currentProcess->pid == initProcess->pid)
    {
        printf("Init process cannot be blocked\n");
        return -1;
    }

    if (semID < 0 || semID > 4)
    {
        printf("Invalid semaphore ID. Please enter a number between 0 and 4\n");
        return -1;
    }
    if (semaphores[semID].waitingProcesses == NULL)
    {
        printf("Semaphore with ID %d does not exist\n", semID);
        return -1;
    }
    else
    {   
        semaphores[semID].value--;
        if (semaphores[semID].value < 0)
        {
            printf("Process %d is blocked on semaphore %d\n", currentProcess->pid, semID);
            currentProcess->pcbState = BLOCKED;
            List_append(semaphores[semID].waitingProcesses, currentProcess);
            changeRunningProcess();
            return 0;
        }
        else
        {
            printf("Process is not blocked because Semaphore with ID %d have value %d\n", semID, semaphores[semID].value);
            return 0;
        }
    }
}

int semaphoreV(int semID)
{
    if (semID < 0 || semID > 4)
    {
        printf("Invalid semaphore ID. Please enter a number between 0 and 4\n");
        return -1;
    }
    if (semaphores[semID].waitingProcesses == NULL)
    {
        printf("Semaphore with ID %d does not exist\n", semID);
        return -1;
    }
    if(List_count(semaphores[semID].waitingProcesses) == 0){
        printf("Semaphore with ID %d does not block anything\n", semID);
        return -1;
    }
    
    else
    {
        semaphores[semID].value++;
        if (semaphores[semID].value <= 0)
        {
            
            List_first(semaphores[semID].waitingProcesses);
            ProcessControlBlock *nextProcess = List_remove(semaphores[semID].waitingProcesses);
            printf("Process %d is unblocked on semaphore %d\n", nextProcess->pid, semID);
            nextProcess->pcbState = READY;
            readyProcess(nextProcess);
        }
        else
        {
            printf("Process is not unblocked because Semaphore with ID %d have value %d\n", semID, semaphores[semID].value);
        }
        
        
        return 0;
    
    }
}

//DOCUMENTATION: pcbState
int procinfo (int pid)
{
    if (pid < 0 || pid > pidAvailable)
    {
        printf("Invalid PID. Please enter a number between 0 and %d\n", pidAvailable);
        return -1;
    }
    if (currentProcess->pid == pid)
    {
        printf("Process %d is currently running\n", pid);
        return 0;
    }

    List_first(readyPriority0);
    List_first(readyPriority1);
    List_first(readyPriority2);
    List_first(waitForReceive);
    List_first(waitForReply);
    List_first(initQueue);
    ProcessControlBlock *item;
    item = List_search(readyPriority0, processComparison, &pid);
    if (item != NULL)
    {
        printf("Process %d is in readyPriority0, pcb state is ready\n", pid);
        return 0;
    }
    item = List_search(readyPriority1, processComparison, &pid);
    if (item != NULL)
    {
        printf("Process %d is in readyPriority1, pcb state is ready\n", pid);
        return 0;
    }
    item = List_search(readyPriority2, processComparison, &pid);
    if (item != NULL)
    {
        printf("Process %d is in readyPriority2, pcb state is ready\n", pid);
        return 0;
    }
    item = List_search(waitForReceive, processComparison, &pid);
    if (item != NULL)
    {
        printf("Process %d is in waitForReceive, pcb state is blocked\n", pid);
        return 0;
    }
    item = List_search(waitForReply, processComparison, &pid);
    if (item != NULL)
    {
        printf("Process %d is in waitForReply, pcb state is blocked\n", pid);
        return 0;
    }
    item = List_search(initQueue, processComparison, &pid);
    if (item != NULL)
    {
        printf("Process %d is the init process, pcb state is ready\n", pid);
        return 0;
    }
    for (int i = 0; i < 5; i++)
    {
        if (semaphores[i].value != -1)
        {
            item = List_search(semaphores[i].waitingProcesses, processComparison, &pid);
            if (item != NULL)
            {
                switch (item->pcbState)
                {
                    case BLOCKED:
                    printf("Process %d is waiting on semaphore %d, pcb state is blocked\n", pid, i);
                    break;
                    case READY:
                    printf("Process %d is waiting on semaphore %d, pcb state is ready\n", pid, i);
                    break;
                    case RUNNING:
                    printf("Process %d is waiting on semaphore %d, pcb state is running\n", pid, i);
                    break;
                }
                return 0;
            }
        }
    }
    printf("Process %d does not exist in the system\n", pid);
    return 0;
}




static void totalInfo(){

    printf("======================================\n");
    printf("Currently running Process: %d\n", currentProcess->pid);

    printf("Processes in Ready Priority Queue 0 (high): \n");
    iterateQueue(readyPriority0);

    printf("Processes in Ready Priority Queue 1 (mid): \n");
    iterateQueue(readyPriority1);

    printf("Processes in Ready Priority Queue 2 (low): \n");
    iterateQueue(readyPriority2);

    printf("The init process queue: \n");
    iterateQueue(initQueue);

    printf("Processes blocked writing for reply: \n");
    iterateQueue(waitForReply);

    printf("Processes blocked writing for receive: \n");
    iterateQueue(waitForReceive);

    for (int i = 0; i < 5; i++)
    {
        if (semaphores[i].waitingProcesses != NULL)
        {
            printf("Processes waiting on semaphore %d: \n", i);
            iterateQueue(semaphores[i].waitingProcesses);
        }
    }
    printf("======================================\n");
}

void process_init()
{
    pidAvailable = 0;
    initProcess = (ProcessControlBlock *)malloc(sizeof(ProcessControlBlock));
    initProcess->pid = pidAvailable;
    pidAvailable++; // increment for other process PID
    initProcess->priority = 3;
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
    initQueue = List_create();

    printf("Processing init process\n");
    while (1)
    {
        printf("Enter command:  \n");
        char input;
        scanf(" %c", &input);

        switch (input)
        {
        case 'C':
            int priority = -1;
            printf("Enter the priority (0-2, 0 being the highest priority) n for the new PCB: \n");
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
            printf("Enter the pid you want to remove from the system: \n");
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
            printf("Enter the PID you want to send the message to: \n");
            scanf("%d", &pidReceiving);

            if (pidReceiving > pidAvailable)
            {
                printf("There's no a process with such  pid\n");
                break;
            }

            int c;
            while((c = getchar()) != '\n' && c != EOF);

            printf("Please enter message (Max 40 chars): \n");
            fgets(inputMessage, MSG_MAX_LENGTH, stdin);
            inputMessage[MSG_MAX_LENGTH - 1] = '\0';
            sendMessage(pidReceiving, inputMessage);
            break;
        case 'R':
            receiveMessage();
            break;
        case 'Y':
            int pidReplied;
            printf("Enter the PID you want to reply a message to: \n");
            scanf("%d", &pidReplied);

            if (pidReplied > pidAvailable)
            {
                printf("There's no a process with such  pid\n");
                break;
            }

            int a;
            while((a = getchar()) != '\n' && a!= EOF);
            printf("Please enter reply (Max 40 chars): \n");
            fgets(inputMessage, MSG_MAX_LENGTH, stdin);
            inputMessage[MSG_MAX_LENGTH - 1] = '\0';
            reply(pidReplied, inputMessage);
            break;
        case 'N':
            int semID, initValue;
            printf("Enter the semaphore ID:\n ");
            scanf("%d", &semID);
            printf("Enter the initial value of the semaphore %d:\n ", semID);
            scanf("%d", &initValue);
            newSemaphore(semID, initValue);
            break;
        case 'P':
            int semIDP;
            printf("Enter the semaphore ID you want to P: \n");
            scanf("%d", &semIDP);
            semaphoreP(semIDP);

            break;
        case 'V':
            int semIDV;
            printf("Enter the semaphore ID you want to V: \n");
            scanf("%d", &semIDV);
            semaphoreV(semIDV);

            break;
        case 'I':
            int pidInfo;
            printf("Enter the PID you want to get info about: \n");
            scanf("%d", &pidInfo);
            procinfo(pidInfo);
            break;
        case 'T':
            totalInfo();
            break;
        case '!':
            shutDown();
            return;
        default:
            printf("Command not recognized\n");
            break;
        }

        printf("\n");
    }

    return;
}
