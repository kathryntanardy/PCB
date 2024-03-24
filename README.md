## Code Documentation and System Conventions 🍀
<br>



### Some Introduction 🍯
#### Init Process Handling
Init cannot be killed or exited unless it is the last process in the system. 
Send and Receive will also not block the init process.
Enter "!" to finish the whole process.

#### Multiple Send(s) to the same Process
Our system will handle this by not allowing a process to receive multiple message at once. 
It will reject the user the input by showing a feedback to the screen. 

#### Message Reply and Receives are handled differently
Reply ≠ Receive. A process might receive a message from a Send command or receive a message from a Reply command concurrently. 


### Commands 🍵

