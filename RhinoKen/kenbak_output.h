
// Marcel Timm, RhinoDevel, 2024may13

#ifndef KENBAK_OUTPUT
#define KENBAK_OUTPUT

#include <stdbool.h>

struct kenbak_output
{
    bool led_bit_7;
    bool led_bit_6;
    bool led_bit_5;
    bool led_bit_4;
    bool led_bit_3;
    bool led_bit_2;
    bool led_bit_1;
    bool led_bit_0;

    // The LED is above the input clear push button, but the LED being on
    // indicates in general that the machine is in the input mode.
    bool led_input_clear;

    bool led_address_set;

    bool led_memory_store;

    // Although LED is above the stop push button, the LED is on, if the machine
    // is in run mode.
    bool led_run_stop;
};

#endif //KENBAK_OUTPUT
