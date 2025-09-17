/*
 *  Lag lead filter for asynchronous sample rate converter
 *  Copyright (C) 2022 Norihiro Kamae <norihiro@nagater.net>
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License along
 *  with this program; if not, write to the Free Software Foundation, Inc.,
 *  51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 */

#pragma once

struct lag_lead_filter {
	int64_t error_ns;
	double vc1;
	double vc2;

	double gain;
	double c1_inv;
	double c2_inv;
};

static void lag_lead_filter_reset(struct lag_lead_filter *f)
{
	f->vc1 = 0.0;
	f->vc2 = 0.0;
}

static void lag_lead_filter_set_parameters(struct lag_lead_filter *f, double gain, double c1, double c2)
{
	f->gain = gain;
	f->c1_inv = 1.0 / c1;
	f->c2_inv = 1.0 / c2;
}

static inline void lag_lead_filter_set_error_ns(struct lag_lead_filter *f, int32_t error_ns)
{
	f->error_ns = error_ns;
}

static void lag_lead_filter_tick(struct lag_lead_filter *f, uint32_t fs, uint32_t frames)
{
	const double icp = f->gain * f->error_ns * 1e-9;
	const double ic2 = (f->vc1 - f->vc2) / fs;
	const double ic1 = icp - ic2;

	f->vc1 += ic1 * f->c1_inv * frames;
	f->vc2 += ic2 * f->c2_inv * frames;
}

static inline double lag_lead_filter_get_drift(const struct lag_lead_filter *f)
{
	return f->vc1;
}
