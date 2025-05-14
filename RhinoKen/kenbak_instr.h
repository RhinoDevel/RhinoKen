
// Marcel Timm, RhinoDevel, 2024may23

#ifndef KENBAK_INSTR
#define KENBAK_INSTR

#include <stdint.h>
#include <stdbool.h>

#include "kenbak_addr_mode.h"

// See PRM, page 24:
//
#define KENBAK_INSTR_IS_031X_NOOP(instr_byte) \
    ((0370 & (instr_byte)) == 0310 && 2 < (7 & (instr_byte)))

// See PRM, page 24:
//
#define KENBAK_INSTR_IS_TWO_BYTE(instr_byte) ( \
    1 < (7 & (instr_byte)) \
        && (!KENBAK_INSTR_IS_031X_NOOP(instr_byte))) // Interpr. as 1-byte!

// Get the addressing mode for add/sub/load/store OR or/and/lneg instructions
// (for other instructions, this will FAIL):
//
#define KENBAK_INSTR_ASLS_OAL_ADDR_MODE(first_byte) \
    (((enum kenbak_addr_mode)(7 & (first_byte))))

// Is it a bit test or manipulation instruction (see PRM, page 24)?
//
#define KENBAK_INSTR_IS_BIT(instr_byte) (2 == (7 & (instr_byte)))

// Is it a halt instruction (see PRM, page 24)?
//
#define KENBAK_INSTR_IS_HALT(instr_byte) (0 == (0307 & (instr_byte)))

// Get the address to search for, if a single byte instruction is currently
// being processed (for other instructions, this will FAIL; see PRM, page 12 and
// 13; for 031* NOOP, this will return "B"):
//
#define KENBAK_INSTR_ONE_BYTE_SEARCH_A_OR_B(first_byte) \
    (((first_byte) >> 5) & 1)
//
// Hard coded, see KENBAK_DATA_ADDR_A & KENBAK_DATA_ADDR_B.

// 0 => A
// 1 => B
// 2 => X
// 3 => A (also)
//
#define KENBAK_INSTR_TWO_BYTE_SEARCH_A_B_OR_X(first_byte) \
    (((first_byte) >> 6) % 3)
//
// Hard coded, see KENBAK_DATA_ADDR_A, KENBAK_DATA_ADDR_B and
// KENBAK_DATA_ADDR_X.

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

    kenbak_instr_type_misc      = 0000  // HALT and NOOP.
};

enum kenbak_addr_mode kenbak_instr_get_addr_mode(uint8_t const first_byte);

enum kenbak_instr_type kenbak_instr_get_type(uint8_t const first_byte);

bool kenbak_instr_fill_str(
    char * const buf, size_t const buf_len, uint8_t const first_byte);

#endif //KENBAK_INSTR