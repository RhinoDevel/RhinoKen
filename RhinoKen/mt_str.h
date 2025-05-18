
// Marcel Timm, RhinoDevel, 2025may17

#ifndef MT_STR
#define MT_STR

#include <assert.h>
#include <stdint.h>

/**
 * - Does nothing (debug assertion, only), if NULL given as buffer pointer or
 *   given buffer length is insufficient (must be at least 3 + 1).
 */
void mt_str_fill_with_octal(
	char * const buf, size_t const buf_len, uint8_t const val);

/**
 * - Does nothing (debug assertion, only), if NULL given as buffer pointer or
 *   given buffer length is insufficient (must be at least 8 + 1).
 */
void mt_str_fill_with_binary(
	char * const buf, size_t const buf_len, uint8_t const val);

#endif //MT_STR