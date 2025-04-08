
// Marcel Timm, RhinoDevel, 2024may13

#ifndef KENBAK_INPUT
#define KENBAK_INPUT

#include <stdbool.h>

#define KENBAK_INPUT_BITS 8

struct kenbak_input
{
    bool buttons_data[KENBAK_INPUT_BITS];

    bool but_input_clear;

    bool but_address_display;
    bool but_address_set;

    bool switch_memory_lock;

    bool but_memory_read;
    bool but_memory_store;

    bool but_run_start;
    bool but_run_stop;

    bool switch_power_on;
};

#endif //KENBAK_INPUT
