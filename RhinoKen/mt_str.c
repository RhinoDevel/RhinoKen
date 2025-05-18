
// Marcel Timm, RhinoDevel, 2025may17

#include <assert.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdint.h>

#include "mt_str.h"

void mt_str_fill_with_octal(
	char * const buf, size_t const buf_len, uint8_t const val)
{
	if(buf == NULL)
	{
		assert(false);
		return;
	}
	if(buf_len < 3 + 1)
	{
		assert(false);
		return;
	}
	snprintf(buf, buf_len, "%03o", (int)val);
}

void mt_str_fill_with_binary(
	char * const buf, size_t const buf_len, uint8_t const val)
{
	int i = -1, max_i = 8, shifted_val = val;

	if(buf == NULL)
	{
		assert(false);
		return;
	}
	if(buf_len < 1)
	{
		assert(false);
		return;
	}

	if(buf_len - 1 < max_i)
	{
		assert(false);
		max_i = (int)buf_len - 1;
	}

	for(i = max_i - 1; 0 <= i; --i)
	{
		buf[i] = '0' + (shifted_val & 1);

		shifted_val >>= 1;
	}
	assert(i == -1);
	buf[max_i] = '\0';
}