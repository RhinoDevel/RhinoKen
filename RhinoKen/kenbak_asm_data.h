
// Marcel Timm, RhinoDevel, 2025nov02

#ifndef KENBAK_ASM_DATA
#define KENBAK_ASM_DATA

#include <stdint.h>

#include "kenbak_asm_constant.h"

struct kenbak_asm_data
{
	char const * const txt;
	int const txt_len;
	
	struct kenbak_asm_constant * first_constant;

	int txt_pos; // Position in the input text file.
	int consumed; // For error checking.

	uint8_t mem[256]; // Represents the Kenbak-1's (whole) memory.
	int mem_loc; // The location counter (initially set to 0 automatically).
};

/**
 * - Does NOT take ownership of given text pointer.
 */
struct kenbak_asm_data * kenbak_asm_data_create(
	char const * const txt, int const txt_len);

/**
 * - Also frees constants (but not the source text pointer).
 */
void kenbak_asm_data_free(struct kenbak_asm_data * const data);

#endif //KENBAK_ASM_DATA
