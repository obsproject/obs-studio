#pragma once

/* Architecture of a capture target process. On Windows on ARM a 64-bit process
 * may be either native ARM64 or emulated x64, and each needs its own hook DLL,
 * inject helper and graphics offsets, so a plain 32/64-bit distinction is not
 * enough. */
enum process_arch { PROCESS_ARCH_X86, PROCESS_ARCH_X64, PROCESS_ARCH_ARM64 };

/* Filename suffix used by the per-architecture helper files, e.g.
 * graphics-hook<suffix>.dll and inject-helper<suffix>.exe. */
static inline const char *process_arch_suffix(enum process_arch arch)
{
	switch (arch) {
	case PROCESS_ARCH_ARM64:
		return "-arm64";
	case PROCESS_ARCH_X64:
		return "64";
	default:
		return "32";
	}
}

/* The architecture this build of OBS runs as. */
static inline enum process_arch obs_process_arch(void)
{
#if defined(_M_ARM64) || defined(_M_ARM64EC)
	return PROCESS_ARCH_ARM64;
#elif defined(_WIN64)
	return PROCESS_ARCH_X64;
#else
	return PROCESS_ARCH_X86;
#endif
}
