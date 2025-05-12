
// Marcel Timm, RhinoDevel, 2024may25

#include <stdint.h>
#include <stdbool.h>
#include <assert.h>

#include "kenbak_instr.h"
#include "kenbak_addr_mode.h"

/**
 * - Can be performance-improved by using a LUT. 
 */
enum kenbak_addr_mode kenbak_instr_get_addr_mode(uint8_t const first_byte)
{
    if(!KENBAK_INSTR_IS_TWO_BYTE(first_byte))
    {
        // Shift/rotate or miscellaneous (halt, noop).

        return kenbak_addr_mode_none;
    }

    // Two-byte instruction.

    if(KENBAK_INSTR_IS_BIT(first_byte))
    {
        return kenbak_addr_mode_memory; // Bit test or manipulation instruction.
    }

    uint8_t const mid_sign_oct = 7 & (first_byte >> 3);

    if(3 < mid_sign_oct)
    {
        // Jump instruction.

        if(mid_sign_oct % 2 == 0)
        {
            // Direct addressing.

            return kenbak_addr_mode_constant/*kenbak_addr_mode_memory*/;
        }

        // Indirect addressing.

        return kenbak_addr_mode_memory/*kenbak_addr_mode_indirect*/;
    }

    // Add/sub/load/store or or/and/lneg instruction.

    return KENBAK_INSTR_ASLS_OAL_ADDR_MODE(first_byte);
}

/**
 * - Can be performance-improved by using a LUT.
 * - Detail may be expanded in the future, if necessary.
 */
enum kenbak_instr_type kenbak_instr_get_type(uint8_t const first_byte)
{
    if(KENBAK_INSTR_IS_031X_NOOP(first_byte))
    {
        return kenbak_instr_type_misc;
    }

    uint8_t const least_sign_oct = 7 & first_byte;

    switch(least_sign_oct)
    {
        case 0:
        {
            return kenbak_instr_type_misc;
        }
        case 1:
        {
            return kenbak_instr_type_shift_rot;
        }
        case 2:
        {
            return kenbak_instr_type_bit;
        }

        default:
        {
            assert(3 <= least_sign_oct && least_sign_oct <= 7); // Stupid..

            uint8_t const mid_sign_oct = 7 & (first_byte >> 3);

            if(3 < mid_sign_oct)
            {
                return kenbak_instr_type_jump;
            }

            uint8_t const most_sign_oct = 7 & (first_byte >> 6);

            if(most_sign_oct == 3)
            {
                switch(mid_sign_oct)
                {
                    case 0:
                    {
                        return kenbak_instr_type_or;
                    }
                    case 1:
                    {
                        return kenbak_instr_type_misc; // NOOP!
                    }
                    case 2:
                    {
                        return kenbak_instr_type_and;
                    }
                    case 3:
                    {
                        return kenbak_instr_type_lneg;
                    }

                    default:
                    {
                        assert(false); // Cannot get here.
                        return kenbak_instr_type_invalid;
                    }
                }
            }

            switch(mid_sign_oct)
            {
                case 0:
                {
                    return kenbak_instr_type_add;
                }
                case 1:
                {
                    return kenbak_instr_type_sub;
                }
                case 2:
                {
                    return kenbak_instr_type_load;
                }
                case 3:
                {
                    return kenbak_instr_type_store;
                }

                default:
                {
                    assert(false); // Cannot get here.
                    return kenbak_instr_type_invalid;
                }
            }
        }
    }
}
