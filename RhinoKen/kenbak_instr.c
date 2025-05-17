
// Marcel Timm, RhinoDevel, 2024may25

#include <stdint.h>
#include <stdbool.h>
#include <stdio.h>
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

bool kenbak_instr_fill_str(
    char * const buf, size_t const buf_len, uint8_t const first_byte)
{
    if(buf == NULL)
    {
        assert(false);
        return false;
    }
    //if(buf_len < ) // TODO: Implement!
    //{
    //    assert(false);
    //    return false;
    //}

    enum kenbak_instr_type const instr_type = kenbak_instr_get_type(first_byte);

    // TODO: Improve the following:
    //
    switch(instr_type)
    {
        case kenbak_instr_type_add: snprintf(buf, buf_len,          "ADD       "); break;
        case kenbak_instr_type_sub: snprintf(buf, buf_len,          "SUB       "); break;
        case kenbak_instr_type_load: snprintf(buf, buf_len,         "LOAD      "); break;
        case kenbak_instr_type_store: snprintf(buf, buf_len,        "STORE     "); break;

        case  kenbak_instr_type_or: snprintf(buf, buf_len,          "OR        "); break;
        case kenbak_instr_type_and: snprintf(buf, buf_len,          "AND       "); break;
        case kenbak_instr_type_lneg: snprintf(buf, buf_len,         "LNEG      "); break;

        case kenbak_instr_type_jump:
        {
            bool is_unc = false;

            switch(7 & (first_byte >> 3))
            {
                case 4: snprintf(buf, buf_len,                      "JPD       "); break;
                case 5: snprintf(buf, buf_len,                      "JPI       "); break;
                case 6: snprintf(buf, buf_len,                      "JMD       "); break;
                case 7: snprintf(buf, buf_len,                      "JMI       "); break;

                default:
                {
                    assert(false);
                    return false;
                }
            }

            switch(3 & (first_byte >> 6)) // TODO: buf_len - 4 is theoretically dangerous!
            {
                case 0: snprintf(buf + 4, buf_len - 4,                  "A     "); break;
                case 1: snprintf(buf + 4, buf_len - 4,                  "B     "); break;
                case 2: snprintf(buf + 4, buf_len - 4,                  "X     "); break;
                case 3: snprintf(buf + 4, buf_len - 4,                  "Unc.  "); is_unc = true; break;
                default:
                {
                    assert(false);
                    return false;
                }
            }

            if(!is_unc)
            {
                switch(7 & first_byte) // TODO: buf_len - 4 - 2 is theoretically dangerous!
                {
                    case 3: snprintf(buf + 4 + 2, buf_len - 4 - 2,        "!= 0"); break;
                    case 4: snprintf(buf + 4 + 2, buf_len - 4 - 2,        "== 0"); break;
                    case 5: snprintf(buf + 4 + 2, buf_len - 4 - 2,        "< 0 "); break;
                    case 6: snprintf(buf + 4 + 2, buf_len - 4 - 2,        ">= 0"); break;
                    case 7: snprintf(buf + 4 + 2, buf_len - 4 - 2,        "> 0 "); break;

                    default:
                    {
                        assert(false);
                        return false;
                    }
                }
            }

            break;
        }

        case kenbak_instr_type_bit: snprintf(buf, buf_len,          "BIT       "); break;

        case kenbak_instr_type_shift_rot: snprintf(buf, buf_len,    "SHIFT/ROT."); break;

        case kenbak_instr_type_misc: snprintf(buf, buf_len,         "NOOP/HALT "); break;

        case kenbak_instr_type_invalid: // (falls through)
        default:
        {
            assert(false);
            return false;
        }
    }

    // TODO: Implement!
    return true;
}
