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

# Mute Button

## Description

The mute button allows a user to mute or silence an audio signal, and then unmute, returning to the previous volume.

## How It's Implemented

When the button is pressed, the input pin will read a HIGH signal. We can then use a digital output pin to control the audio signal. When the button is pressed, the digital output pin will be set to LOW, which will silence the audio signal. When the button is released, the digital output pin will be set to HIGH, which will allow the audio signal to play.

Debouncing is the process of filtering out false signals that can occur when a button is pressed or released. When a button is pressed or released, it can cause a rapid fluctuation in the input signal, which can be interpreted as multiple button presses or releases. Debouncing helps to filter out these false signals, ensuring that the button press or release is registered accurately.

We used a software debounce algorithm. This involves using a timer to delay the processing of the input signal for a short period of time, typically a few milliseconds. This allows any false signals to settle down, ensuring that only a single button press or release is registered.

## Demonstration Video

https://user-images.githubusercontent.com/59746049/227383712-b8c6c74f-915b-4a59-b3dc-8cdd9af34075.mp4

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

https://user-images.githubusercontent.com/59746049/227383796-2a96231c-7dc4-45ef-8f43-c731b9d578e4.mp4

# Polyphony

## Description

Polyphony is the ability to play multiple notes at once, enabling the ability to play chords and more complicated harmonies. Polyphony is always enabled and works with all connected keys, with up to 5 keyboards.

## How It's Implemented

The speaker function was modified to include multiple phase accumulators, one for every key that was to be pressed. Originally this was only 12 accumulators, but after the introduction of CAN this was changed to 60 - enabling the use of 5 boards simultaneously.

Each key was made to write a single bit high or low in a 64 bit integer, based on its positioning relative to the 'Leader' keyboard. This integer is then read in the speaker class, adding the appropriate step size to the corresponding phase accumulator when the bit is high.

During the loop which adds to the accumulators, the accumulators are also used to sum together a 'total voltage' which will represent the combinations of all the keys pressed and will be output to the DAC after being appropriately shifted to 12 bits.

## Demonstration Video

https://user-images.githubusercontent.com/59746049/227383833-22b7f5ff-3641-4768-bb61-938833e30829.mp4

# CAN Auto Detection

The final, main advanced feature added was the ability for boards to automatically be deteced and for board octaves to be dynamically scaled based on their location in relation to the main board - the designated leader. It allows one to stick multiple boards together, have them be automatically detected and send their key press values to the leader board which will play them. Up to 5 boards can be connected up together: when connecting new boards, ideally connect one at a time.

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

Video is quite large, so it can be found within the repository at: vid/autoCAN.mp4

# Analysis

## Tasks Overview

For the implementation of our Synthesiser we used multiple concurrent tasks with the following properties:

| Task Name      | Task Type | Purpose                                 | Theoretical Minimum Initiation /μs | Maximum Execution /μs |
| ---            | ---       | ---                                                                      | ---  | ---  |
| Key Scanner    | Thread    | Reading key presses and transmitting them as CAN Messages.               | 100 | 132.88 |
| Display Update | Thread    | Taking in information from other tasks and displaying it.                | 20 | 5284.13 |
| CAN TX         | Thread    | Sending out items from the CAN Out Queue                                 | 10 | 884.16 |
| CAN RX         | Thread    | Interpreting and processing items from the CAN In Queue                  | 2 | 884.25 |
| Speaker        | Interrupt | Sending voltage values to the DAC based on the desired sound to be played| 45.5 | 8.43 |
| CAN_TX_ISR     | Interrupt | Free mutexes when CAN outboxes are available                             | N/A  | N/A  |
| CAN_RX_ISR     | Interrupt | Enqueue recieved CAN messages into the IN Queue                          | N/A  | N/A  |

## Critical Instant Analysis
| Task                          | Max Execution Time (C) /μs    | Initiation Interval/Deadline (T) /ms | Ratio C/T  |
| ----------------------------- | ------------------------------| -------------------------------------| -----------|
| Display                       | 5284.125                      | 100                                  | 0.05284125 | 
| KeyScanning                   | 132.875                       | 20                                   | 0.00664375 |
| CAN_RX                        | 884.25                        | 10                                   | 0.088425   | 
| CAN_TX                        | 884.15625                     | 2                                    | 0.442078125| 

U = Sum of Ratios = 0.589988125
U <= Utilisation Limit = $n(2^{ \frac{1}{n}} - 1)$ =  0.75682846
where n (= 4) is the number of tasks 

The processes meet their deadlines and can be scheduled using rate monotonic scheduling.

