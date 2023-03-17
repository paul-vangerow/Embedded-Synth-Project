An identification of all the tasks that are performed by the system with their method of implementation, thread or interrupt:
    Key scanner "scanKeysTask": thread, to read and interpret all inputs
    Display updater "displayUpdateTask": thread, to update display with current state
    Speaker "soundISR": interrupt, to play current pressed notes with configured settings
    CAN TxRx: task & interrupt, to monitor and update TxRx queues

A characterisation of each task with its theoretical minimum initiation interval and measured maximum execution time
    The minimum initiation interval, in this case, might be the total number of clock cycles required/time taken

A critical instant analysis of the rate monotonic scheduler, showing that all deadlines are met under worst-case conditions

A quantification of total CPU utilisation

An identification of all the shared data structures and the methods used to guarantee safe access and synchronisation

An analysis of inter-task blocking dependencies that shows any possibility of deadlock