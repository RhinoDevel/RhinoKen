
// Marcel Timm, RhinoDevel, 2024may06

#include <stdio.h>
#include <assert.h>

#include "kenbak_emu.h"
#include "kenbak_data.h"

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

static uint32_t get_ms()
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

// *****************************************************************************

static void print_at(int const x, int const y, char const c)
{
	set_cursor_pos(x, y);
	printf("%c", c);
}

static void print_kenbak()
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
static void print_keys()
{
	printf("     ^ ^  ^ ^  ^  ^ ^ ^     ^        ^     ^     ^     ^       ^    ^" "\n");
	printf("     | |  | |  |  | | |     |        |     |     |     |       |    |" "\n");
	printf("     1 2  3 4  5  6 7 8     D        F     G     H     J       K    L" "\n");
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

int main()
{
	struct kenbak_data * const d = kenbak_emu_create();
	uint32_t last = 0;

	set_cursor_visibility(false);

	print_kenbak();
	print_keys();

	d->input.switch_power_on = true;

// TODO: Debugging:
//
#ifndef NDEBUG
	{
		int i = KENBAK_DATA_ADDR_P;

		d->delay_line_0[i++] = 0x04; // P = 4
		assert(i == 0x04);
		d->delay_line_0[i++] = 0x80; // NOOP
		d->delay_line_0[i++] = 0x13; // 00010011 LOAD immediate/constant to A.
		d->delay_line_0[i++] = 0xAA; // 10101010
		d->delay_line_0[i++] = 0xCA; // 11001010 SKIP ON BIT 1 BEING 1.
		d->delay_line_0[i++] = 0x00; // Address of A.
		d->delay_line_0[i++] = 0x00; // HALT (shall be skipped).
		d->delay_line_0[i++] = 0x00; // HALT (shall be skipped).

		d->delay_line_0[i++] = 0xC9; // 11001001 SHIFT A 1 TO THE LEFT.
		                             // => A = 01010101
		d->delay_line_0[i++] = 0x7A; // 01111010 SET BIT 7 TO 1
		d->delay_line_0[i++] = 0x00; // Address of A.
		                             // => A = 11010101

		d->delay_line_0[i++] = 0xAA; // 10101010 SKIP ON BIT 5 BEING 0.
		d->delay_line_0[i++] = 0x00; // Address of A.
		d->delay_line_0[i++] = 0x00; // HALT (shall be skipped).
		d->delay_line_0[i++] = 0x00; // HALT (shall be skipped).

		d->delay_line_0[i++] = 0x32; // 00110010 SET BIT 6 TO 0.
		d->delay_line_0[i++] = 0x00; // Address of A.
								     // => A = 10010101
		d->delay_line_0[i++] = 0xD3; // 11010011 AND.
		d->delay_line_0[i++] = 0xC9; // 11001001
		                             // => A = 10000001
		d->delay_line_0[i++] = 0xC3; // 11000011 OR.
		d->delay_line_0[i++] = 0x98; // 10011000
		                             // => A = 10011001
		d->delay_line_0[i++] = 0x0B; // 00001011 SUB immediate from A.
		d->delay_line_0[i++] = 0x99; // 10011001
		                             // => A = 00000000
		d->delay_line_0[i++] = 0x03; // 00000011 ADD immediate to A.
		d->delay_line_0[i++] = 0x01; // => A = 00000001
		d->delay_line_0[i++] = 0xDC; // 11011100 LNEG memory A to A.
		d->delay_line_0[i++] = KENBAK_DATA_ADDR_A; // => A = 11111111
		d->delay_line_0[i++] = 0x1B; // 00011011 STORE immediate from A.
		d->delay_line_0[i++] = KENBAK_DATA_ADDR_OUTPUT;
		d->delay_line_0[i++] = 0x00; // HALT
	}
#endif //NDEBUG

	kenbak_emu_step(d);

	// The "game" loop:
	//
	last = get_ms();
	do
	{
		uint32_t const cur = get_ms(), cur_interval = cur - last;

		if(cur_interval < MT_UPDATE_INTERVAL_MS)
		{
			continue; // Wait a little longer.
		}

		// TODO: Do NOT assume this, but replace MT_STEPS_PER_FRAME below with
		//       a dynamic value instead to process the amount of steps that
		//       fits into cur_interval!
		//
		// There is NO guarantee about this when running in a multitasking
		// environment:
		//
		//assert((double)cur_interval / (double)MT_UPDATE_INTERVAL_MS < 1.5);

		last = cur;

		// Get user input:
		//
		if(is_key_down('Q'))
		{
			break; // => Exit emulation.
		}
		kenbak_emu_init_input(d, true);
		update_input(&d->input);

		// Let the emulator do the work that a real Kenbak-1 computer can do in
		// the update interval timespan MT_UPDATE_INTERVAL_MS:
		//
		for(int i = 0; i < MT_STEPS_PER_FRAME; ++i)
		{
			kenbak_emu_step(d);
		}

		// Update output:
		//
		print_leds(&d->output);
	} while(true);

	set_cursor_visibility(true);
	set_cursor_pos(0, 11);
	kenbak_emu_delete(d);
	printf("\n" ">>> Long live the Kenbak-1! <<<" "\n");
	return 0;
}