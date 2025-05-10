
// Marcel Timm, RhinoDevel, 2024may13

#include <stdlib.h>
#include <assert.h>
#include <time.h>

#include "kenbak_emu.h"
#include "kenbak_data.h"
#include "kenbak_instr.h"
#include "kenbak_x.h"
#include "kenbak_jmp_cond.h"

// *****************************************************************************
// *** HELPER FUNCTIONS                                                      ***
// *****************************************************************************

static uint8_t get_rotated_left(uint8_t const val, int const places)
{
    assert(1 <= places && places <= 4); // This is what the Kenbak-1 supports.

    // (76543210 << 1) | (76543210 >> (8 - 1)) = 6543210x | xxxxxxx7 = 65432107
    // (76543210 << 2) | (76543210 >> (8 - 2)) = 543210xx | xxxxxx76 = 54321076
    // (76543210 << 3) | (76543210 >> (8 - 3)) = 43210xxx | xxxxx765 = 43210765
    // (76543210 << 4) | (76543210 >> (8 - 4)) = 3210xxxx | xxxx7654 = 32107654

    return (val << places) | (val >> (8 - places));
}

static uint8_t get_rotated_right(uint8_t const val, int const places)
{
    assert(1 <= places && places <= 4); // This is what the Kenbak-1 supports.

    // (76543210 >> 1) | (76543210 << (8 - 1)) = x7654321 | 0xxxxxxx = 07654321
    // (76543210 >> 2) | (76543210 << (8 - 2)) = xx765432 | 10xxxxxx = 10765432
    // (76543210 >> 3) | (76543210 << (8 - 3)) = xxx76543 | 210xxxxx = 21076543
    // (76543210 >> 4) | (76543210 << (8 - 4)) = xxxx7654 | 3210xxxx = 32107654

    return (val >> places) | (val << (8 - places));
}

// *****************************************************************************
// *** READ-TO AND WRITE-FROM MEMORY                                         ***
// *****************************************************************************

static void mem_write(
    struct kenbak_data * const d, uint8_t const addr, uint8_t const val)
{
    if(addr < KENBAK_DATA_DELAY_LINE_SIZE)
    {
        d->delay_line_0[addr] = val;
        return;
    }
    d->delay_line_1[addr - KENBAK_DATA_DELAY_LINE_SIZE] = val;
}

static uint8_t mem_read(struct kenbak_data * const d, uint8_t const addr)
{
    if(addr < KENBAK_DATA_DELAY_LINE_SIZE)
    {
        return d->delay_line_0[addr];
    }
    return d->delay_line_1[addr - KENBAK_DATA_DELAY_LINE_SIZE];
}

// *****************************************************************************
// *** INITIALIZE KENBAK-1 DATA STRUCTURE                                    ***
// *****************************************************************************

void kenbak_emu_init_input(
    struct kenbak_data * const d, bool const keep_switch_power_on)
{
    for(int i = 0; i < KENBAK_INPUT_BITS; ++i)
    {
        d->input.buttons_data[i] = false;
    }
    
    d->input.but_input_clear = false;

    d->input.but_address_display = false;
    d->input.but_address_set = false;

    d->input.switch_memory_lock = false;

    d->input.but_memory_read = false;
    d->input.but_memory_store = false;

    d->input.but_run_start = false;
    d->input.but_run_stop = false;

    if (!keep_switch_power_on)
    {
        d->input.switch_power_on = false;
    }
}

static void init_output(struct kenbak_data * const d)
{
    d->output.led_bit_7 = false;
    d->output.led_bit_6 = false;
    d->output.led_bit_5 = false;
    d->output.led_bit_4 = false;
    d->output.led_bit_3 = false;
    d->output.led_bit_2 = false;
    d->output.led_bit_1 = false;
    d->output.led_bit_0 = false;

    d->output.led_input_clear = false;
    
    d->output.led_address_set = false;
    
    d->output.led_memory_store = false;
    
    d->output.led_run_stop = false;
}

static void init_state(struct kenbak_data * const d)
{
    d->state = kenbak_state_power_off;
}

static void init_registers(struct kenbak_data * const d)
{
    d->reg_i = 0;
    d->reg_k = 0;
    d->reg_w = 0;
}

static void init_signals(struct kenbak_data * const d)
{
    d->sig_bu = false;
    d->sig_cl = false;
    d->sig_da = false;
    d->sig_dd = false;
    d->sig_ea = false;
    d->sig_ed = false;
    d->sig_en = false;
    d->sig_go = false;

    d->sig_x = kenbak_x_none;

    d->sig_r = 0;

    d->sig_inc = 0;
}

static void init_mem(struct kenbak_data * const d)
{
    if(d->randomize_memory)
    {
        srand((unsigned int)time(NULL));
    }

    for(int i = 0; i < 2 * KENBAK_DATA_DELAY_LINE_SIZE; ++i)
    {
        uint8_t const val = d->randomize_memory ? (rand() % 256) : 0;

        mem_write(d, i, val);
    }
}

static void init(struct kenbak_data * const d)
{
    kenbak_emu_init_input(d, false);
    init_output(d);
    init_state(d);
    init_registers(d);
    init_signals(d);
    init_mem(d);
}

// *****************************************************************************
// *** THE STATES OF THE KENBAK-1 STATE MACHINE                              ***
// *****************************************************************************

/** Start of the next instruction. Locates the P register in memory.
 *
 *  - Byte time count depends on delay line position. 
 *  - See page 28.
 */
static int step_in_sa(struct kenbak_data * const d)
{
    assert(d->state == kenbak_state_sa);

    d->sig_r = KENBAK_DATA_ADDR_P;

    // In reality, waiting for CM, here.
    //
    d->state = kenbak_state_sb;
    return 1;
}

/** Increments the program counter (P register). Fills W register with resulting
 *  address of next instruction. Decides, if QC or SC is next.
 * 
 *  - Lasts one byte time.
 *  - See page 28.
 */
