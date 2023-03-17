# Embedded Systems Synthesiser Coursework Report

Part 2 for our embedded systems coursework involved writing the embedded software for a music synthesiser. The project's objective is to create and put into use software that can control the synthesiser and generate various sounds and effects. As well as learning about embedded systems and Platform.io, this project also also helped us gain a string knowledge of software engineering principles.

## Features

A list of all features + descriptions + videos for them

#### Basic Features

- Sawtooth Notes
- Volume Control
- Display Note and Volume Display
- Synthesiser Multi-board Operation

#### Advanced Features

- Different Wave Types
- DAC Output
- Auto board detection and dynamic octave scaling
- Polyphony
- Mute Button

## Analysis

CPU Utilisation

Tasks 

- How they are impelemented (ISR / Thread)
- Maximum Exec Time per Task
- Deadline Analysis

Concurrent Programming Stuff

- Data Structures + How they are kept in Synch / Thread Safe
- Inter Task dependency + Blocking Analysis





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
    Display:
        localNotes: atomic access, uint16_t storing currently pressed notes on this keyboard only
    Speaker:
        stepsActive0/stepsActive32: atomic access, uint32_t
        volume/shape/octave: atomic access, int32_t
        
An analysis of inter-task blocking dependencies that shows any possibility of deadlock
