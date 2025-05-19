
// Marcel Timm, RhinoDevel, 2024may06

#include <stdio.h>
#include <assert.h>
#include <stdint.h>

#include "mt_str.h"

#include "kenbak_instr.h"
#include "kenbak_emu.h"
#include "kenbak_data.h"

// See kenbak_emu.h:
//
#define MT_FPS 25
#define MT_UPDATE_INTERVAL_MS (1000 / MT_FPS)
#define MT_INSTRUCTIONS_PER_SEC 480
#define MT_STEPS_PER_INSTRUCTION 10 // A guessed average value.
#define MT_STEPS_PER_SEC (MT_INSTRUCTIONS_PER_SEC * MT_STEPS_PER_INSTRUCTION)
#define MT_STEPS_PER_FRAME (MT_STEPS_PER_SEC / MT_FPS)

// *****************************************************************************
// *** WINDOWS-SPECIFIC                                                      ***
// *****************************************************************************

#include <windows.h>
#include <conio.h>

static uint32_t get_ms(void)
{
	static LARGE_INTEGER freq;
	static LARGE_INTEGER start;
	static bool is_init = false;

	LARGE_INTEGER now;

	if(!is_init)
	{
		QueryPerformanceFrequency(&freq); // Ignoring return value..
		QueryPerformanceCounter(&start); // Ignoring return value..
		is_init = true;
	}

	QueryPerformanceCounter(&now); // Ignoring return value..

	LONGLONG const offset_pc = now.QuadPart - start.QuadPart;

	return (uint32_t)(1000 * offset_pc / freq.QuadPart);
}

static bool is_key_down(char const key)
{
	return (GetAsyncKeyState((int)key) & 0x8000) != 0;
}

static void set_cursor_pos(int const x, const int y)
{
	HANDLE const h_console = GetStdHandle(STD_OUTPUT_HANDLE);
	COORD const pos = { x, y };

	SetConsoleCursorPosition(h_console, pos); // Return value ignored..
}

static void set_cursor_visibility(bool const is_visible)
{
	HANDLE const h_console = GetStdHandle(STD_OUTPUT_HANDLE);
	CONSOLE_CURSOR_INFO cursor_info;

	GetConsoleCursorInfo(h_console, &cursor_info); // Return value ignored..
	cursor_info.bVisible = (BOOL)is_visible;
	SetConsoleCursorInfo(h_console, &cursor_info); // Return value ignored..
}

static char wait_for_key_presses(char const a, char const b, char const c)
{
	while(true)
	{
		int const ch = _getch();

		if(ch == (int)a)
		{
			return a;
		}
		if(ch == (int)b)
		{
			return b;
		}
		if(ch == (int)c)
		{
			return c;
		}
	}
}

// *****************************************************************************

static void print_at(int const x, int const y, char const c)
{
	set_cursor_pos(x, y);
	printf("%c", c);
}

/**
 * - Acts as if given string always fits in one line from given X position on!
 * - Does NOT clear the line before write!
 */
static int print_str_at(int const x, int const y, char const * const str)
{
	assert(str != NULL);
	
	int i = -1;

	while(str[++i] != '\0')
	{
		print_at(x + i, y, str[i]);
	}
	return i;
}

static int print_byte_at(
	int const x, int const y, char const title, uint8_t const val)
{
	char buf[15 + 1];
	int i = 0;

	//int const buf_len = sizeof buf / sizeof *buf;

	buf[i++] = title;
	buf[i++] = ':';
	buf[i++] = ' ';

	mt_str_fill_with_octal(buf + i, 3 + 1, val);
	i += 3;

	buf[i++] = ' ';

	mt_str_fill_with_binary(buf + i, 8 + 1, val);
	i += 8;

	return print_str_at(x, y, buf);
}