static int step_in_sb(struct kenbak_data * const d)
{
    assert(d->state == kenbak_state_sb);
    assert(d->sig_r == KENBAK_DATA_ADDR_P); // See SA.
    assert(0 <= d->sig_inc && d->sig_inc != 3 && d->sig_inc <= 4);

    // Address of the last executed instruction plus the length of that
    // instruction, to get the address of the next instruction to be executed:
    //
    uint8_t const val = mem_read(d, d->sig_r) + d->sig_inc;

    d->sig_inc = 255; // Sets to invalid to indicate that it needs to be set.

    mem_write(d, d->sig_r, val);

    d->reg_w = val;

    if(d->sig_ed)
    {
        // Last instruction was a halt or stop button was pressed.

        d->state = kenbak_state_qc;

        // Disabling ED here to prevent overwrite that would happen, if ED was
        // triggered by a HALT instruction (and not the run stop button), also
        // see update_input_signals():
        //
        d->sig_ed = false;

        return 1;
    }

    // Continue automatic operation.

    d->state = kenbak_state_sc;
    return 1;
}

/** Finds the next instruction in memory.
 *
 *  - Byte time count depends on delay line position.
 *  - See page 29.
 */
static int step_in_sc(struct kenbak_data * const d)
{
    assert(d->state == kenbak_state_sc);
    assert(d->reg_w == mem_read(d, KENBAK_DATA_ADDR_P)); // See SB.

    d->sig_r = d->reg_w;

    // In reality, waiting for CM, here.
    //
    d->state = kenbak_state_sd;
    return 1;
}

/** Transfers next instruction's first byte to register I. Selects next state
 *  based on the next instruction's length. 
 * 
 *  - Lasts one byte time.
 *  - See page 29.
 */
static int step_in_sd(struct kenbak_data * const d)
{
    assert(d->state == kenbak_state_sd);
    assert(d->reg_w == mem_read(d, KENBAK_DATA_ADDR_P)); // See SB.
    assert(d->sig_r == d->reg_w); // See SC.

    // Transfer first byte of to-be-executed instruction to I register:
    //
    d->reg_i = mem_read(d, d->sig_r);

    if(KENBAK_INSTR_IS_TWO_BYTE(d->reg_i))
    {
        d->state = kenbak_state_se; // Will read second byte of transfer.
        return 1;
    }

    // It is a single byte instruction.

    assert(d->sig_inc == 255); // Must still be unset/invalid, here.
    d->sig_inc = 1; // All one byte instructions cause a P = P + 1.

    d->state = kenbak_state_su; // Will seek A or B register.
    return 1;
}

/** First byte of instruction is already in register I. This function transfers
 *  the address of the to-be-used value to the W register; decides, which state
 *  is next by the address mode of the instruction.
 * 
 *  - Lasts one byte time.
 *  - See page 29.
 */
static int step_in_se(struct kenbak_data * const d)
{
    assert(d->state == kenbak_state_se);
    assert(d->reg_w == mem_read(d, KENBAK_DATA_ADDR_P)); // See SB.
    assert(d->reg_w == d->sig_r); // See SC.
    assert(d->reg_i == mem_read(d, d->sig_r)); // See SD.
    assert(KENBAK_INSTR_IS_TWO_BYTE(d->reg_i)); // See SD.

    enum kenbak_addr_mode const addr_mode =
        kenbak_instr_get_addr_mode(d->reg_i);

    enum kenbak_instr_type const instr_type = kenbak_instr_get_type(d->reg_i);

    if(addr_mode == kenbak_addr_mode_constant
        && instr_type == kenbak_instr_type_store)
    {
        // The instruction is store constant/immediate, load ADDRESS of the
        // second byte of the instruction into W register (this following second
        // byte will be overwritten in-place by the store immediate operation):

        d->reg_w = d->sig_r + 1;
    }
    else
    {
        // The instruction is NOT store constant/immediate.
        // Transfer second byte of to-be-executed instruction to W register:

        d->reg_w = mem_read(d, d->sig_r + 1);
    }

    if(addr_mode == kenbak_addr_mode_indirect
        || addr_mode == kenbak_addr_mode_indirect_indexed)
    {
        d->state = kenbak_state_sf; // SE -JI+IND-> SF
        return 1;
    }

    if(addr_mode == kenbak_addr_mode_indexed)
    {
        d->state = kenbak_state_sh; // SE -^IND*DEX-> SH
        return 1;
    }
    
    if(addr_mode == kenbak_addr_mode_constant
        || instr_type == kenbak_instr_type_jump // Must be JD, here.
        || (instr_type == kenbak_instr_type_store // TM
                && addr_mode == kenbak_addr_mode_memory))
    {
        // No operand to be found.

        assert(
            instr_type != kenbak_instr_type_jump
                || ((d->reg_i >> 3) & 7) == 4 // JPD
                || ((d->reg_i >> 3) & 7) == 6); // JMD

        d->state = kenbak_state_sm; // SE -IMMED+JD+TM*MEM-> SM
        return 1;
    }

    assert(instr_type == kenbak_instr_type_bit // BM
        || (instr_type != kenbak_instr_type_store // ^TM
                && addr_mode == kenbak_addr_mode_memory // MEM
                && instr_type != kenbak_instr_type_jump)); // ^J

    d->state = kenbak_state_sk; // SE -BM+^TM*MEM*^J-> SK
    return 1;
}

/** Searches for the indirect address which is already in register W (see SE).
 * 
 *  - Byte time count depends on delay line position.
 *  - See page 31.
 */
static int step_in_sf(struct kenbak_data * const d)
{
    assert(d->state == kenbak_state_sf);
    assert(d->sig_r == mem_read(d, KENBAK_DATA_ADDR_P)); // See SB & SC.
    assert(d->reg_i == mem_read(d, d->sig_r)); // See SD.
    assert(KENBAK_INSTR_IS_TWO_BYTE(d->reg_i)); // See SD.

    d->sig_r = d->reg_w;

    // In reality, waiting for CM, here.
    //
    d->state = kenbak_state_sg;
    return 1;
}

