///////////////////////////////////////////////////////////////////////////////
//
/// \file       tuklib_cpucores.c
/// \brief      Get the number of CPU cores online
//
//  Author:     Lasse Collin
//
//  This file has been put into the public domain.
//  You can do whatever you want with this file.
//
///////////////////////////////////////////////////////////////////////////////

#include "tuklib_cpucores.h"

#if defined(TUKLIB_CPUCORES_SYSCTL)
#	ifdef HAVE_SYS_PARAM_H
#		include <sys/param.h>
#	endif
#	include <sys/sysctl.h>

#elif defined(TUKLIB_CPUCORES_SYSCONF)
#	include <unistd.h>

// HP-UX
#elif defined(TUKLIB_CPUCORES_PSTAT_GETDYNAMIC)
#	include <sys/param.h>
#	include <sys/pstat.h>
#endif


extern uint32_t
tuklib_cpucores(void)
{
	uint32_t ret = 0;

#if defined(TUKLIB_CPUCORES_SYSCTL)
	int name[2] = { CTL_HW, HW_NCPU };
	int cpus;
	size_t cpus_size = sizeof(cpus);
	if (sysctl(name, 2, &cpus, &cpus_size, NULL, 0) != -1
			&& cpus_size == sizeof(cpus) && cpus > 0)
		ret = cpus;

#elif defined(TUKLIB_CPUCORES_SYSCONF)
#	ifdef _SC_NPROCESSORS_ONLN
	// Most systems
	const long cpus = sysconf(_SC_NPROCESSORS_ONLN);
#	else
	// IRIX
	const long cpus = sysconf(_SC_NPROC_ONLN);
#	endif
	if (cpus > 0)
		ret = cpus;

#elif defined(TUKLIB_CPUCORES_PSTAT_GETDYNAMIC)
	struct pst_dynamic pst;
	if (pstat_getdynamic(&pst, sizeof(pst), 1, 0) != -1)
		ret = pst.psd_proc_cnt;
#endif

	return ret;
}
