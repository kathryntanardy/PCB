## Code Documentation and System Conventions 🍀
<br>



### Some Introduction 🍯
make and type in ./output to start the system. <br/>
Enter "!" to finish the whole process.<br/>

#### Init Process Handling
Init cannot be killed or exited unless it is the last process in the system. <br/>
Send and Receive will also not block the init process.<br/>
Init process will automatically be replaced once another process is created (Calling the 'C' command will trigger this)

#### Multiple Send(s) to the same Process
Our system will handle this by not allowing a process to receive multiple message at once. 
It will reject the user the input by showing a feedback to the screen. 

#### Message Reply and Receives are handled differently
Reply ≠ Receive. A process might receive a message from a Send command or receive a message from a Reply command concurrently. 

#### Blocked Process Handling
1. Messages waiting for reply (after sending a message) would not be able to receive a message.
2. Processes blocked by the handling of Semaphores won't be able to receive message. 

### Commands 🍵