static void print_kenbak(void)
{
	printf( "  /-----------------------------------------------------------------------------\\" "\n");
	printf( " /   7 6  5 4  3  2 1 0                                                          \\" "\n");
	printf( "/    O O  O O  O  O O O     Y              Y     C-o   Y            Y             \\" "\n");
	printf( "|                                            LOCK UNL                             |" "\n");
	printf( "|   <2 1><4 2  1><4 2 1> -INPUT- ---ADDRESS--- ---MEMORY--- -----RUN---- ON       |" "\n");
	printf("\\   <8 4  2 1><8  4 2 1>  CLEAR   DISPLAY SET   READ STORE   START STOP  U POWER  /" "\n");
    printf(" \\   b b  b b  w  w w w     b        w     b     w     b       w    b    |       /" "\n");
	printf("  \\-----------------------------------------------------------------------------/" "\n");
}
static void print_keys(void)
{
	printf("     ^ ^  ^ ^  ^  ^ ^ ^     ^        ^     ^     ^     ^       ^    ^" "\n");
	printf("     | |  | |  |  | | |     |        |     |     |     |       |    |" "\n");
	printf("     1 2  3 4  5  6 7 8     D        F     G     H     J       K    L" "\n");

	printf("\n");
	printf("A = Enable step mode." "\n");
	printf("Y = Disable step mode." "\n");
	printf("X = Next step (if step mode is enabled)." "\n");
}
static void print_leds(struct kenbak_output const * const o)
{
	int x = 5, y = 2;

	print_at(x, y, o->led_bit_7 ? '1' : '0');
	x += 2;
	print_at(x, y, o->led_bit_6 ? '1' : '0');
	x += 3;
	print_at(x, y, o->led_bit_5 ? '1' : '0');
	x += 2;
	print_at(x, y, o->led_bit_4 ? '1' : '0');
	x += 3;
	print_at(x, y, o->led_bit_3 ? '1' : '0');
	x += 3;
	print_at(x, y, o->led_bit_2 ? '1' : '0');
	x += 2;
	print_at(x, y, o->led_bit_1 ? '1' : '0');
	x += 2;
	print_at(x, y, o->led_bit_0 ? '1' : '0');

	x += 6;

	print_at(x, y, o->led_input_clear ? '1' : '0');

	x += 15;

	print_at(x, y, o->led_address_set ? '1' : '0');

	x += 12;

	print_at(x, y, o->led_memory_store ? '1' : '0');

	x += 13;

	print_at(x, y, o->led_run_stop ? '1' : '0');
}

/**
 * - Assumes all (relevant) input values of given object already being set to
 *   false.
 * 
 * - Ignores all pressed keys, but one (precedence is kind of random..).
 */
static void update_input(struct kenbak_input * const input)
{
	for(int i = 0; i <= 7; ++i)
	{
		if(is_key_down('1' + 7 - i)) // <=> '8' to '1'.
		{
			input->buttons_data[i] = true;
			return;
		}
	}

	if(is_key_down('D'))
	{
		input->but_input_clear = true;
		return;
	}

	if(is_key_down('F'))
	{
		input->but_address_display = true;
		return;
	}
	if(is_key_down('G'))
	{
		input->but_address_set = true;
		return;
	}

	if(is_key_down('H'))
	{
		input->but_memory_read = true;
		return;
	}
	if(is_key_down('J'))
	{
		input->but_memory_store = true;
		return;
	}

	if(is_key_down('K'))
	{
		input->but_run_start = true;
		return;
	}
	if(is_key_down('L'))
	{
		input->but_run_stop = true;
		return;
	}
}

