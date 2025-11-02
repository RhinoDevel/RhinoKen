
// Marcel Timm, RhinoDevel, 2025oct23

#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <stdbool.h>
#include <ctype.h>
#include <stdio.h>

#include "kenbak_asm.h"
#include "kenbak_asm_constant.h"
#include "kenbak_asm_data.h"

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
// *** Error message stuff:                                                  ***
// *****************************************************************************

static char const * const s_err_prefix_part_one = "ERROR: Pos. ";
static char const * const s_err_prefix_part_two = ": ";

// *****************************************************************************
// *** Allowed name characters and stuff (for constants and labels):         ***
// *****************************************************************************

#define MT_NAME_MAX_LEN 16 // For simplicity.

static char const s_name_chars_allowed_all[] = {
	'a', 'b', 'c', 'd', 'e', 'f', 'g', 'h', 'i', 'j', 'k', 'l', 'm', 'n',
	'o', 'p', 'q', 'r', 's', 't', 'u', 'v', 'w', 'x', 'y', 'z',

	'_'
};

static char const s_name_chars_allowed_following[] = {
	'0', '1', '2', '3', '4', '5', '6', '7', '8', '9'
};

static char const s_constant_sep = '=';

static char const s_val_oct = '0';
static char const s_val_hex = '$';

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
 * - Caller takes ownership of returned string.
 */
