
// Marcel Timm, RhinoDevel, 2025oct23

#ifndef KENBAK_ASM
#define KENBAK_ASM

#include <stdint.h>

/**
 * - Returns NULL on error.
 * - Caller takes ownership of returned bytes.
 * - Assumes that all three input pointers are NOT NULL.
 * - Caller also takes ownership of returned message, if not NULL.
 */
uint8_t* kenbak_asm_exec(
	char const * const txt,
	int const txt_len,
	int * const out_len,
	char * * const out_msg);

#endif //KENBAK_ASM