## CPU Utilisation
Measured with RTOS vTaskGetRunTimeStats() with micros() precision

USAGE WHILE IDLE
| Task          | Total Execution Time /μs  | Percentage Utilisation /%|
| --------------| --------------------------| -------------------------|
|statsTask      |  1955428                  | 2                        |
|IDLE           |  52443250                 | 74                       |
|displayUpdate  |  12762808                 | 18                       |
|scanKeys       |  2828623                  | 4                        |
|RXTask         |  8067                     | <1                       |
|Tmr Svc        |  15                       | <1                       |
|TXTask         |  109                      | <1                       |

WHILE ALL KEYS PRESSED REPEATEDLY AND KNOBS TURNED
| Task          | Total Execution Time /μs  | Percentage Utilisation /%|
| --------------| --------------------------| -------------------------|
|statsTask      |  427536                   | 2                        |
|IDLE           |  13481899                 | 67                       |
|displayUpdate  |  4141945                  | 20                       |
|scanKeys       |  1942962                  | 9                        |
|RXTask         |  10425                    | <1                       |
|Tmr Svc        |  15                       | <1                       |
|TXTask         |  109                      | <1                       |

Much more time is spent IDLE than on any task. This shows that our CPU usage is well within reasonable limits.

## Shared Dependencies

### Thread Safety

When working with this many concurrent tasks, the potential issue of race conditions becomes quite prevalent. As we had a number of variables accessed between different threads, we had to ensure there were no concurrent accesses. There were two main ways in which this was done: Semaphores and Atomic accesses. The following is a list of each shared variable, it's purpose and how it has been made thread safe.

| Variable Name     | Purpose                                                         | Thread Safety Implementation |
| ---               | ---                                                             | ---                          |
| isLeader          | Stores the boards leadership status                             | Atomic Memory Access         |
| current_board     | Mid-handshake - which board is next in the board detect array   | Atomic Memory Access         |
| board_detect_array| Stores all of the detected boards                               | Semaphore                    |
| board_ID          | Stores the boards Unique ID                                     | Atomic Memory Access         |
| board_number      | Stores which index of the board array the current board is      | Atomic Memory Access         |
| leader_number     | Stores which index of the board array the leader is             | Atomic Memory Access         |
| number_of_boards  | Stores the total number of detected boards                      | Atomic Memory Access         |
| localNotes        | Used for transmitting keypresses to the Display                 | Atomic Memory Access         |
| OUT_EN            | Used for enabling / disabling the note press detection          | Atomic Memory Access         |
| debounce          | Used for adding a timeout to the mute button                    | Atomic Memory Access         |
| ticks             | Used for measuring the time for the debounce of the mute button | Atomic Memory Access         |
| stepsActive32     | Stores the top 28 note values (1 bit per note )                 | Atomic Memory Access         |
| stepsActive0      | Stores the botton 32 note values ( 1 bit per note )             | Atomic Memory Access         |
| volume            | Stores the current volume of the leader                         | Atomic Memory Access         |
| volume_store      | Used to temporarily store the volume when muted                 | Atomic Memory Access         |
| shape             | Used to store the current shape of the leader                   | Atomic Memory Access         |
| octave            | Used to store the current octave of the leader                  | Atomic Memory Access         |


### Inter Task Blocking

In our system, there are very few cases of inter task blocking and none of them are at all likely to cause a deadlock. 
Blocking is typically a result of one task requiring a Semaphore or Mutex from another, however since the majority of our 
tasks exclusively use atomic accesses this rarely becomes an issue. There are 3 occurances of semaphores in our code:

1. CAN Class TX Semaphore, used to give the Transmitter access to the CAN outboxes when they become available.

2. Board Array Mutex, used to ensure that read and writes to the board detect array by both the CAN_Class and 
   KeyScanner don't cause a race condition.

3. Mux Mutex, multiple classes sometimes try to use the setOutMuxBit function - this prevents them from causing any issues by 
   writing to the same pin twice.

Besides the TX mutex, these are used extremely rarely - only during handshaking and are therefore unlikely to cause any kind of deadlock.

There is one kind of deadlock that does occur sometimes, but it is not due to tasks on the same board and usually relates more to
tasks on different boards. Sometimes when boards are connected, they do not turn on their CAN in time (it takes a while for
them to power on) - resulting in an EAST message that is never received. Since the board will wait indefinitely until it receives a FIN
message, this can sometimes cause the entire system to freeze and need to be reset. Other than this very rare occurance, there should
not be an issue however.
