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

## Advanced features

In this report, we will discuss how to create a mute button in C++. We will also describe how

### Creating a Mute Button

A mute button is a button that allows a user to mute or silence an audio signal. In order to create a mute button in C++.

When the button is pressed, the input pin will read a HIGH signal. We can then use a digital output pin to control the audio signal. When the button is pressed, the digital output pin will be set to LOW, which will silence the audio signal. When the button is released, the digital output pin will be set to HIGH, which will allow the audio signal to play.

Debouncing

Debouncing is the process of filtering out false signals that can occur when a button is pressed or released. When a button is pressed or released, it can cause a rapid fluctuation in the input signal, which can be interpreted as multiple button presses or releases. Debouncing helps to filter out these false signals, ensuring that the button press or release is registered accurately.

There are several ways to debounce a button in C++. One common method is to use a software debounce algorithm. This involves using a timer to delay the processing of the input signal for a short period of time, typically a few milliseconds. This allows any false signals to settle down, ensuring that only a single button press or release is registered.

Another method is to use a hardware debounce circuit, which filters out false signals at the hardware level. This can be done using a capacitor or a Schmitt trigger circuit.

Conclusion

In conclusion, creating a mute button in C++ involves connecting a button to a microcontroller and using a digital output pin to control the audio signal. Debouncing is an important process that helps to filter out false signals, ensuring that the button press or release is registered accurately. There are several ways to debounce a button, including software debounce algorithms and hardware debounce circuits.
