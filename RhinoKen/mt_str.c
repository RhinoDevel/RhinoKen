
// Marcel Timm, RhinoDevel, 2025may17

#include <assert.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdint.h>

#include "mt_str.h"

void mt_str_fill_with_hex(
	char * const buf, size_t const buf_len, uint8_t const val)
{
	if(buf == NULL)
	{
		assert(false);
		return;
	}
	if(buf_len < 2 + 1)
	{
		assert(false);
		return;
	}
	snprintf(buf, buf_len, "%02X", (int)val);
}

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
	int shifted_val = 0;

	if(buf == NULL)
	{
		assert(false);
		return;
	}
	if(buf_len < 8 + 1)
	{
		assert(false);
		return;
	}

	shifted_val = val;
	for(int i = 7; 0 <= i; --i)
	{
		buf[i] = '0' + (shifted_val & 1);

		shifted_val >>= 1;
	}

	buf[8] = '\0';
}