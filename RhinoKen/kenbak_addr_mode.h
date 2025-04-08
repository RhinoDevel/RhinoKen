
// Marcel Timm, RhinoDevel, 2024may26

#ifndef KENBAK_ADDR_MODE
#define KENBAK_ADDR_MODE

// See PRM, page 4 and 24.
// 
// - The values equal the bit pattern of the last three instruction bits for the
//   addressing mode of add/sub/load/store and or/and/lneg instructions.
// - Bit manipulation instructions have only the memory addressing mode.
// - Jump/branch instructions have memory (called direct) addressing and
//   indirect addressing.
// 
// - Shifts/rotates and miscellaneous (halt and noop) are one byte, only.
//   <=> Have no addressing mode.
//
enum kenbak_addr_mode
{
    kenbak_addr_mode_none = 0,

    kenbak_addr_mode_constant = 3,
    kenbak_addr_mode_memory = 4,
    kenbak_addr_mode_indirect = 5,
    kenbak_addr_mode_indexed = 6,
    kenbak_addr_mode_indirect_indexed = 7
};

#endif //KENBAK_ADDR_MODE