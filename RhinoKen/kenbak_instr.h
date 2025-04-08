
// Marcel Timm, RhinoDevel, 2024may23

#ifndef KENBAK_INSTR
#define KENBAK_INSTR

#include <stdint.h>
#include <stdbool.h>

#include "kenbak_addr_mode.h"

// See PRM, page 24:
//
#define KENBAK_INSTR_IS_TWO_BYTE(instr_byte) (2 < (7 & (instr_byte)))

// Get the addressing mode for add/sub/load/store OR or/and/lneg instructions
// (for other instructions, this will FAIL):
//
#define KENBAK_INSTR_ASLS_OAL_ADDR_MODE(first_byte) \
    (((enum kenbak_addr_mode)(7 & (first_byte))))

// Is it a bit test or manipulation instruction (see PRM, page 24)?
//
#define KENBAK_INSTR_IS_BIT(instr_byte) (2 == (7 & (instr_byte)))

// - See PRM, page 24.
// - Detail may be inconsistent and to-be-expanded in the future, if necessary.
//
enum kenbak_instr_type
{
    kenbak_instr_type_invalid   = 0377, // 0377 would be a valid JMI-U, but is
                                        // used as invalid-value, here.

    kenbak_instr_type_add       = 0003, // E.g. ADD-A constant.
    kenbak_instr_type_sub       = 0013, // E.g. SUB-A constant.
    kenbak_instr_type_load      = 0023, // E.g. LOAD-A constant.
    kenbak_instr_type_store     = 0033, // E.g. STORE-A constant.

    kenbak_instr_type_or        = 0303, // E.g. OR constant.
    kenbak_instr_type_and       = 0323, // E.g. AND constant.
    kenbak_instr_type_lneg      = 0333, // E.g. LNEG constant.

    kenbak_instr_type_jump      = 0043, // E.g. JPD-A on != 0.

    kenbak_instr_type_bit       = 0002, // E.g. BSET 0 to 0.

    kenbak_instr_type_shift_rot = 0001, // E.g. RS-A by 4.

    kenbak_instr_type_misc      = 0000  // E.g. HALT.
};

enum kenbak_addr_mode kenbak_instr_get_addr_mode(uint8_t const first_byte);

enum kenbak_instr_type kenbak_instr_get_type(uint8_t const first_byte);

#endif //KENBAK_INSTR