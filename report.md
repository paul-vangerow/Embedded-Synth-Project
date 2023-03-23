# Embedded Systems Synthesiser Coursework Report

- Small Introduction

Part 2 for our embedded systems coursework involved writing the embedded software for a music synthesiser. The project's objective is to create and put into use software that can control the synthesiser and generate various sounds and effects. As well as learning about embedded systems and Platform.io, this project also also helped us gain a string knowledge of software engineering principles.

## Features

A list of all features + descriptions. Here is a link to a video which demonstrates all of our implemented features.

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

Display
- What kind of Method (ISR / Thread)
- What it does
- Why this method of implementation
- Maximum Execution Time (Deadline Analysis)
- Minimum theoretical initiation interval
- Does it meet the deadline?

| Task | Thread | Interrupt | Min initiation interval (theoretical) | Max execution time (microseconds) for 1 iteration|
| ----------------------------- | -------------- | ------------- | ----------------- | -- |
|Speaker  |&check;|  |  | 8.125 | 
|Display  | |  |  | 5301 |
| KeyScanning with Handshake |  | &check; |  | 321810 |
| KeyScanning without Handshake | &cross; |  |  | 131.9 |
| CAN_RX| &cross; |  |  | 54112 |
| CAN_TX| &cross; |  |  | 884.25 |

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

Critical instant - occurs whenever the task is requested simultaneously with requests for all higher priority tasks

Monotonic rate scheduling - If the process has a small job duration, then it has the highest priority. Thus if a process with highest priority starts execution, it will preempt the other running processes. The priority of a process is inversely proportional to the period it will run for.

A quantification of total CPU utilisation

An identification of all the shared data structures and the methods used to guarantee safe access and synchronisation
Display:
localNotes: atomic access, uint16_t storing currently pressed notes on this keyboard only
Speaker:
stepsActive0/stepsActive32: atomic access, uint32_t
volume/shape/octave: atomic access, int32_t

An analysis of inter-task blocking dependencies that shows any possibility of deadlock

# Synthesiser Features

## Basic Features

## Advanced Features

To improve the functionality of the Synthesiser, we added an array of more Advanced features including:

- [Mute Button](#Mute-Button)
- [Different Output Waveforms](#Different-Output-Waveforms)
- [Polyphony](#Polyphony)
- [CAN Auto Detect](#CAN-Auto-Detection)

## Mute Button -- REWORD

A mute button is a button that allows a user to mute or silence an audio signal. In order to create a mute button in C++.

When the button is pressed, the input pin will read a HIGH signal. We can then use a digital output pin to control the audio signal. When the button is pressed, the digital output pin will be set to LOW, which will silence the audio signal. When the button is released, the digital output pin will be set to HIGH, which will allow the audio signal to play.

Debouncing is the process of filtering out false signals that can occur when a button is pressed or released. When a button is pressed or released, it can cause a rapid fluctuation in the input signal, which can be interpreted as multiple button presses or releases. Debouncing helps to filter out these false signals, ensuring that the button press or release is registered accurately.

I used a software debounce algorithm. This involves using a timer to delay the processing of the input signal for a short period of time, typically a few milliseconds. This allows any false signals to settle down, ensuring that only a single button press or release is registered.

## Different Output Waveforms

### Description

Another feature implemented is the ability to switch between a number of output waveforms. These can be cycled through using the rotary nob second from the right, with the selected value displayed as the bottom left number on the screen. Available waveforms are as follows:

- Sawtooth (Option 0)
- Sine Wave (Option 1)
- Square Wave (Option 2)

### How It's Implemented

This feature was implemented in a number of steps:

1. The Volume Nob change detection was standardised into a generic function that can apply to any of the nobs, and change their values to any specified value range. This was then used to control a 'Shape' variable stored in the Speaker class by modifying the value through an Atomic Load and Store whenever the nob's state was changed.
   
2. The speaker ISR was modified to use the shape in a Switch-Case statement to determine how the phase accumulator values would be used. For the basic sawtooth, the value would be used as is, giving us the standard behaviour expected. 


## Polyphony

Feature:
- Description
- What it does
- How it's implemented
- Image/Video of feature


## Different Output Waveforms

## CAN Auto Detection

