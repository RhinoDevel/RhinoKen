
// Marcel Timm, RhinoDevel, 2025apr23

#ifndef KENBAK_JMP_COND
#define KENBAK_JMP_COND

// See PRM, page 9.
// 
// - The values equal the three least significant bits' patterns of the
//   available jump (and mark) instructions.
//
enum kenbak_jmp_cond
{
	kenbak_jmp_cond_non_zero = 3, // 011 Non-zero
	kenbak_jmp_cond_zero = 4, // 100 Zero
	kenbak_jmp_cond_neg = 5, // 101 Negative
	kenbak_jmp_cond_pos = 6, // 110 Positive
	kenbak_jmp_cond_pos_non_zero = 7 // 111 Positive Non-zero
};

#endif //KENBAK_JMP_COND