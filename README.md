# PWM-LED-ARRAY

This is a C file intended to be executed by an RP2040 microprocessor connected to an array of parallel LEDs, a transistor, a 7-segment display, and an HC4511E BCD-to-7-segment decoder.

The cathode of each LED is connected to the collector of the transistor. A PWM pin of the RP2040 is connected to the base. This program controls the operating mode of every LED synchronously by
modifying the duty cycle/status of the PWM pin.

There are 4 modes:
0: Off 
1: LEDs are solid
2: LEDs 'breathe' (fade in and out)
3: LEDs blink a message in Morse code

The 7-segment display displays the number of the current mode (blank if the mode is 0).
