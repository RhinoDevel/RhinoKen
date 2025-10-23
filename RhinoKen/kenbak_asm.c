
// Marcel Timm, RhinoDevel, 2025oct23

#include <stdint.h>
#include <assert.h>

#include "kenbak_asm.h"

uint8_t* kenbak_asm_exec(
	char const * const txt,
	int const txt_len,
	char * * const out_msg,
	int * const out_msg_len)
{
	assert(txt != NULL);
	assert(0 <= txt_len);
	assert(out_msg != NULL);
	assert(out_msg_len != NULL);

	return NULL;
}
