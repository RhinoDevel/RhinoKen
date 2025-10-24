
// Marcel Timm, RhinoDevel, 2025oct23

#include <stdlib.h>
#include <string.h>
#include <assert.h>

#include "kenbak_asm.h"

static char const s_err_not_implemented[] = "Not implemented!";
static int const s_err_len_not_implemented = sizeof s_err_not_implemented;

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

	{
		assert(sizeof(**out_msg) == 1);
		*out_msg = malloc(s_err_len_not_implemented);
		assert(*out_msg != NULL);
		strcpy_s(*out_msg, s_err_len_not_implemented, s_err_not_implemented);
	}

	return NULL;
}