int main(void)
{
	struct kenbak_data * const d = kenbak_emu_create(true);
	uint32_t last = 0;
	bool stepMode = false;

	set_cursor_visibility(false);

	print_kenbak();
	print_keys();

	d->input.switch_power_on = true;

// TODO: Debugging:
//
#ifndef NDEBUG
	{
		int i = KENBAK_DATA_ADDR_P;

		// Own: Rotate a bit, with inner delay loop (countdown)
		// 
		// *=3
		//
		d->delay_line_0[i++] = 0004; //  3 004 P = 4
		d->delay_line_0[i++] = 0023; //  4 023 LOAD-A constant
		d->delay_line_0[i++] = 0200; //  5 - constant -
		d->delay_line_0[i++] = 0311; //  6 311 ROTATE_LEFT_1-A
		d->delay_line_0[i++] = 0034; //  7 034 STORE-A memory
		d->delay_line_0[i++] = 0200; //  8 - address -
		d->delay_line_0[i++] = 0223; //  9 223 LOAD-X constant
		d->delay_line_0[i++] = 0040; // 10 - constant -
		d->delay_line_0[i++] = 0213; // 11 213 SUB-X constant
		d->delay_line_0[i++] = 0001; // 12 - constant -
		d->delay_line_0[i++] = 0243; // 13 243 JPD-X != 0
		d->delay_line_0[i++] = 0013; // 14 - address -
		d->delay_line_0[i++] = 0343; // 15 343 JPD-Unc. "!= 0"
		d->delay_line_0[i++] = 0006; // 16 - address -

		// EX 3-1
		//
		// *=3
		//
		//d->delay_line_0[i++] = 0004; // 3 004 P = 4
		//assert(i == 0x04);
		//d->delay_line_0[i++] = 0023; // 4 023 A LOAD constant
		//d->delay_line_0[i++] = 0000; // 5 000
		//d->delay_line_0[i++] = 0034; // 6 034 A STORE memory
		//d->delay_line_0[i++] = 0200; // 7 200
		//d->delay_line_0[i++] = 0000; // 8 000 HALT 
		//d->delay_line_0[i++] = 0003; // 9 003 A ADD constant
		//d->delay_line_0[i++] = 0001; // A 001
		//d->delay_line_0[i++] = 0344; // B 344 Unconditional JPD
		//d->delay_line_0[i++] = 0006; // C 006
	}
#endif //NDEBUG

	kenbak_emu_step(d);

	// The "game" loop:
	//
	last = get_ms();
	do
	{
		uint32_t const cur = get_ms(), cur_interval = cur - last;

		if(!stepMode && cur_interval < MT_UPDATE_INTERVAL_MS)
		{
			continue; // Wait a little longer.
		}

		last = cur;

		// Get user input:
		//
		if(is_key_down('Q'))
		{
			break; // => Exit emulation (1/2).
		}
		kenbak_emu_init_input(d, true);
		update_input(&d->input);

		if(stepMode)
		{
			kenbak_emu_step(d); // (returned byte time is unused..)
			
			char const pressed_key = wait_for_key_presses(
				'y', // Exit step mode.
				'x', // Next step. 
				'q'); // Exit emulation (2/2).

			if(pressed_key == 'y') // Hard-coded
			{
				stepMode = false;
			}
			else
			{
				if(pressed_key == 'q') // Hard-coded
				{
					break; // => Exit emulation.
				}

				assert(pressed_key == 'x'); // Hard-coded
			}
		}
		else
		{
			if(is_key_down('A'))
			{
				stepMode = true;
			}

			// Let the emulator do the work that a real Kenbak-1 computer can do
			// in the current update interval timespan:
			//
			uint32_t const frames_per_cur_interval =
				cur_interval / MT_UPDATE_INTERVAL_MS;
			//
			for(uint32_t f = 0; f < frames_per_cur_interval; ++f)
			{
				for(int s = 0; s < MT_STEPS_PER_FRAME; ++s)
				{
					kenbak_emu_step(d); // (returned byte time is unused..)
				}
			}
		}

		// Update output:
		//
		print_leds(&d->output);

		print_str_at(0, 16, kenbak_state_get_str(d->state));

		// Overdone (printing everything each time):
		//
		print_str_at(3, 16, stepMode ? "[x] Step mode" : "[ ] Step mode");

		print_byte_at(0, 18, 'A', d->delay_line_0[KENBAK_DATA_ADDR_A]);
		print_byte_at(0, 19, 'B', d->delay_line_0[KENBAK_DATA_ADDR_B]);
		print_byte_at(0, 20, 'X', d->delay_line_0[KENBAK_DATA_ADDR_X]);
		print_byte_at(0, 21, 'P', d->delay_line_0[KENBAK_DATA_ADDR_P]);

		// TODO: Implement correctly:
		//
		{
			char buf[81];
			int const buf_len = sizeof buf / sizeof *buf;
			uint8_t const first_byte_addr = d->delay_line_0[KENBAK_DATA_ADDR_P],
				second_byte_addr = (first_byte_addr + 1) % 256; // TODO: Test! Even necessary?

			kenbak_instr_fill_str(
				buf,
				buf_len,
				*kenbak_get_mem_ptr(d, first_byte_addr),
				*kenbak_get_mem_ptr(d, second_byte_addr));

			print_str_at(
				16, // Hard-coded
				21,
				buf);
		}

		print_byte_at(0, 23, 'W', d->reg_w);
		print_byte_at(0, 24, 'I', d->reg_i);
	} while(true);

	set_cursor_visibility(true);
	set_cursor_pos(0, 11);
	kenbak_emu_delete(d);
	printf("\n" ">>> Long live the Kenbak-1! <<<" "\n");
	return 0;
}