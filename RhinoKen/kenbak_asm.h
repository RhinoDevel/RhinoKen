
// Marcel Timm, RhinoDevel, 2025oct23

#ifndef KENBAK_ASM
#define KENBAK_ASM

#include <stdint.h>

/**
 * - Returns NULL on error.
 * - Caller takes ownership of returned bytes.
 */
uint8_t* kenbak_asm_exec(char const * const txt, size_t const txt_len);

#endif //KENBAK_ASM