/** Transfer content of indirect address location to W register; decides, which
 *  state is next based on the addressing mode (is indexing also required or
 *  not? If no indexing, is it a jump instruction?).
 * 
 *  - Probably always takes on byte time..
 *  - See page 31.
 */
static int step_in_sg(struct kenbak_data * const d)
{
    assert(d->state == kenbak_state_sg);
    assert(KENBAK_INSTR_IS_TWO_BYTE(d->reg_i)); // See SD.
    assert(d->sig_r == d->reg_w); // See SF.

    d->reg_w = mem_read(d, d->sig_r);

    enum kenbak_addr_mode const addr_mode =
        kenbak_instr_get_addr_mode(d->reg_i);

    switch(addr_mode)
    {
        case kenbak_addr_mode_indirect_indexed: // SG -DEX-> SH
        {
            d->state = kenbak_state_sh;
            return 1;
        }
        case kenbak_addr_mode_indirect: 
        {
            enum kenbak_instr_type const instr_type = kenbak_instr_get_type(
                d->reg_i);

            // Jump indirect or store without indexing. SG -JI+TM*^DEX-> SM
            //
            if(instr_type == kenbak_instr_type_jump
                || instr_type == kenbak_instr_type_store)
            {
                d->state = kenbak_state_sm;
                return 1;
            }

            // SG -^DEX*^J*^TM-> SK
            //
            d->state = kenbak_state_sk;
            return 1;
        }

        case kenbak_addr_mode_none: // Falls through..
        case kenbak_addr_mode_constant: // Falls through..
        case kenbak_addr_mode_memory: // Falls through..
        case kenbak_addr_mode_indexed: // Falls through..
        default:
        {
            assert(false); // Must not get here.
            return 0;
        }
    }
}

/** Searches for X register. Previous states are SE (for NON-indirect indexing)
 *  or SG (for indirect indexing).
 * 
 *  - Byte time count depends on delay line position.
 *  - See page 31.
 */
static int step_in_sh(struct kenbak_data * const d)
{
    assert(d->state == kenbak_state_sh);
    assert(d->sig_r == mem_read(d, KENBAK_DATA_ADDR_P)); // See SB & SC.
    assert(d->reg_i == mem_read(d, d->sig_r)); // See SD.
    assert(KENBAK_INSTR_IS_TWO_BYTE(d->reg_i)); // See SD.

    d->sig_r = KENBAK_DATA_ADDR_X;

    // In reality, waiting for CM, here.
    //
    d->state = kenbak_state_sj;
    return 1;
}

/** Adds contents of X register (X was found via signal R in SH) to W.
 * 
 *  - Takes one byte time.
 *  - See page 31.
 */
static int step_in_sj(struct kenbak_data * const d)
{
    assert(d->state == kenbak_state_sj);
    assert(d->reg_i == mem_read(d, d->sig_r)); // See SD.
    assert(KENBAK_INSTR_IS_TWO_BYTE(d->reg_i)); // See SD.
    assert(d->sig_r == KENBAK_DATA_ADDR_X); // see SH.

    d->reg_w += mem_read(d, d->sig_r); // Adds content of X register to W reg.

    enum kenbak_instr_type const instr_type = kenbak_instr_get_type(d->reg_i);

    if(instr_type == kenbak_instr_type_store)
    {
        d->state = kenbak_state_sm; // SJ -TM-> SM
        return 1;
    }
    d->state = kenbak_state_sk; // SJ -^TM-> SK
    return 1;
}

/** W holds the operand's address on entry. Searches for that address to be able
 *  to read the operand from memory later.
 * 
 *  - Byte time count depends on delay line position.
 *  - See page 32.
 */
static int step_in_sk(struct kenbak_data * const d)
{
    assert(d->state == kenbak_state_sk);

    // d->reg_w contains the address of the operand.

    d->sig_r = d->reg_w;

    // In reality, waiting for CM, here.
    //
    d->state = kenbak_state_sl;
    return 1;
}

/**
 * - Takes one byte time for non-bit manipulation instructions.
 *   Assuming one byte time for SKIP, too.
 *   For SET, this should take the maximum byte time (128 bytes), if I am
 *   correct..
 * - See page 32.
 */
static int step_in_sl(struct kenbak_data * const d)
{
    assert(d->state == kenbak_state_sl);
    assert(d->sig_r == d->reg_w);

    uint8_t mask = 0;

    enum kenbak_instr_type const instr_type = kenbak_instr_get_type(d->reg_i);

    d->reg_w = mem_read(d, d->sig_r); // Loads operand.

    if(instr_type != kenbak_instr_type_bit)
    {
        d->state = kenbak_state_sm; // SL -^BM-> SM
        return 1;
    }

    // "For the Set 0 or Set 1 instructions, the designated bit is set during
    // SL. For the Skip on 0 and Skip on 1 instructions, the P register
    // increment control is set as necessary."

    d->state = kenbak_state_sa; // SL -BM-> SA

    int const bit_pos = (d->reg_i >> 3) & 7;

    mask = 1 << bit_pos;
    if((d->reg_i & 0x80) != 0)
    {
        // SKIP

        bool const bit_is_set = (d->reg_w & mask) != 0;

        assert(d->sig_inc == 255);
        d->sig_inc = 2;
        if((d->reg_i & 0x40) != 0)
        {
            // Skip on 1.

            if(bit_is_set)
            {
                d->sig_inc += 2; // Always skips two bytes.
            }
        }
        else
        {
            // Skip on 0.

            if(!bit_is_set)
            {
                d->sig_inc += 2; // Always skips two bytes.
            }
        }
        return 1; // Assuming one byte time, here.
    }

    // SET

    assert(d->sig_inc == 255);
    d->sig_inc = 2;

    if((d->reg_i & 0x40) != 0)
    {
        d->reg_w = d->reg_w | mask; // Sets to 1.
    }
    else
    {
        d->reg_w = d->reg_w & (uint8_t)~mask; // Sets to 0.
    }

    mem_write(d, d->sig_r, d->reg_w);
    return 1; // Not correct, see comment above.
}

