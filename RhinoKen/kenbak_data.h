
// Marcel Timm, RhinoDevel, 2024may13

#ifndef KENBAK_DATA
#define KENBAK_DATA

#include <stdint.h>

#include "kenbak_input.h"
#include "kenbak_output.h"
#include "kenbak_state.h"

#define KENBAK_DATA_DELAY_LINE_SIZE 128 // bytes

#define KENBAK_DATA_ADDR_A 0 // A "register".
#define KENBAK_DATA_ADDR_B 1 // B "register".
#define KENBAK_DATA_ADDR_X 2 // X "register".
#define KENBAK_DATA_ADDR_P 3 // P "register".
#define KENBAK_DATA_ADDR_OUTPUT 128 // Output "register".
#define KENBAK_DATA_ADDR_OC_A 129 // Overflow and carry for the A "register".
#define KENBAK_DATA_ADDR_OC_B 130 // Overflow and carry for the B "register".
#define KENBAK_DATA_ADDR_OC_X 131 // Overflow and carry for the X "register".
#define KENBAK_DATA_ADDR_INPUT 255 // Input "register".

struct kenbak_data
{
    struct kenbak_input input;

    struct kenbak_output output;

    enum kenbak_state state;
    
    // The boolean signals are all to-be-interpreted as true meaning that the
    // signal is active, even if the hardware puts the "signal" to ground (0V)
    // to signalize that it is active:

    // BU = The eight data input switches are scanned by T0 through T7 and ORed
    //      together to produce the serial signal BU.
    bool sig_bu;

    // CL = True when the clear push button is depressed. (04)
    bool sig_cl;

    // ^DA = Display address (from push button of that name). Goes to ground
    //       when button is pushed. (04)
    bool sig_da;

    // ^DD = Display data (from read memory push button) after being cleaned up.
    //       Goes to ground when button is pushed. (04)
    bool sig_dd;

    // ^EA = Enter Address (from set address push button) after being cleaned
    //       up. Goes to ground when button is pushed. (04)
    bool sig_ea;

    // ED = Automatic processing should end after the current instruction is
    //      finished. (22)
    bool sig_ed;

    // ^EN = Enter data (from store memory push button) after being cleaned up.
    //       Goes to ground when button is pushed. (04)
    bool sig_en;

    // ^GO = Signal from start push button after being cleaned up. Goes to
    //       ground when button is pushed. (04)
    bool sig_go;

    // X1, X2, X3, X4 or none of these signals:
    //
    enum kenbak_x sig_x;

    // R = Serial signal, "holds" the address for read or write.
    //     When R0 to R6 equal L0 to L6, the data for the desired address will
    //     be at the input and output of the memory at NEXT byte time.
    //     R7 is used to determine A7 (delay line select signal).
    // ^R = Desired address in logic complement form.
    //
    uint8_t sig_r;

    // I0/I7 = Generally the I register is the instruction register. I7 is the
    //         most significant bit. As an instruction register, I holds the
    //         first byte of an instruction. (14)
    //
    uint8_t reg_i;

    // K0/K7 = The K register controls the data lamps. K7 is the most
    //         significant bit. (07)
    //
    uint8_t reg_k;

    // W0/W7 = W register. W0 is least significant bit. A general utility
    //         register. (17)
    //
    uint8_t reg_w;

    uint8_t delay_line_0[KENBAK_DATA_DELAY_LINE_SIZE];
    uint8_t delay_line_1[KENBAK_DATA_DELAY_LINE_SIZE];
};

#endif //KENBAK_DATA
