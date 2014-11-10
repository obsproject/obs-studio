#include <windows.h>
#include <stdlib.h>
#include "funchook.h"

#define JMP_64_SIZE            14
#define JMP_32_SIZE            5

#define X86_NOP                0x90
#define X86_JMP_NEG_5          0xF9EB

static inline void fix_permissions(void *addr, size_t size)
{
	DWORD protect_val;
	VirtualProtect(addr, size, PAGE_EXECUTE_READWRITE, &protect_val);
}

void hook_init(struct func_hook *hook,
		void *func_addr, void *hook_addr, const char *name)
{
	memset(hook, 0, sizeof(*hook));

	hook->func_addr = (uintptr_t)func_addr;
	hook->hook_addr = (uintptr_t)hook_addr;
	hook->name = name;

	fix_permissions((void*)(hook->func_addr - JMP_32_SIZE),
			JMP_64_SIZE + JMP_32_SIZE);

	memcpy(hook->unhook_data, func_addr, JMP_64_SIZE);
}

static inline size_t patch_size(struct func_hook *hook)
{
	return hook->is_64bit_jump ? JMP_64_SIZE : JMP_32_SIZE;
}

static const uint8_t longjmp64[6] = {0xFF, 0x25, 0x00, 0x00, 0x00, 0x00};

static inline void rehook64(struct func_hook *hook)
{
	uint8_t data[JMP_64_SIZE];
	uintptr_t *ptr_loc = (uintptr_t*)((uint8_t*)data + sizeof(longjmp64));

	fix_permissions((void*)hook->func_addr, JMP_64_SIZE);

	memcpy(data, (void*)hook->func_addr, JMP_64_SIZE);
	memcpy(data, longjmp64, sizeof(longjmp64));
	*ptr_loc = hook->hook_addr;

	hook->call_addr = (void*)hook->func_addr;
	hook->type = HOOKTYPE_FORWARD_OVERWRITE;
	hook->hooked = true;

	memcpy((void*)hook->func_addr, data, JMP_64_SIZE);
}

static inline void hook_reverse_new(struct func_hook *hook, uint8_t *p)
{
	hook->call_addr = (void*)(hook->func_addr + 2);
	hook->type = HOOKTYPE_REVERSE_CHAIN;
	hook->hooked = true;

	p[0] = 0xE9;
	*((uint32_t*)&p[1]) = (uint32_t)(hook->hook_addr - hook->func_addr);
	*((uint16_t*)&p[5]) = X86_JMP_NEG_5;
}

static inline void hook_reverse_chain(struct func_hook *hook, uint8_t *p)
{
	if (hook->type != HOOKTYPE_FORWARD_OVERWRITE)
		return;

	hook->call_addr = (void*)(hook->func_addr + *((int32_t*)&p[1]));
	hook->type = HOOKTYPE_REVERSE_CHAIN;
	hook->hooked = true;

	*((uint32_t*)&p[1]) = (uint32_t)(hook->hook_addr - hook->func_addr);
}

static inline void hook_forward_chain(struct func_hook *hook, uint8_t *p,
		intptr_t offset)
{
	int32_t cur_offset = *(int32_t*)&p[6];

	if (hook->type != HOOKTYPE_FORWARD_OVERWRITE)
		return;

	hook->call_addr = (void*)(hook->func_addr + JMP_32_SIZE + cur_offset);
	hook->type = HOOKTYPE_FORWARD_CHAIN;
	hook->hooked = true;

	*((int32_t*)&p[6]) = (int32_t)offset;
}

static inline void hook_forward_overwrite(struct func_hook *hook,
		intptr_t offset)
{
	uint8_t *ptr = (uint8_t*)hook->func_addr;

	hook->call_addr = (void*)hook->func_addr;
	hook->type = HOOKTYPE_FORWARD_OVERWRITE;
	hook->hooked = true;

	*(ptr++) = 0xE9;
	*((int32_t*)ptr) = (int32_t)offset;
}

static inline void rehook32(struct func_hook *hook, bool force, intptr_t offset)
{
	fix_permissions((void*)(hook->func_addr - JMP_32_SIZE),
			JMP_32_SIZE * 2);

	if (force || !hook->started) {
		uint8_t *p        = (uint8_t*)hook->func_addr - JMP_32_SIZE;
		size_t  nop_count = 0;

		/* check for reverse chain hook availability */
		for (size_t i = 0; i < JMP_32_SIZE; i++) {
			if (p[i] == X86_NOP)
				nop_count++;
		}

		if (nop_count == JMP_32_SIZE && p[5] == 0x8B && p[6] == 0xFF) {
			hook_reverse_new(hook, p);

		} else if (p[0] == 0xE9 && *(uint16_t*)&p[5] == X86_JMP_NEG_5) {
			hook_reverse_chain(hook, p);

		} else if (p[5] == 0xE9) {
			hook_forward_chain(hook, p, offset);

		} else if (hook->type != HOOKTYPE_FORWARD_OVERWRITE) {
			hook->type = HOOKTYPE_FORWARD_OVERWRITE;
		}

		hook->started = true;
	}

	if (hook->type == HOOKTYPE_FORWARD_OVERWRITE) {
		hook_forward_overwrite(hook, offset);
	}
}

void do_hook(struct func_hook *hook, bool force)
{
	intptr_t offset;

	/* chained hooks do not unhook */
	if (!force && hook->hooked)
		return;

	/* if the hook is a forward overwrite hook, copy back the memory that
	 * was previously encountered to preserve any new hooks on top */
	if (hook->started && !force &&
	    hook->type == HOOKTYPE_FORWARD_OVERWRITE) {
		memcpy((void*)hook->func_addr, hook->rehook_data,
				patch_size(hook));
		hook->hooked = true;
		return;
	}

	offset = hook->hook_addr - hook->func_addr - JMP_32_SIZE;

#ifdef _WIN64
	hook->is_64bit_jump = (llabs(offset) >= 0x7fffffff);

	if (hook->is_64bit_jump) {
		rehook64(hook);
		return;
	}
#endif

	rehook32(hook, force, offset);
}

void unhook(struct func_hook *hook)
{
	size_t size;

	/* chain hooks do not need to unhook */
	if (!hook->hooked || hook->type != HOOKTYPE_FORWARD_OVERWRITE)
		return;

	size = patch_size(hook);
	fix_permissions((void*)hook->func_addr, size);
	memcpy(hook->rehook_data, (void*)hook->func_addr, size);
	memcpy((void*)hook->func_addr, hook->unhook_data, size);

	hook->hooked = false;
}