/**
 * - See page 32.
 * - Byte time count depends on delay line position.
 * - W already contains the operand for instructions that will modify A, B or X.
 */
static int step_in_sm(struct kenbak_data * const d)
{
    assert(d->state == kenbak_state_sm);
    assert(KENBAK_INSTR_IS_TWO_BYTE(d->reg_i));

    d->sig_r = KENBAK_INSTR_TWO_BYTE_SEARCH_A_B_OR_X(d->reg_i);

    assert(
        d->sig_r == KENBAK_DATA_ADDR_A
            || d->sig_r == KENBAK_DATA_ADDR_B
            || d->sig_r == KENBAK_DATA_ADDR_X);

    enum kenbak_instr_type const instr_type = kenbak_instr_get_type(d->reg_i);

    if(instr_type == kenbak_instr_type_jump)
    {
        // JPD, JPI, JMD and JMI.

        // W already contains the target address (which may be the jump or the
        // "mark" address, see PRM, page 33).

        // In reality, waiting for CM, here.
        //
        d->state = kenbak_state_sz;
        return 1;
    }

    if(instr_type == kenbak_instr_type_store)
    {
        // STORE

        // W already contains the address where the data is to be stored
        // (see PRM, page 33).

        // In reality, waiting for CM, here.
        //
        d->state = kenbak_state_sp;
        return 1;
    }

    // ADD, SUB, LOAD, AND, OR and LNEG.

    assert(
        instr_type == kenbak_instr_type_add
        || instr_type == kenbak_instr_type_sub
        || instr_type == kenbak_instr_type_load
        || instr_type == kenbak_instr_type_and
        || instr_type == kenbak_instr_type_or
        || instr_type == kenbak_instr_type_lneg);

    // W already contains the operand (see PRM, page 32).

    // In reality, waiting for CM, here.
    //
    d->state = kenbak_state_sn;
    return 1;
}

/** Instructions changing A, B or X do so during SN. W contains the operand.
 * 
 * - Takes one byte time.
 * - See page 33.
 */
static int step_in_sn(struct kenbak_data * const d)
{
    assert(d->state == kenbak_state_sn);
    assert(
        d->sig_r == KENBAK_DATA_ADDR_A
        || d->sig_r == KENBAK_DATA_ADDR_B
        || d->sig_r == KENBAK_DATA_ADDR_X
    
        || d->sig_r == KENBAK_DATA_ADDR_P); // P in case of a jump.

    enum kenbak_instr_type const instr_type = kenbak_instr_get_type(d->reg_i);

    // A, B or X is read from memory (see SM):
    //
    uint8_t const reg_content = mem_read(d, d->sig_r);
    uint8_t result = 0;
    bool do_sub = false;

    switch(instr_type)
    {
        case kenbak_instr_type_sub: // See PRM, page 5.
        {
            do_sub = true; // Falls through.
        }
        case kenbak_instr_type_add: // See PRM, page 5.
        {
            uint16_t const buf =
                (uint16_t)(do_sub
                    ? (uint8_t)(-d->reg_w) // Use two's complement.
                    : d->reg_w)
                + (uint16_t)reg_content;
            uint8_t overflow_and_carry = 0;

            if(255 < buf)
            {
                overflow_and_carry = overflow_and_carry & 1; // Hard-coded 1.
            }

            result = (uint8_t)buf;

            // TODO: Verify that this is correctly implemented:
            //
            if(d->reg_w <= 127 && 127 < result)
            {
                overflow_and_carry = overflow_and_carry & 2; // Hard-coded 2.
            }

            mem_write(d, KENBAK_DATA_ADDR_OC_FOR(d->sig_r), overflow_and_carry);

            assert(d->sig_inc == 255);
            d->sig_inc = 2;
            break;
        }
        case kenbak_instr_type_load: // See PRM, page 6.
        {
            result = d->reg_w;

            assert(d->sig_inc == 255);
            d->sig_inc = 2;
            break;
        }
        case kenbak_instr_type_and: // See PRM, page 7.
        {
            assert(d->sig_r == KENBAK_DATA_ADDR_A);

            result = d->reg_w & reg_content;

            assert(d->sig_inc == 255);
            d->sig_inc = 2;
            break;
        }
        case kenbak_instr_type_or: // See PRM, page 7.
        {
            assert(d->sig_r == KENBAK_DATA_ADDR_A);

            result = d->reg_w | reg_content;

            assert(d->sig_inc == 255);
            d->sig_inc = 2;
            break;
        }
        case kenbak_instr_type_lneg: // See PRM, page 8.
        {
            assert(d->sig_r == KENBAK_DATA_ADDR_A);

            result = -d->reg_w; // "Arithmetic complement" // TODO: Verify (e.g. -128 => -128)!

            assert(d->sig_inc == 255);
            d->sig_inc = 2;
            break;
        }
        case kenbak_instr_type_jump:
        {
            assert(d->sig_r == KENBAK_DATA_ADDR_P);

            result = d->reg_w; // W holds the jump destination address.

            assert(d->sig_inc == 0); // See SZ.
            break;
        }

        default:
        {
            assert(false);
            return 0; // Error!
        }
    }

    mem_write(d, d->sig_r, result);

    d->state = kenbak_state_sa;
    assert(d->sig_inc != 255);
    return 1;
}

/**
 * - See page 33.
 */
