
// Marcel Timm, RhinoDevel, 2025oct23

#include <stdlib.h>
#include <string.h>
#include <assert.h>

#include "kenbak_asm.h"

// *****************************************************************************
// *** Notes:                                                                ***
// *****************************************************************************

// Special memory locations:
//
//   0 = A "register".
//   1 = B "register".
//   2 = X "register".
//   3 = P "register".
// 
// 128 = Output "register".
// 129 = "Register" A's overflow (bit 0) and carry (bit 1) flags.
// 130 = "Register" B's overflow (bit 0) and carry (bit 1) flags.
// 131 = "Register" X's overflow (bit 0) and carry (bit 1) flags.
//
// 255 = Input "register".

// *****************************************************************************
// *** Error messages and their lengths:                                     ***
// *****************************************************************************

static char const s_err_not_implemented[] = "Not implemented!";
static int const s_err_len_not_implemented = sizeof s_err_not_implemented;

// *****************************************************************************
// *** Functions:                                                            ***
// *****************************************************************************

static void clear_bytes(uint8_t * const bytes, int const byte_count)
{
	// Although would work (see loop, below):
	assert(bytes != NULL);
	assert(0 < byte_count);

	for(int i = 0; i < byte_count; ++i)
	{
		bytes[i] = 0;
	}
}

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

	uint8_t mem[256]; // Represents the Kenbak-1's (whole) memory.
	int pos = 0; // The location counter (initially set to zero automatically).

	clear_bytes(mem, (int)(sizeof mem));

	{
		assert(sizeof **out_msg == 1);
		*out_msg = malloc(s_err_len_not_implemented);
		assert(*out_msg != NULL);
		strcpy_s(*out_msg, s_err_len_not_implemented, s_err_not_implemented);
	}

	return NULL;
}
