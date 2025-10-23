
// Marcel Timm, RhinoDevel, 2025oct23

#include <stdint.h>
#include <assert.h>

#include "kenbak_asm.h"

static char const * const s_err_not_implemented = "Not implemented!";
static int const s_err_len_not_implemented = sizeof s_err_len_not_implemented;

uint8_t* kenbak_asm_exec(
	char const * const txt,
	int const txt_len,
	int * const out_len,
	char * * const out_msg)
{
	assert(txt != NULL);
	assert(0 <= txt_len);
	assert(out_len != NULL);
	assert(out_msg != NULL);

	return NULL;
}