static int step_in_sp(struct kenbak_data * const d)
{
    assert(d->state == kenbak_state_sp);
    assert(KENBAK_INSTR_IS_TWO_BYTE(d->reg_i));
    assert(
        d->sig_r == KENBAK_DATA_ADDR_A
        || d->sig_r == KENBAK_DATA_ADDR_B
        || d->sig_r == KENBAK_DATA_ADDR_X);
    assert(kenbak_instr_get_type(d->reg_i) == kenbak_instr_type_store);

    // W already contains the address where the data is to be stored (see PRM,
    // page 33).

    // Load the byte to be stored in memory to the I register:
    //
    d->reg_i = mem_read(d, d->sig_r);

    d->state = kenbak_state_sr;
    return 1; // Unsure, if this really takes a single byte time.
}

/**
 *  - Byte time count depends on delay line position.
 *  - See page 34.
 */
static int step_in_sr(struct kenbak_data * const d)
{
    assert(d->state == kenbak_state_sr);

    d->sig_r = d->reg_w;

    // In reality, waiting for CM, here.
    //
    d->state = kenbak_state_ss;
    return 1;
}

/** Write content of register I to memory.
 * 
 *  - Takes one byte time.
 *  - See page 34.
 */
static int step_in_ss(struct kenbak_data * const d)
{
    assert(d->state == kenbak_state_ss);
    assert(d->sig_r == d->reg_w); // See SR.

    mem_write(d, d->sig_r, d->reg_i);

    // Maybe there is a better position in code to do this?
    //
    assert(d->sig_inc == 255);
    d->sig_inc = 2;

    d->state = kenbak_state_sa;
    return 1;
}

/**
 * - Byte time count depends on delay line position.
 * - See page 34.
 */
static int step_in_st(struct kenbak_data * const d)
{
    assert(d->state == kenbak_state_st);
    assert(kenbak_instr_get_type(d->reg_i) == kenbak_instr_type_jump);

    d->sig_r = KENBAK_DATA_ADDR_P;

    // Without "marking" => SN

    // With "marking" => SQ

    // In reality, waiting for CM, here.

    if((0x10 & d->reg_i) == 0) // See PRM, page 9 (bit 4 decides about marking).
    {
        d->state = kenbak_state_sn; // Jump (without Mark).
        return 1;
    }
    d->state = kenbak_state_sq; // Jump and Mark.
    return 1;
}

/**
 * - Byte time count depends on delay line position.
 * - See page 35.
 */
static int step_in_su(struct kenbak_data * const d)
{
    assert(d->state == kenbak_state_su);
    assert(!KENBAK_INSTR_IS_TWO_BYTE(d->reg_i));

    d->sig_r = KENBAK_INSTR_ONE_BYTE_SEARCH_A_OR_B(d->reg_i);

    assert(d->sig_r == KENBAK_DATA_ADDR_A || d->sig_r == KENBAK_DATA_ADDR_B);

    // In reality, waiting for CM, here.
    //
    d->state = kenbak_state_sv;
    return 1;
}

/**
 * - See page 35.
 */
static int step_in_sv(struct kenbak_data * const d)
{
    assert(d->state == kenbak_state_sv);

    // See SU:
    //
    assert(d->sig_r == KENBAK_INSTR_ONE_BYTE_SEARCH_A_OR_B(d->reg_i));
    assert(d->sig_r == KENBAK_DATA_ADDR_A || d->sig_r == KENBAK_DATA_ADDR_B);

    // Transfer content of A or B to W:
    //
    d->reg_w = mem_read(d, d->sig_r);

    enum kenbak_instr_type const instr_type = kenbak_instr_get_type(d->reg_i);

    if(instr_type == kenbak_instr_type_misc)
    {
        // ^IO

        if(KENBAK_INSTR_IS_HALT(d->reg_i))
        {
            d->sig_ed = true;
        }

        d->state = kenbak_state_sa; // Done for HALT and NOOP.
        return 1;
    }

    // IO

    assert(instr_type == kenbak_instr_type_shift_rot);
    d->state = kenbak_state_sw; // Will execute shifts or rotates.
    return 1;
}

/** Shifts and rotates are executed here.
 * 
 * - Takes one byte time.
 * - See page 35.
 * - Also see PRM, page 12.
 */
static int step_in_sw(struct kenbak_data * const d)
{
    // See SU:
    //
    assert(d->sig_r == KENBAK_INSTR_ONE_BYTE_SEARCH_A_OR_B(d->reg_i));
    assert(d->sig_r == KENBAK_DATA_ADDR_A || d->sig_r == KENBAK_DATA_ADDR_B);

    // See SV:
    //
    assert(kenbak_instr_get_type(d->reg_i) == kenbak_instr_type_shift_rot);
    assert(d->reg_w == mem_read(d, d->sig_r));

    // W already holds the content loaded from A or B.

    // 76 543 210
    //
    uint8_t const kind = d->reg_i >> 6;
    uint8_t places = (d->reg_i >> 3) & 3; // 0 means 4!
    
    if(places == 0)
    {
        places = 4;
    }
    assert(1 <= places && places <= 4);

    switch(kind)
    {
        case 0: // Right shift.
        {
            d->reg_w = d->reg_w >> places;
            break;
        }
        case 1: // Right rotate.
        {
            d->reg_w = get_rotated_right(d->reg_w, places);
            break;
        }
        case 2: // Left shift.
        {
            d->reg_w = d->reg_w << places;
            break;
        }
        case 3: // Left rotate.
        {
            d->reg_w = get_rotated_left(d->reg_w, places);
            break;
        }

        default: // Must not get here.
        {
            assert(false);
            return 0; // Error!
        }
    }

    d->state = kenbak_state_sx;
    return 1;
}

/**
 * - Byte time count depends on delay line position.
 * - See page 35.
 */