static char * create_err_msg(int const pos, char const * const msg)
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

	char * const ret_val = malloc(len * sizeof * ret_val);

	if(ret_val == NULL)
	{
		assert(false); // Must not happen.
		return NULL;
	}

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
	assert(txt_pos != NULL && 0 <= *txt_pos && *txt_pos <= txt_len);

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
	assert(txt_pos != NULL && 0 <= *txt_pos && *txt_pos <= txt_len);

	static char const begin = ';';
	static char const end = '\n'; // In addition to reached text end.

	int ret_val = 0;

	if(*txt_pos == txt_len)
	{
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
 * - Caller takes ownership of returned string.
 * - Function will increase the given text position only, if a name was found.
 * - Returns NULL and *out_err_msg == NULL, if no name is found.
 * - Returns NULL and *out_err_msg != NULL, if an error occurred.
 */
static char * try_read_name(
	char const * const txt,
	int const txt_len,
	int * const txt_pos,
	char * * const out_err_msg)
{
	assert(txt != NULL);
	assert(0 <= txt_len);
	assert(txt_pos != NULL && 0 <= *txt_pos && *txt_pos <= txt_len);
	assert(out_err_msg != NULL && *out_err_msg == NULL);

	char buf[MT_NAME_MAX_LEN];
	int buf_pos = 0;
	char cur_char = '\0';
	int txt_pos_buf = *txt_pos;

	if(txt_pos_buf == txt_len)
	{
		create_err_msg(txt_pos_buf, "End of text, expected beginning of name!");
		return NULL;
	}

	cur_char = txt[txt_pos_buf];

	if(get_index_of(
		s_name_chars_allowed_all,
		(int)(sizeof s_name_chars_allowed_all),
		cur_char) == -1)
	{
		assert(*out_err_msg == NULL);
		return NULL; // NOT an error (just no name found).
	}

	++txt_pos_buf;
	buf[buf_pos/*0*/] = cur_char;
	++buf_pos;

	while(txt_pos_buf < txt_len)
	{
		cur_char = txt[txt_pos_buf];

		if(get_index_of(
			s_name_chars_allowed_all,
			(int)(sizeof s_name_chars_allowed_all),
			cur_char) == -1
			&& get_index_of(
				s_name_chars_allowed_following,
				(int)(sizeof s_name_chars_allowed_following),
				cur_char) == -1)
		{
			break; // Reached next other character behind valid name.
		}

		// Another valid name character found.

		if(buf_pos == (int)(sizeof buf))
		{
			*out_err_msg = create_err_msg(txt_pos_buf, "Name is too long!");
			return NULL;
		}

		// Fits into buffer.

		++txt_pos_buf;
		buf[buf_pos] = cur_char;
		++buf_pos;
	}

	assert(0 < buf_pos);

	char * const ret_val = malloc((buf_pos + 1) * sizeof *ret_val);

	assert(ret_val != NULL);

	for(int i = 0; i < buf_pos; ++i)
	{
		ret_val[i] = buf[i];
	}
	ret_val[buf_pos] = '\0';

	*txt_pos = txt_pos_buf;
	return ret_val;
}

/**
 * - Function will increase the given text position only, if a value was found.
 * - Returns count of characters consumed.
 * - Also consumes any whitespace and/or comments behind the value!
 * - Returns -1 and *out_err_msg != NULL, if an error occurred, caller takes
 *   ownership of that message string.
 */
static int read_val(
	char const * const txt,
	int const txt_len,
	int * const txt_pos,
	uint8_t * const out_val,
	char * * const out_err_msg)
{
	assert(txt != NULL);
	assert(0 <= txt_len);
	assert(txt_pos != NULL && 0 <= *txt_pos && *txt_pos <= txt_len);
	assert(out_val != NULL);
	assert(out_err_msg != NULL && *out_err_msg == NULL);

	int consumed_buf = 0;
	int txt_pos_buf = *txt_pos;
	char cur_char = '\0';
	uint8_t val = 0;

	if(txt_pos_buf == txt_len)
	{
		// No more text.

		*out_err_msg = create_err_msg(
			txt_pos_buf, "No more input text, expected value of constant!");
		return -1;
	}

	cur_char = txt[txt_pos_buf];
	++txt_pos_buf;
	if(cur_char == s_val_oct)
	{
		int fac = 64;

		do
		{
			int cur_val = 0;
			cur_char = txt[txt_pos_buf];
			++txt_pos_buf;
			++consumed_buf;

			if(cur_char < '0')
			{
				*out_err_msg = create_err_msg(
					txt_pos_buf, "Invalid octal digit detected (must be at least 0)!");
				return -1;
			}
			if(fac == 64)
			{
				if('3' < cur_char)
				{
					*out_err_msg = create_err_msg(
						txt_pos_buf, "Invalid octal digit detected (must be at most 3)!");
					return -1;
				}
			}
			else
			{
				if('7' < cur_char)
				{
					*out_err_msg = create_err_msg(
						txt_pos_buf, "Invalid octal digit detected (must be at most 7)!");
					return -1;
				}
			}
			
			//   2|   1 |   0
			// 8^2| 8^1 | 8^0
			//  64|   8 |   1

			cur_val = (int)(cur_char);
			cur_val -= (int)'0';
			cur_val *= fac;
			assert(cur_val < UINT8_MAX);
			val += (uint8_t)cur_val;
			if(fac == 1)
			{
				break;
			}
			fac /= 8;
		} while(true);
	}
	else
	{
		if(cur_char == s_val_hex)
		{
			*out_err_msg = create_err_msg( // TODO: Implement!
				txt_pos_buf, "Hexadecimal values are not implemented, yet!");
			return -1;
		}
		else
		{
			*out_err_msg = create_err_msg( // TODO: Implement!
				txt_pos_buf, "Decimal values are not implemented, yet!");
			return -1;
		}
	}

	int const wsc_count = consume_whitespaces_and_comments(
			txt, txt_len, &txt_pos_buf);

	if(wsc_count == 0)
	{
		*out_err_msg = create_err_msg(
			txt_pos_buf,
			"Expected white-space and/or comment after read value!");
		return -1;
	}
	consumed_buf += wsc_count;

	*txt_pos = txt_pos_buf;
	*out_val = val;
	return consumed_buf;
}

/**
 * - Also consumes all whitespaces and comments in-between name and value.
 * - Also consumes whitespaces and comments after last found value.
 * - Function will increase the given text position only, if a name was found.
 * - Returns count of characters consumed.
 * - Returns 0 and *out_err_msg == NULL, if no name is found at beginning.
 * - Returns -1 and *out_err_msg != NULL, if an error occurred, caller takes
 *   ownership of that message string.
 * - Caller takes ownership of kenbak_asm_constant->name, if set here (not done
 *   on error of if no constant found).
 */
static int try_read_constant(
	char const * const txt,
	int const txt_len,
	int * const txt_pos,
	struct kenbak_asm_constant * const out_constant,
	char * * const out_err_msg)
{
	assert(txt != NULL);
	assert(0 <= txt_len);
	assert(txt_pos != NULL && 0 <= *txt_pos && *txt_pos <= txt_len);
	assert(out_constant != NULL);
	assert(out_err_msg != NULL && *out_err_msg == NULL);

	char* name = NULL;
	uint8_t val = 0;
	int consumed_buf = 0;
	int txt_pos_buf = *txt_pos;

	name = try_read_name(txt, txt_len, &txt_pos_buf, out_err_msg);
	if(name == NULL)
	{
		if(*out_err_msg != NULL)
		{
			return -1; // An error occurred!
		}
		return 0; // Just no constant (name) found.
	}
	consumed_buf += (int)strlen(name);
	assert(0 < strlen(name));

	// The name of a constant was found.

	consumed_buf += consume_whitespaces_and_comments(
		txt, txt_len, &txt_pos_buf);

	if(txt_pos_buf == txt_len)
	{
		// No more text.

		free(name);
		name = NULL;

		*out_err_msg = create_err_msg(
			txt_pos_buf,
			"No more input text, expected value of constant (or something else)!");
		return -1;
	}

	// There is more after the parsed name.

	if(txt[txt_pos_buf] != s_constant_sep)
	{
		// Does not seem to be a constant. => Back to before the read name.
		
		free(name);
		name = NULL;

		return 0; // Not an error, just no constant found.
	}
	consumed_buf += 1;
	++txt_pos_buf;

	// Separator found and consumed.

	consumed_buf += consume_whitespaces_and_comments(
		txt, txt_len, &txt_pos_buf);

	// Maybe existing comments and/or whitespace after separator consumed.

	int const consumed_val = read_val(
			txt, txt_len, &txt_pos_buf, &val, out_err_msg);

	if(consumed_val == -1)
	{
		assert(*out_err_msg != NULL);
		free(name);
		name = NULL;
		return -1;
	}
	if(consumed_val == 0)
	{
		assert(*out_err_msg == NULL);
		free(name);
		name = NULL;
		*out_err_msg = create_err_msg(
			txt_pos_buf, "Expected value of constant was not found!");
		return -1;
	}
	consumed_buf += consumed_val;
	
	assert(*out_err_msg == NULL);
	*txt_pos = txt_pos_buf;
	out_constant->name = name; // Takes ownership.
	name = NULL;
	out_constant->val = val;
	out_constant->next = NULL;
	return consumed_buf;
}

/**
 * - Also consumes all whitespaces and comments in-between constants.
 * - Given text position is the position of the first character to check.
 * - Function will increase the given text position only, if at last one
 *   constant was found.
 * - Returns count of characters consumed.
 * - Caller also takes ownership of returned message, if not NULL.
 * - Caller takes ownership of returned linked list of constants, if not NULL.
 * - Returns -1 on error.
 */
static int read_constants(
	char const * const txt,
	int const txt_len,
	int * const txt_pos,
	struct kenbak_asm_constant * * const out_first_constant,
	char * * const out_err_msg)
{
	assert(txt != NULL);
	assert(0 <= txt_len);
	assert(txt_pos != NULL && 0 <= *txt_pos && *txt_pos <= txt_len);
	assert(out_first_constant != NULL && *out_first_constant == NULL);
	assert(out_err_msg != NULL && *out_err_msg == NULL);

	int constant_count = 0;
	int consumed_chars = 0;
	int txt_pos_buf = *txt_pos;
	struct kenbak_asm_constant * first_constant = kenbak_asm_constant_create();
	struct kenbak_asm_constant * cur_constant = first_constant;
	struct kenbak_asm_constant * prev_constant = NULL;

	do
	{
		int const constant_consumed = try_read_constant(
				txt, txt_len, &txt_pos_buf, cur_constant, out_err_msg);

		if(constant_consumed == 0)
		{
			if(prev_constant != NULL)
			{
				// Prune the last (unused) node:
				assert(prev_constant->next == cur_constant);
				prev_constant->next = NULL;
				kenbak_asm_constant_free(cur_constant);
				cur_constant = prev_constant;
				prev_constant = NULL; // Kind of wrong, but OK..
			}
			break;
		}
		if(constant_consumed == -1)
		{
			assert(*out_err_msg != NULL);
			kenbak_asm_constant_free(first_constant);
			return -1;
		}

		++constant_count;
		consumed_chars += constant_consumed;
		
		// May gets pruned during next iteration, if no more found:
		struct kenbak_asm_constant * const next = kenbak_asm_constant_create();

		cur_constant->next = next;
		prev_constant = cur_constant;
		cur_constant = next;
	} while(true);

	if(constant_count == 0)
	{
		assert(*out_err_msg == NULL);
		kenbak_asm_constant_free(first_constant);
		first_constant = NULL;
		return 0;
	}
	*txt_pos = txt_pos_buf;
	*out_first_constant = first_constant; // Takes ownership.
	first_constant = NULL;
	return consumed_chars;
}

uint8_t* kenbak_asm_exec(
	char const * const txt,
	int const txt_len,
	int * const out_len,
	char * * const out_msg)
{
	assert(out_len != NULL);
	assert(out_msg != NULL);

	struct kenbak_asm_data * data = kenbak_asm_data_create(txt, txt_len);

	assert(data->consumed == 0);
	data->consumed += consume_whitespaces_and_comments(
		data->txt, data->txt_len, &(data->txt_pos));
	
	int const consumed_read_constants = read_constants(
			data->txt,
			data->txt_len,
			&(data->txt_pos),
			&(data->first_constant),
			out_msg);

	if(consumed_read_constants == -1)
	{
		assert(*out_msg != NULL);
		kenbak_asm_data_free(data);
		return NULL;
	}

	kenbak_asm_constant_print(data->first_constant);

	// TODO: Implement!
	*out_msg = create_err_msg(data->txt_pos, "Not implemented!");

	kenbak_asm_data_free(data);
	return NULL;
}
