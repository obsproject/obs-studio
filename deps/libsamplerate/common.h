/*
** Copyright (C) 2002-2011 Erik de Castro Lopo <erikd@mega-nerd.com>
**
** This program is free software; you can redistribute it and/or modify
** it under the terms of the GNU General Public License as published by
** the Free Software Foundation; either version 2 of the License, or
** (at your option) any later version.
**
** This program is distributed in the hope that it will be useful,
** but WITHOUT ANY WARRANTY; without even the implied warranty of
** MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
** GNU General Public License for more details.
**
** You should have received a copy of the GNU General Public License
** along with this program; if not, write to the Free Software
** Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307, USA.
*/

/*
** This code is part of Secret Rabbit Code aka libsamplerate. A commercial
** use license for this code is available, please see:
**		http://www.mega-nerd.com/SRC/procedure.html
*/

#ifndef COMMON_H_INCLUDED
#define COMMON_H_INCLUDED

#include <stdint.h>

#define	SRC_MAX_RATIO			256
#define	SRC_MAX_RATIO_STR		"256"

#define	SRC_MIN_RATIO_DIFF		(1e-20)

#define	MAX(a,b)	(((a) > (b)) ? (a) : (b))
#define	MIN(a,b)	(((a) < (b)) ? (a) : (b))

#define	ARRAY_LEN(x)			((int) (sizeof (x) / sizeof ((x) [0])))
#define OFFSETOF(type,member)	((int) (&((type*) 0)->member))

#define	MAKE_MAGIC(a,b,c,d,e,f)	((a) + ((b) << 4) + ((c) << 8) + ((d) << 12) + ((e) << 16) + ((f) << 20))

/*
** Inspiration : http://sourcefrog.net/weblog/software/languages/C/unused.html
*/
#ifdef UNUSED
#elif defined (__GNUC__)
#	define UNUSED(x) UNUSED_ ## x __attribute__ ((unused))
#elif defined (__LCLINT__)
#	define UNUSED(x) /*@unused@*/ x
#else
#	define UNUSED(x) x
#endif

#ifdef __GNUC__
#	define WARN_UNUSED	__attribute__ ((warn_unused_result))
#else
#	define WARN_UNUSED
#endif


#include "samplerate.h"

enum
{	SRC_FALSE	= 0,
	SRC_TRUE	= 1,

	SRC_MODE_PROCESS	= 555,
	SRC_MODE_CALLBACK	= 556
} ;

enum
{	SRC_ERR_NO_ERROR = 0,

	SRC_ERR_MALLOC_FAILED,
	SRC_ERR_BAD_STATE,
	SRC_ERR_BAD_DATA,
	SRC_ERR_BAD_DATA_PTR,
	SRC_ERR_NO_PRIVATE,
	SRC_ERR_BAD_SRC_RATIO,
	SRC_ERR_BAD_PROC_PTR,
	SRC_ERR_SHIFT_BITS,
	SRC_ERR_FILTER_LEN,
	SRC_ERR_BAD_CONVERTER,
	SRC_ERR_BAD_CHANNEL_COUNT,
	SRC_ERR_SINC_BAD_BUFFER_LEN,
	SRC_ERR_SIZE_INCOMPATIBILITY,
	SRC_ERR_BAD_PRIV_PTR,
	SRC_ERR_BAD_SINC_STATE,
	SRC_ERR_DATA_OVERLAP,
	SRC_ERR_BAD_CALLBACK,
	SRC_ERR_BAD_MODE,
	SRC_ERR_NULL_CALLBACK,
	SRC_ERR_NO_VARIABLE_RATIO,
	SRC_ERR_SINC_PREPARE_DATA_BAD_LEN,

	/* This must be the last error number. */
	SRC_ERR_MAX_ERROR
} ;

typedef struct SRC_PRIVATE_tag
{	double	last_ratio, last_position ;

	int		error ;
	int		channels ;

	/* SRC_MODE_PROCESS or SRC_MODE_CALLBACK */
	int		mode ;

	/* Pointer to data to converter specific data. */
	void	*private_data ;

	/* Varispeed process function. */
	int		(*vari_process) (struct SRC_PRIVATE_tag *psrc, SRC_DATA *data) ;

	/* Constant speed process function. */
	int		(*const_process) (struct SRC_PRIVATE_tag *psrc, SRC_DATA *data) ;

	/* State reset. */
	void	(*reset) (struct SRC_PRIVATE_tag *psrc) ;

	/* Data specific to SRC_MODE_CALLBACK. */
	src_callback_t	callback_func ;
	void			*user_callback_data ;
	long			saved_frames ;
	float			*saved_data ;
} SRC_PRIVATE ;

/* In src_sinc.c */
const char* sinc_get_name (int src_enum) ;
const char* sinc_get_description (int src_enum) ;

int sinc_set_converter (SRC_PRIVATE *psrc, int src_enum) ;

/* In src_linear.c */
const char* linear_get_name (int src_enum) ;
const char* linear_get_description (int src_enum) ;

int linear_set_converter (SRC_PRIVATE *psrc, int src_enum) ;

/* In src_zoh.c */
const char* zoh_get_name (int src_enum) ;
const char* zoh_get_description (int src_enum) ;

int zoh_set_converter (SRC_PRIVATE *psrc, int src_enum) ;

/*----------------------------------------------------------
**	Common static inline functions.
*/

static inline double
fmod_one (double x)
{	double res ;

	res = x - lrint (x) ;
	if (res < 0.0)
		return res + 1.0 ;

	return res ;
} /* fmod_one */

#endif	/* COMMON_H_INCLUDED */
