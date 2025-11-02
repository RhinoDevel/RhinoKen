
// Marcel Timm, RhinoDevel, 2025nov02

#include "kenbak_asm_constant.h"

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>

struct kenbak_asm_constant * kenbak_asm_constant_create()
{
	struct kenbak_asm_constant * const ret_val = malloc(sizeof *ret_val);

	assert(ret_val != NULL);

	ret_val->name = NULL;
	ret_val->val = 0;
	ret_val->next = NULL;

	return ret_val;
}

void kenbak_asm_constant_free(struct kenbak_asm_constant * const first_constant)
{
	struct kenbak_asm_constant * cur_constant = first_constant;

	while(cur_constant != NULL)
	{
		struct kenbak_asm_constant * next = cur_constant->next;

		free(cur_constant->name);
		cur_constant->name = NULL;

		free(cur_constant);
		cur_constant = next;
	}
}

void kenbak_asm_constant_print(
	struct kenbak_asm_constant * const first_constant)
{
	struct kenbak_asm_constant * cur_constant = first_constant;

	while(cur_constant != NULL)
	{
		printf("%s = %d\n", cur_constant->name, (int)cur_constant->val);

		cur_constant = cur_constant->next;
	}
}
