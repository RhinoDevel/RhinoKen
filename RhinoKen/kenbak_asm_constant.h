
// Marcel Timm, RhinoDevel, 2025nov02

#ifndef KENBAK_ASM_CONSTANT
#define KENBAK_ASM_CONSTANT

#include <stdint.h>

struct kenbak_asm_constant
{
	char* name;
	uint8_t val;
	struct kenbak_asm_constant * next;
};

struct kenbak_asm_constant * kenbak_asm_constant_create();

/**
 * - Also frees next and all following constants.
 * - Assumes that the name property needs to be freed, too.
 */
void kenbak_asm_constant_free(
	struct kenbak_asm_constant * const first_constant);

/**
 * - Also prints next and all following constants.
 */
void kenbak_asm_constant_print(
	struct kenbak_asm_constant * const first_constant);

#endif //KENBAK_ASM_CONSTANT