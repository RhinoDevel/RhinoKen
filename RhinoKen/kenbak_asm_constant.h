
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

#endif //KENBAK_ASM_CONSTANT