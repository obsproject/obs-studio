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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "config.h"
#include "float_cast.h"
#include "common.h"

static int zoh_vari_process (SRC_PRIVATE *psrc, SRC_DATA *data) ;
static void zoh_reset (SRC_PRIVATE *psrc) ;

/*========================================================================================
*/

#define	ZOH_MAGIC_MARKER	MAKE_MAGIC ('s', 'r', 'c', 'z', 'o', 'h')

typedef struct
{	int		zoh_magic_marker ;
	int		channels ;
	int		reset ;
	long	in_count, in_used ;
	long	out_count, out_gen ;
	float	last_value [1] ;
} ZOH_DATA ;

/*----------------------------------------------------------------------------------------
*/

static int
zoh_vari_process (SRC_PRIVATE *psrc, SRC_DATA *data)
{	ZOH_DATA 	*priv ;
	double		src_ratio, input_index, rem ;
	int			ch ;

	if (data->input_frames <= 0)
		return SRC_ERR_NO_ERROR ;

	if (psrc->private_data == NULL)
		return SRC_ERR_NO_PRIVATE ;

	priv = (ZOH_DATA*) psrc->private_data ;

	if (priv->reset)
	{	/* If we have just been reset, set the last_value data. */
		for (ch = 0 ; ch < priv->channels ; ch++)
			priv->last_value [ch] = data->data_in [ch] ;
		priv->reset = 0 ;
		} ;

	priv->in_count = data->input_frames * priv->channels ;
	priv->out_count = data->output_frames * priv->channels ;
	priv->in_used = priv->out_gen = 0 ;

	src_ratio = psrc->last_ratio ;
	input_index = psrc->last_position ;

	/* Calculate samples before first sample in input array. */
	while (input_index < 1.0 && priv->out_gen < priv->out_count)
	{
		if (priv->in_used + priv->channels * input_index >= priv->in_count)
			break ;

		if (priv->out_count > 0 && fabs (psrc->last_ratio - data->src_ratio) > SRC_MIN_RATIO_DIFF)
			src_ratio = psrc->last_ratio + priv->out_gen * (data->src_ratio - psrc->last_ratio) / priv->out_count ;

		for (ch = 0 ; ch < priv->channels ; ch++)
		{	data->data_out [priv->out_gen] = priv->last_value [ch] ;
			priv->out_gen ++ ;
			} ;

		/* Figure out the next index. */
		input_index += 1.0 / src_ratio ;
		} ;

	rem = fmod_one (input_index) ;
	priv->in_used += priv->channels * lrint (input_index - rem) ;
	input_index = rem ;

	/* Main processing loop. */
	while (priv->out_gen < priv->out_count && priv->in_used + priv->channels * input_index <= priv->in_count)
	{
		if (priv->out_count > 0 && fabs (psrc->last_ratio - data->src_ratio) > SRC_MIN_RATIO_DIFF)
			src_ratio = psrc->last_ratio + priv->out_gen * (data->src_ratio - psrc->last_ratio) / priv->out_count ;

		for (ch = 0 ; ch < priv->channels ; ch++)
		{	data->data_out [priv->out_gen] = data->data_in [priv->in_used - priv->channels + ch] ;
			priv->out_gen ++ ;
			} ;

		/* Figure out the next index. */
		input_index += 1.0 / src_ratio ;
		rem = fmod_one (input_index) ;

		priv->in_used += priv->channels * lrint (input_index - rem) ;
		input_index = rem ;
		} ;

	if (priv->in_used > priv->in_count)
	{	input_index += (priv->in_used - priv->in_count) / priv->channels ;
		priv->in_used = priv->in_count ;
		} ;

	psrc->last_position = input_index ;

	if (priv->in_used > 0)
		for (ch = 0 ; ch < priv->channels ; ch++)
			priv->last_value [ch] = data->data_in [priv->in_used - priv->channels + ch] ;

	/* Save current ratio rather then target ratio. */
	psrc->last_ratio = src_ratio ;

	data->input_frames_used = priv->in_used / priv->channels ;
	data->output_frames_gen = priv->out_gen / priv->channels ;

	return SRC_ERR_NO_ERROR ;
} /* zoh_vari_process */

/*------------------------------------------------------------------------------
*/

const char*
zoh_get_name (int src_enum)
{
	if (src_enum == SRC_ZERO_ORDER_HOLD)
		return "ZOH Interpolator" ;

	return NULL ;
} /* zoh_get_name */

const char*
zoh_get_description (int src_enum)
{
	if (src_enum == SRC_ZERO_ORDER_HOLD)
		return "Zero order hold interpolator, very fast, poor quality." ;

	return NULL ;
} /* zoh_get_descrition */

int
zoh_set_converter (SRC_PRIVATE *psrc, int src_enum)
{	ZOH_DATA *priv = NULL ;

	if (src_enum != SRC_ZERO_ORDER_HOLD)
		return SRC_ERR_BAD_CONVERTER ;

	if (psrc->private_data != NULL)
	{	free (psrc->private_data) ;
		psrc->private_data = NULL ;
		} ;

	if (psrc->private_data == NULL)
	{	priv = calloc (1, sizeof (*priv) + psrc->channels * sizeof (float)) ;
		if (priv == NULL)
			return SRC_ERR_MALLOC_FAILED ;
		psrc->private_data = priv ;
		} ;

	priv->zoh_magic_marker = ZOH_MAGIC_MARKER ;
	priv->channels = psrc->channels ;

	psrc->const_process = zoh_vari_process ;
	psrc->vari_process = zoh_vari_process ;
	psrc->reset = zoh_reset ;

	zoh_reset (psrc) ;

	return SRC_ERR_NO_ERROR ;
} /* zoh_set_converter */

/*===================================================================================
*/

static void
zoh_reset (SRC_PRIVATE *psrc)
{	ZOH_DATA *priv ;

	priv = (ZOH_DATA*) psrc->private_data ;
	if (priv == NULL)
		return ;

	priv->channels = psrc->channels ;
	priv->reset = 1 ;
	memset (priv->last_value, 0, sizeof (priv->last_value [0]) * priv->channels) ;

	return ;
} /* zoh_reset */