static int step_in_sx(struct kenbak_data * const d)
{
    assert(d->state == kenbak_state_sx);

    d->sig_r = KENBAK_INSTR_ONE_BYTE_SEARCH_A_OR_B(d->reg_i);

    assert(d->sig_r == KENBAK_DATA_ADDR_A || d->sig_r == KENBAK_DATA_ADDR_B);

    // In reality, waiting for CM, here.
    //
    d->state = kenbak_state_sy;
    return 1;
}

/**
 * - See page 35.
 */
static int step_in_sy(struct kenbak_data * const d)
{
    assert(d->state == kenbak_state_sy);
    assert(d->sig_r == KENBAK_DATA_ADDR_A || d->sig_r == KENBAK_DATA_ADDR_B);

    // W holds the shifted or rotated value that also originated in A or B.
    //
    mem_write(d, d->sig_r, d->reg_w);

    d->state = kenbak_state_sa; // Done for rotate/shift instructions.
    return 1;
}

/**
 * - Takes one byte time.
 * - See page 34.
 */
static int step_in_sz(struct kenbak_data * const d)
{
    assert(d->state == kenbak_state_sz);
    assert(kenbak_instr_get_type(d->reg_i) == kenbak_instr_type_jump);
    assert(
        d->sig_r == KENBAK_DATA_ADDR_A
        || d->sig_r == KENBAK_DATA_ADDR_B
        || d->sig_r == KENBAK_DATA_ADDR_X);

    bool cond_is_true = false;
    uint8_t const reg_sel = (0xC0 & d->reg_i) >> 6;

    if(reg_sel == 3) // Unconditional jump, if bit 7 and 6 are both set.
    {
        d->state = kenbak_state_st; // Jump!
        assert(d->sig_inc == 255);
        d->sig_inc = 0;
        return 1;
    }

    // A, B or X need to be checked.

    assert(reg_sel == d->sig_r);

    uint8_t const reg_val = mem_read(d, d->sig_r);

    uint8_t const cond = (7 & d->reg_i);

    assert(3 <= cond); // See possible jump conditions (from 3 to 7).

    assert(!cond_is_true);
    switch((enum kenbak_jmp_cond)cond)
    {
        case kenbak_jmp_cond_non_zero:
        {
            if(reg_val != 0)
            {
                cond_is_true = true;
            }
            break;
        }
        case kenbak_jmp_cond_zero:
        {
            if(reg_val == 0)
            {
                cond_is_true = true;
            }
            break;
        }
        case kenbak_jmp_cond_neg:
        {
            if((0x80 & reg_val) != 0) // Is 7th bit set?
            {
                cond_is_true = true;
            }
            break;
        }
        case kenbak_jmp_cond_pos:
        {
            if((0x80 & reg_val) == 0) // Is 7th bit unset?
            {
                cond_is_true = true;
            }
            break;
        }
        case kenbak_jmp_cond_pos_non_zero:
        {
            if((0x80 & reg_val) == 0 && (0x7F & reg_val) != 0)
            {
                cond_is_true = true;
            }
            break;
        }

        default:
        {
            assert(false); // Must not get here.
            return 0;
        }
    }

    if(cond_is_true)
    {
        d->state = kenbak_state_st; // Jump!
        assert(d->sig_inc == 255);
        d->sig_inc = 0;
        return 1;
    }

    d->state = kenbak_state_sa; // NO jump.
    assert(d->sig_inc == 255);
    d->sig_inc = 2;
    return 1;
}

/** Just waits for start button to be released.
 * 
 * - See page 37.
 */
static int step_in_qb(struct kenbak_data * const d)
{
    assert(d->state == kenbak_state_qb);

    if(d->sig_go)
    {
        return 1;
    }

    d->state = kenbak_state_sa;
    return 1;
}

/** The idle state (also entered, if the computer is halted).
 * 
 * - See page 36.
 */
static int step_in_qc(struct kenbak_data * const d)
{
    assert(d->state == kenbak_state_qc);

    d->sig_inc = 0; // Add zero bytes to P for first instruction on next run.

    d->reg_i = mem_read(d, KENBAK_DATA_ADDR_INPUT);

    // - The order is random here, not checked, in which "order" this works on
    //   the hardware..

    // X5 = EN or DA or DD (see page 25):
    //
    if(d->sig_en) // <= Store memory push button.
    {
        d->state = kenbak_state_qd;
        return 1;
    }
    if(d->sig_da) // <= Display address push button.
    {
        d->state = kenbak_state_qd;
        return 1;
    }
    if(d->sig_dd) // <= Read memory push button.
    {
        d->state = kenbak_state_qd;
        return 1;
    }

    if(d->sig_go) // <= Start push button.
    {
        d->state = kenbak_state_qb;
        return 1;
    }

    if(d->sig_ea) // <= Set address push button.
    {
        d->reg_w = d->reg_i;
    }
    assert(d->state == kenbak_state_qc);
    return 1;
}

/**
 * - See page 36.
 */
static int step_in_qd(struct kenbak_data * const d)
{
    assert(d->state == kenbak_state_qd);

    d->sig_r = d->reg_w;

    // In reality, waiting for CM, here.
    //
    d->state = kenbak_state_qe;
    return 1;
}

/**
 * - See page 36.
 */
static int step_in_qe(struct kenbak_data * const d)
{
    assert(d->state == kenbak_state_qe);
    assert(d->sig_r == d->reg_w);

    d->state = kenbak_state_qf; // State QF follows after one byte time.

    if(d->sig_en) // Enter data:
    {
        assert(!d->sig_da && !d->sig_dd); // See QC.

        mem_write(d, d->sig_r, d->reg_i); // Transfers I to memory.
        ++d->reg_w; // Adds 1 to W.
        return 1;
    }

    if(d->sig_da) // Display address:
    {
        assert(!d->sig_en && !d->sig_dd); // See QC.

        d->reg_k = d->sig_r; // Transfers W to K (content of R equals W, here).
        return 1;
    }

    // Display data:

    assert(d->sig_dd); // Because there is only the path QC -X5-> QD -CM-> QE.
    assert(!d->sig_en && !d->sig_da); // See QC.

    d->reg_k = mem_read(d, d->sig_r); // Transfers memory to K.
    ++d->reg_w; // Adds 1 to W.
    return 1;
}

