# Embedded Systems Synthesiser Coursework Report

Part 2 for our embedded systems coursework involved writing the embedded software for a music synthesiser. The project's objective is to create and put into use software that can control the synthesiser and generate various sounds and effects. As well as learning about embedded systems and Platform.io, this project also also helped us gain a strong knowledge of software engineering principles.

# Synthesiser Features

## Basic Features

All the core features for the synthesiser were implemented. Including:

- Basic Sawtooth Notes
  - Can be played to create sound
- Multi Board CAN communication
  - Can be used to chain multiple boards together and increase the size of the Synthesiser
- Volume Control
  - Divides up the value of the output to reduce or increase volume.
- Display
  - Shows current note being played as well as values for several control settings.


### Board Control Scheme

The basic control scheme for the synthesiser.

| Name | Property | Control Nob Location | Control Type | Display Location |
| --- | --- | --- | --- | --- |
| Volume Nob | Volume | ○ ○ ○ ● | Rotary Dial | Top Left |
| Octave Nob | Octave | ○ ● ○ ○ | Rotary Dial | Top Right | 
| Shape Nob | Waveform | ○ ○ ● ○ | Rotary Dial | Bottom Left |
| Mute Button | Volume | ○ ○ ○ ● | Button | Top Left |


## Advanced Features

To improve the functionality of the Synthesiser, we added an array of more Advanced features including:

