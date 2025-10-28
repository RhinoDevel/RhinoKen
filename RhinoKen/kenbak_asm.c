
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
// *** Error messages:                                                       ***
// *****************************************************************************

static char const * const s_err_prefix_part_one = "ERROR: Pos. ";
static char const * const s_err_prefix_part_two = ": ";

static char const * const s_err_expected_constant = "Expected constant!";
static char const * const s_err_constant_name_too_long = "Constant name too long!";
static char const * const s_err_not_implemented = "Not implemented!";

// *****************************************************************************
// *** Functions:                                                            ***
// *****************************************************************************

static int get_dec_digit_count(unsigned int const val)
{
	int ret_val = 0;
	unsigned int n = val;

	do
	{
		n /= 10;
		++ret_val;
	}while(0 < n);

	return ret_val;
}

/**
 * - Caller takes ownership of returned string.
 */
static char * create_msg(int const pos, char const * const msg)
{
	assert(msg != NULL);
	assert(0 <= pos); // <=> Position must be smaller than UINT_MAX.

	size_t const len_prefix_part_one = strlen(s_err_prefix_part_one);
	size_t const len_pos = get_dec_digit_count((unsigned int)pos);
	size_t const len_prefix_part_two = strlen(s_err_prefix_part_two);
	size_t const len_msg = strlen(msg);
	size_t const len =
		len_prefix_part_one
		+ len_pos
		+ len_prefix_part_two
		+ len_msg
		+ 1;

	char * const ret_val = malloc(len * sizeof *ret_val);
	
	assert(ret_val != NULL);
	
	sprintf_s(
		ret_val,
		len,
		"%s%d%s%s",
		s_err_prefix_part_one,
		pos,
		s_err_prefix_part_two,
		msg);

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

static int get_index_of(
	char const * const arr, int const arr_len, char const val)
{
	// Although would work (see loop, below):
	assert(arr != NULL);
	assert(0 < arr_len);

	for(int i = 0; i < arr_len; ++i)
	{
		if(arr[i] == val)
		{
			return i;
		}
	}
	return -1;
}

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

/**
 * - Also consumes all whitespaces and comments in-between constants.
 * - Given text position is the position of the first character to check.
 * - Function will increase the given text position only, if at last one
 *   constant was found.
 * - Returns count of characters consumed.
 * - Caller also takes ownership of returned message, if not NULL.
 * - Returns -1 on error.
 */
static int read_constants(
	char const * const txt,
	int const txt_len,
	int * const txt_pos,
	char * * const out_msg)
{
	static char const allowed_all[] = {
		'a', 'b', 'c', 'd', 'e', 'f', 'g', 'h', 'i', 'j', 'k', 'l', 'm', 'n',
		'o', 'p', 'q', 'r', 's', 't', 'u', 'v', 'w', 'x', 'y', 'z',

		'_'
	};
	static char const allowed_following[] = {
		'0', '1', '2', '3', '4', '5', '6', '7', '8', '9'
	};

	int ret_val = 0;

	do
	{
		char name_buf[32 + 1];
		char name_pos = 0;
		char cur_char = txt[*txt_pos];

		if(get_index_of(allowed_all, (int)(sizeof allowed_all), cur_char) == -1)
		{
			*out_msg = create_msg(*txt_pos, s_err_expected_constant);
			return -1;
		}
		name_buf[name_pos] = cur_char;
		++name_pos;

		if(name_pos == (int)(sizeof name_buf))
		{
			*out_msg = create_msg(*txt_pos, s_err_constant_name_too_long);
			return -1;
		}

		// TODO: Implement!

		int const wsc_cnt = consume_whitespaces_and_comments(
				txt, txt_len, txt_pos);

		if(wsc_cnt == 0)
		{
			break;
		}
		ret_val += wsc_cnt;
	} while(true);

	return ret_val;
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

	int consumed = 0; // For error checking.

	clear_bytes(mem, (int)(sizeof mem));

	consumed += consume_whitespaces_and_comments(txt, txt_len, &txt_pos);
	
	int const consumed_read_constants = read_constants(
			txt, txt_len, &txt_pos, out_msg);

	if(consumed_read_constants == -1)
	{
		assert(*out_msg != NULL);
		return NULL;
	}

	assert(consumed == txt_len);

	// TODO: Implement!
	*out_msg = create_msg(txt_pos, s_err_not_implemented);
	return NULL;
}
