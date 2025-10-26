
// Marcel Timm, RhinoDevel, 2025oct23

#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <stdbool.h>
#include <ctype.h>

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

/**
 * - Given text position is the position of the first character to check.
 * - Function will increment the given text position only, if a whitespace was
 *   found.
 * - Returns count of whitespace characters consumed.
 */
static int consume_whitespace(
	char const * const txt, int const txt_len, int * const txt_pos)
{
	assert(txt != NULL);
	assert(0 <= txt_len);
	assert(txt_pos != NULL);
	assert(0 <= *txt_pos);
	assert(*txt_pos <= txt_len);

	int ret_val = 0;

	while(*txt_pos < txt_len)
	{
		if(!isspace((unsigned char)txt[*txt_pos]))
		{
			break;
		}
		++ret_val;
		++(*txt_pos);
	}
	return ret_val;
}

/**
 * - A comment starts with a ';' and ends with a '\n'.
 * - Given text position is the position of the first character to check.
 * - Function will increase the given text position only, if a comment was
 *   found.
 * - Returns count of comment characters consumed.
 */
static int consume_comment(
	char const * const txt, int const txt_len, int * const txt_pos)
{
	assert(txt != NULL);
	assert(0 <= txt_len);
	assert(txt_pos != NULL);
	assert(0 <= *txt_pos);
	assert(*txt_pos <= txt_len);

	static char const begin = ';';
	static char const end = '\n'; // In addition to reached text end.

	int ret_val = 0;

	if(*txt_pos == txt_len)
	{
		assert(*txt_pos == 0); // (would just do nothing, otherwise)
		return ret_val/*0*/;
	}

	if(txt[*txt_pos] != begin)
	{
		return ret_val/*0*/; // No comment at current position.
	}

	// A comment starts at current position.

	++ret_val/*ret_val = 1*/;
	++(*txt_pos); // Go to next character (or end).

	while(*txt_pos < txt_len)
	{
		char const cur_char = txt[*txt_pos];

		++ret_val;
		++(*txt_pos); // Always go to next character, here.

		if(txt[*txt_pos] == end)
		{
			break; // End-comment character found, done.
		}
	}
	return ret_val;
}

/**
 * - Consumes all whitespaces and comments (order is unimportant), until
 *   something else is detected.
 * - Given text position is the position of the first character to check.
 * - Function will increase the given text position only, if whitespaces and/or
 *   comments were found.
 * - Returns count of characters consumed.
 */
static int consume_whitespaces_and_comments(
	char const * const txt, int const txt_len, int * const txt_pos)
{
	int ret_val = 0;

	do
	{
		int const cur_ws_count = consume_whitespace(txt, txt_len, txt_pos);

		if(ret_val != 0 // If return value is 0, no comment check was done, yet.
			&& cur_ws_count == 0)
		{
			break; // Done.
		}
		ret_val += cur_ws_count;

		int const cur_comment_count = consume_comment(txt, txt_len, txt_pos);

		if(cur_comment_count == 0)
		{
			break; // Done.
		}
		ret_val += cur_comment_count;
	} while(true);

	return ret_val;
}

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
	int mem_loc = 0; // The location counter (initially set to 0 automatically).
	
	int txt_pos = 0; // Position in the input text file.

	clear_bytes(mem, (int)(sizeof mem));

	consume_whitespaces_and_comments(txt, txt_len, &txt_pos);

	{
		assert(sizeof **out_msg == 1);
		*out_msg = malloc(s_err_len_not_implemented);
		assert(*out_msg != NULL);
		strcpy_s(*out_msg, s_err_len_not_implemented, s_err_not_implemented);
	}

	return NULL;
}