- [Mute Button](#Mute-Button)
- [Different Output Waveforms](#Different-Output-Waveforms)
- [Polyphony](#Polyphony)
- [CAN Auto Detect](#CAN-Auto-Detection)

# Mute Button -- REWORD

A mute button is a button that allows a user to mute or silence an audio signal. In order to create a mute button in C++.

When the button is pressed, the input pin will read a HIGH signal. We can then use a digital output pin to control the audio signal. When the button is pressed, the digital output pin will be set to LOW, which will silence the audio signal. When the button is released, the digital output pin will be set to HIGH, which will allow the audio signal to play.

Debouncing is the process of filtering out false signals that can occur when a button is pressed or released. When a button is pressed or released, it can cause a rapid fluctuation in the input signal, which can be interpreted as multiple button presses or releases. Debouncing helps to filter out these false signals, ensuring that the button press or release is registered accurately.

I used a software debounce algorithm. This involves using a timer to delay the processing of the input signal for a short period of time, typically a few milliseconds. This allows any false signals to settle down, ensuring that only a single button press or release is registered.

# Different Output Waveforms

## Description

Another feature implemented is the ability to switch between a number of output waveforms. These can be cycled through using the rotary nob second from the right, with the selected value displayed as the bottom left number on the screen. Available waveforms are as follows:

- Sawtooth (Option 0)
- Sine Wave (Option 1)
- Square Wave (Option 2)

## How It's Implemented

The Volume Nob change detection was standardised into a generic function that can apply to any of the nobs, and change their values to any specified value range. This was then used to control a 'Shape' variable stored in the Speaker class by modifying the value through an Atomic Load and Store whenever the nob's state was changed.
   
The speaker ISR was modified to use the shape in a Switch-Case statement to determine how the phase accumulator values would be used. For the basic sawtooth, the value would be used as is, giving us the standard behaviour expected. The other wave forms transformed the sawtooth through basic mathematical operations / lookup tables to achieve the expected function.

## Demonstration Video

INSERT VIDEO LINK

# Polyphony

## Description

Polyphony is the ability to play multiple notes at once, enabling the ability to play chords and more complicated harmonies. Polyphony is always enabled and works with all connected keys, with up to 5 keyboards.

## How It's Implemented

The speaker function was modified to include multiple phase accumulators, one for every key that was to be pressed. Originally this was only 12 accumulators, but after the introduction of CAN this was changed to 60 - enabling the use of 5 boards simultaneously.

Each key was made to write a single bit high or low in a 64 bit integer, based on its positioning relative to the 'Leader' keyboard. This integer is then read in the speaker class, adding the appropriate step size to the corresponding phase accumulator when the bit is high.

During the loop which adds to the accumulators, the accumulators are also used to sum together a 'total voltage' which will represent the combinations of all the keys pressed and will be output to the DAC after being appropriately shifted to 12 bits.

## Demonstration Video

INSERT VIDEO LINK

# CAN Auto Detection

The final, main advanced feature added was the ability for boards to automatically be deteced and for board octaves to be dynamically scaled based on their location in relation to the main board - the designated leader. It allows one to stick boards together, have them be automatically detected and send their key press values to the leader board which will play them.

## How It's Implemented

### General Function

CAN auto detection was implemented by first rearranging the code a bit. The largest change made was in the way key presses were handled. Originally we had a seperate Speaker task which would read values from the KeyScanner using a semaphore. All of this was scrapped in favor of a purely CAN based system.

When the user presses a key, releases one or turns any of the implemented dials, a CAN message is broadcast to the the network with information regarding the nature of the keypress. The board that is the designated leader receives these in its Receive Queue and processes them as individual key inputs. Other boards can also receive these messages but will simply discard them as a leadership status is required for them to be processes. To allow the leader to read it's own key presses - it doesn't transmit them into the CAN network, but instead just adds them to its own receive queue.

Each board contains information on its location relative to the leader board (to be discussed later) and uses this information to encode a octave differential into its key messages. This means that when the leader receives the inputs - it knows how to scale them and activate the correct note without having to do much additional processing (something which would massively slow down the receiver task).

### Handshaking and Auto Detection

Every cycle of the KeyScanner task, all of the implemented input parameters are measured, including those of the East and West detect. When the board detects a change in either it's east or west connections - it changes its leadership status to false, clears its record of other boards, disables the key press messages and evaluates it's new state and what to do about it. If the change results in it no longer being connected to any boards, then it will set itself to its own leader and reinitialise key reading and display operation. If the change results in a different configuration than it was connected to before, it initiates the following handshaking procedure:

1. Broadcast a Leadership reset signal, this will make all the other connected boards also reset their leadership statuses as well as activating their own handshaking procedures.

2. Once every board is ready, handshaking begins:
   
3. If the board is most Westerly (It has no West connection, only east), then the board will transmit an East message on it's CAN bus. This message contains information on the current board ID as well as it's position in the order of boards. Every board that picks up this message adds the board ID to their boards array at the specified index and increments their own board counter to be the received value + 1.

4. After sending out it's east message, the board deactivates it's east facing handshake signals - making the next board over a 'most westerly' board.

5. The process propagates and repeats until the very end is reached, where a 'most easterly' board exists that is not connected to either a west or east board. This board repeats the east message but as a 'Fin' message - indicating to other boards that the handshaking procedure is finished.

6. When the message is sent out / received, every board reactivates their east / west handshake signals and begin their leader selection algorithm, where they check the middle value of the boards array and compare it to their own ID. If the ID's match, the board sets itself to be the leader. If they do not, then it does nothing. After a new leader has been found, the boards reset their screens and KeyScanner enables, allowing for operation to continue. 

## Demonstration Video

INSERT VIDEO LINK

## Analysis

## Tasks Overview

Each Task and ISR:
- What it is 
- What kind of method
- Minimum Initiation (In a table)
- Maximum Execution Time (In a Table)

## Critical Instant Analysis
| Task                          | Max Execution Time (C) /us    | Initiation Interval/Deadline (T) /ms | Ratio C/T  |
| ----------------------------- | ------------------------------| -------------------------------------| -----------|
| Display                       | 5284.125                      | 100                                  | 0.05284125 | 
| KeyScanning with Handshake    | 132.875                       | 20                                   | 0.00664375 |
| CAN_RX                        | 884.25                        | 10                                   | 0.088425   | 
| CAN_TX                        | 884.15625                     | 2                                    | 0.442078125| 

U = Sum of Ratios = 0.589988125
U <= Utilisation Limit = n(2^(1/n) -1) =  0.75682846
where n (= 4) is the number of tasks 

The processes meet their deadlines and can be scheduled using rate monotonic scheduling.

## CPU Utilisation
Measured with RTOS vTaskGetRunTimeStats() with micros() precision

USAGE WHILE IDLE
statsTask       1955428         2%
IDLE            52443250        74%
displayUpdate   12762808        18%
scanKeys        2828623         4%
RXTask          8067            <1%
Tmr Svc         15              <1%
TXTask          109             <1%

WHILE ALL KEYS PRESSED REPEATEDLY AND KNOBS TURNED
statsTask       427536          2%
IDLE            13481899        67%
displayUpdate   4141945         20%
scanKeys        1942962         9%
RXTask          10425           <1%
Tmr Svc         15              <1%
TXTask          109             <1%

Much more time is spent IDLE than on any task. This shows that our CPU usage is well within reasonable limits.

## Shared Dependencies

- Inter task blocking
- How things are kept Thread safe 

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