/**
 * - See page 37.
 */
static int step_in_qf(struct kenbak_data * const d)
{
    assert(d->state == kenbak_state_qf);

    // The state register control waits at QF until the control buttons are
    // released (waiting for ^X5).
    //
    if(!d->sig_en && !d->sig_da && !d->sig_dd)
    {
        d->state = kenbak_state_qc;
    }
    return 1;
}

// *****************************************************************************
// *** PROCESSING OF A SINGLE "STEP"                                         ***
// *****************************************************************************

/**
 * - See page 06 of the logic schematics.
 */
static void update_x_signal(struct kenbak_data* const d)
{
    if (d->sig_da)
    {
        assert(!d->sig_dd);
        assert(d->state != kenbak_state_sa);
        assert(!d->sig_bu && !d->sig_cl);

        d->sig_x = kenbak_x_1;
        return;
    }
    if (d->sig_dd)
    {
        assert(!d->sig_da);
        assert(d->state != kenbak_state_sa);
        assert(!d->sig_bu && !d->sig_cl);

        d->sig_x = kenbak_x_2;
        return;
    }
    if (d->state == kenbak_state_sa)
    {
        assert(!d->sig_da);
        assert(!d->sig_dd);
        assert(!d->sig_bu && !d->sig_cl);

        d->sig_x = kenbak_x_3;
        return;
    }
    if (d->sig_bu || d->sig_cl)
    {
        assert(!d->sig_da);
        assert(!d->sig_dd);
        assert(d->state != kenbak_state_sa);

        d->sig_x = kenbak_x_4;
        return;
    }

    // Keep current X state, when getting here.
}

/** Update the content at address (octal) 377 by the current data buttons'
 *  states and the current state of the clear signal (as it is not possible to
 *  toggle the bits from 1 to 0 via data buttons, you need to clear ALL ones via
 *  the clear signal for that).
 */
static void update_input_byte(struct kenbak_data * const d)
{
    // Get state of the 8 data buttons into (octal) address 377 (this is the
    // serial signal BU) [page 22]:

    uint8_t val = 0, or_mask = 0;

    // (chose precedence of clear signal over data buttons, maybe wrong..)
    //
    if(d->sig_cl)
    {
        mem_write(d, KENBAK_DATA_ADDR_INPUT, 0);
        return;
    }
 
    // TODO: Skip the following, if !d->sig_bu?
    //
    for(int i = 0; i < KENBAK_INPUT_BITS; ++i)
    {
        if(d->input.buttons_data[i])
        {
            // Current bit's input button is currently pushed down.

            or_mask |= 1 << i; // Triggers enabling of bit value.
        }
    }
    val = mem_read(d, KENBAK_DATA_ADDR_INPUT);
    val |= or_mask;
    mem_write(d, KENBAK_DATA_ADDR_INPUT, val);
}

static void update_input_signals(struct kenbak_data * const d)
{
    // Update signals generated by pushed control buttons:
    //
    d->sig_bu = false;
    for (int i = 0; i < KENBAK_INPUT_BITS; ++i)
    {
        if (d->input.buttons_data[i])
        {
            d->sig_bu = true;
            break;
        }
    }
    d->sig_cl = d->input.but_input_clear;
    d->sig_da = d->input.but_address_display;
    d->sig_dd = d->input.but_memory_read;
    d->sig_ea = d->input.but_address_set;

    // Never disabling ED here to prevent overwrite, if cause is HALT
    // instruction and not the run stop button, also see step_in_sb():
    //
    d->sig_ed = d->sig_ed || d->input.but_run_stop;
    
    d->sig_en = d->input.but_memory_store;
    d->sig_go = d->input.but_run_start;
}

/**
 * - See logic schematics, page 07.
 */
static void update_reg_k(struct kenbak_data * const d)
{
    switch(d->sig_x)
    {
        case kenbak_x_none:
        {
            return; // Nothing to do.
        }
        case kenbak_x_1:
        {
            return; // Nothing to do, as already done via QE state handler.
        }
        case kenbak_x_2:
        {
            return; // Nothing to do, as already done via QE state handler.
        }
        case kenbak_x_3:
        {
            d->reg_k = mem_read(d, KENBAK_DATA_ADDR_OUTPUT);
            return;
        }
        case kenbak_x_4:
        {
            d->reg_k = mem_read(d, KENBAK_DATA_ADDR_INPUT);
            return;
        }

        default:
        {
            assert(false);
            return;
        }
    }
}

/**
 * - If clear or console data push buttons are depressed during run mode, the
 *   real Kenbak-1 will display the contents of location 128 as a faint
 *   background light (see page 20 of the programming reference manual)!
 */
static void update_output(struct kenbak_data * const d)
{
    // See logic schematics, page 06:
    //
    d->output.led_address_set = d->sig_x == kenbak_x_1;
    d->output.led_memory_store = d->sig_x == kenbak_x_2;
    d->output.led_input_clear = d->sig_x == kenbak_x_4;
    d->output.led_run_stop = d->state != kenbak_state_qc;

    d->output.led_bit_0 = ((d->reg_k >> 0) & 1) == 1;
    d->output.led_bit_1 = ((d->reg_k >> 1) & 1) == 1;
    d->output.led_bit_2 = ((d->reg_k >> 2) & 1) == 1;
    d->output.led_bit_3 = ((d->reg_k >> 3) & 1) == 1;
    d->output.led_bit_4 = ((d->reg_k >> 4) & 1) == 1;
    d->output.led_bit_5 = ((d->reg_k >> 5) & 1) == 1;
    d->output.led_bit_6 = ((d->reg_k >> 6) & 1) == 1;
    d->output.led_bit_7 = ((d->reg_k >> 7) & 1) == 1;
}

