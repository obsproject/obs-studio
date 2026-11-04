#pragma once

#include <stdint.h>

struct preview_present_counters {
	uint64_t produced;
	uint64_t skipped;
	uint64_t submitted;
	uint64_t no_candidate;
	uint64_t expired;
	uint64_t dropped;
	uint64_t gpu_pending;
	uint64_t busy;
	uint64_t age_total;
	uint64_t age_max;
	uint64_t present_total;
	uint64_t present_max;
	uint64_t gap_max;
	uint64_t lock_total;
	uint64_t lock_max;
	uint64_t lock_samples;
};

static inline void preview_stats_accumulate(struct preview_present_counters *total,
					    const struct preview_present_counters *interval)
{
	total->produced += interval->produced;
	total->skipped += interval->skipped;
	total->submitted += interval->submitted;
	total->no_candidate += interval->no_candidate;
	total->expired += interval->expired;
	total->dropped += interval->dropped;
	total->gpu_pending += interval->gpu_pending;
	total->busy += interval->busy;
	total->age_total += interval->age_total;
	total->present_total += interval->present_total;
	total->lock_total += interval->lock_total;
	total->lock_samples += interval->lock_samples;
	if (interval->age_max > total->age_max)
		total->age_max = interval->age_max;
	if (interval->present_max > total->present_max)
		total->present_max = interval->present_max;
	if (interval->gap_max > total->gap_max)
		total->gap_max = interval->gap_max;
	if (interval->lock_max > total->lock_max)
		total->lock_max = interval->lock_max;
}
