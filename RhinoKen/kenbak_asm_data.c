
// Marcel Timm, RhinoDevel, 2025nov02

#include <assert.h>
#include <stdlib.h>
#include <string.h>

#include "kenbak_asm_data.h"
#include "kenbak_asm_constant.h"

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

struct kenbak_asm_data * kenbak_asm_data_create(
	char const * const txt, int const txt_len)
{
	assert(txt != NULL);
	assert(0 <= txt_len);

	struct kenbak_asm_data * const ret_val = malloc(sizeof *ret_val);

	assert(ret_val != NULL);

	struct kenbak_asm_data buf = (struct kenbak_asm_data){
		.txt = txt,
		.txt_len = txt_len,

		.first_constant = NULL,

		.txt_pos = 0,
		.consumed = 0,

		//mem
		.mem_loc = 0
	};
	clear_bytes(buf.mem, (int)(sizeof buf.mem));

	memcpy(ret_val, &buf, sizeof *ret_val);

	return ret_val;
}

void kenbak_asm_data_free(struct kenbak_asm_data * const data)
{
	kenbak_asm_constant_free(data->first_constant); // (OK, if NULL)

	free(data);
}