static void update_input_signals_byte_and_x(struct kenbak_data * const d)
{
    update_input_signals(d);
    update_input_byte(d);
    update_x_signal(d);
}

/**
 * - To be called, if Kenbak-1 is in a defined state and a step shall be taken.
 */
static int step_in_defined_state(struct kenbak_data * const d)
{
    assert(d->state != kenbak_state_power_off);
    assert(d->state != kenbak_state_unknown);

    int c = 0;

    switch(d->state)
    {
        case kenbak_state_sa: // SL, SN, SS, SV, SY or SZ -> SA
        {
            update_input_signals_byte_and_x(d);

            c = step_in_sa(d);
            break;
        }
        case kenbak_state_sb: // SA -CM-> SB
        {
            c = step_in_sb(d);
            break;
        }
        case kenbak_state_sc: // SB -^ED-> SC
        {
            c = step_in_sc(d);
            break;
        }
        case kenbak_state_sd: // SC -CM-> SD
        {
            c = step_in_sd(d); // I <- Next instr. first byte.
            break;
        }
        case kenbak_state_se: // SD -I3+I2-> SE (I2+I1 in emulator..)
        {
            c = step_in_se(d); // Gets here for TWO byte instructions.
            break;
        }
        case kenbak_state_sf: // SE -JI+IND-> SF
        {
            c = step_in_sf(d);
            break;
        }
        case kenbak_state_sg: // SF -CM-> SG
        {
            c = step_in_sg(d);
            break;
        }
        case kenbak_state_sh: // ... -> SH
        {
            c = step_in_sh(d);
            break;
        }
        case kenbak_state_sj: // SH -CM-> SJ
        {
            c = step_in_sj(d);
            break;
        }
        case kenbak_state_sk: // ... -> SK
        {
            c = step_in_sk(d);
            break;
        }
        case kenbak_state_sl: // SK -CM-> SL
        {
            c = step_in_sl(d);
            break;
        }
        case kenbak_state_sm: // ... -> ...
        {
            c = step_in_sm(d);
            break;
        }
        case kenbak_state_sn: // SM -^TM*^J*CM-> SN or ST-JP CM-> SN
        {
            c = step_in_sn(d);
            break;
        }
        case kenbak_state_sp: // SM -TM*CM-> SP
        {
            c = step_in_sp(d);
            break;
        }
        // TODO: Implement SQ!
        case kenbak_state_sr: // SP, SQ -> SR
        {
            c = step_in_sr(d);
            break;
        }
        case kenbak_state_ss: // SR -CM-> SS
        {
            c = step_in_ss(d);
            break;
        }
        case kenbak_state_st: // SZ -JC-> ST
        {
            c = step_in_st(d);
            break;
        }
        case kenbak_state_su: // SD -^I3*^I2-> SU (^I2*^I1 in emulator..)
        {
            c = step_in_su(d); // Gets here for ONE byte instructions.
            break;
        }
        case kenbak_state_sv: // SU -CM-> SV
        {
            c = step_in_sv(d);
            break;
        }
        case kenbak_state_sw: // SV -IO-> SW
        {
            c = step_in_sw(d);
            break;
        }
        case kenbak_state_sx: // SW -1-> SX
        {
            c = step_in_sx(d);
            break;
        }
        case kenbak_state_sy: // SX -CM-> SY
        {
            c = step_in_sy(d);
            break;
        }
        case kenbak_state_sz: // SM -> SZ
        {
            c = step_in_sz(d);
            break;
        }

        case kenbak_state_qb: // QC -GO-> QB
        {
            update_input_signals_byte_and_x(d);

            c = step_in_qb(d);
            break;
        }
        case kenbak_state_qc: // SB -ED-> QC or QF -^X5-> QC
        {
            update_input_signals_byte_and_x(d);

            c = step_in_qc(d);
            break;
        }
        case kenbak_state_qd: // QC -X5-> QD
        {
            c = step_in_qd(d);
            break;
        }
        case kenbak_state_qe: // QD -CM-> QE
        {
            c = step_in_qe(d);
            break;
        }
        case kenbak_state_qf: // QE -1-> QF
        {
            update_input_signals_byte_and_x(d);

            c = step_in_qf(d);
            break;
        }

        case kenbak_state_power_off: // (falls through)
        case kenbak_state_unknown: // (falls through)
        default:
        {
            assert(false);
            return -1; // Error!
        }
    }

    update_reg_k(d);
    update_output(d);
    assert(0 <= c);
    return c;
}

int kenbak_emu_step(struct kenbak_data * const d)
{
    if(d->state == kenbak_state_power_off)
    {
        if(!d->input.switch_power_on)
        {
            return 0;
        }

        d->state = kenbak_state_unknown; // Must be handled below.
    }

    // Kenbak-1 is powered-on.

    if(!d->input.switch_power_on)
    {
        // Power-off:

        init(d);
        assert(d->state == kenbak_state_power_off);
        return 0;
    }

    // Power toggle is (still) in ON position.

    if(d->state == kenbak_state_unknown)
    {
        d->state = kenbak_state_qc;
    }

    // Kenbak-1 is in a defined state.

    return step_in_defined_state(d);
}

// *****************************************************************************
// *** CREATION AND DELETION OF A KENBAK-1'S STATE REPRESENTATION            ***
// *****************************************************************************

void kenbak_emu_delete(struct kenbak_data * const d)
{
    if(d == NULL)
    {
        return; // Just do nothing.
    }
    free(d);
}

struct kenbak_data * kenbak_emu_create(bool const randomize_memory)
{
    struct kenbak_data * const d = malloc(sizeof *d);

    if(d == NULL)
    {
        assert(false); // Must not get here.
        return NULL;
    }

    d->randomize_memory = randomize_memory;

    init(d);

    return d;
}
