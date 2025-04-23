
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

// See PRM, page 2:
// 
// - Bit 0 is the overflow flag/bit OF.
// - Bit 1 is the carry flag/bit CA.
// 
// - OF and CA are set on ADD and SUBTRACT, only.
// - When set, the other six bits are set to zero.
// 
// See PRM, page 3:
// 
// - Overflow is based on the signed representations of the bytes,
//   -128 to +127 (10000000 to 01111111).
// - Carry is the overflow for positive integer representations of the bytes,
//   0 to +255 (00000000 to 11111111).
//
#define KENBAK_DATA_ADDR_OC_FOR(addr_reg) (129 + (addr_reg))
//
#define KENBAK_DATA_ADDR_OC_A KENBAK_DATA_ADDR_OC_FOR(KENBAK_DATA_ADDR_A)
#define KENBAK_DATA_ADDR_OC_B KENBAK_DATA_ADDR_OC_FOR(KENBAK_DATA_ADDR_B)
#define KENBAK_DATA_ADDR_OC_X KENBAK_DATA_ADDR_OC_FOR(KENBAK_DATA_ADDR_X)

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

    // The count of bytes that need to be added to P during state SB.
    //
    // PF => Four is to be added to the P register:
    //       Only SKP 0 and SKP 1, if condition is true.
    // PT => Two is to be added to the P register:
    //       All two-byte instructions, but SKP 0 and SKP 1, if condition is
    //       true AND probably something special with jump instructions..?
    // PO => One is to be added to the P register:
    //       All one-byte instructions (even HALT).
    // 
    // - What about JPD, JPI, JMD and JMI, if jump condition is true?
    //
    uint8_t sig_inc;

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

    // This indicates, if the memory shall initially be filled with random
    // values or with zeros.
    bool randomize_memory;

    uint8_t delay_line_0[KENBAK_DATA_DELAY_LINE_SIZE];
    uint8_t delay_line_1[KENBAK_DATA_DELAY_LINE_SIZE];
};

#endif //KENBAK_DATA
