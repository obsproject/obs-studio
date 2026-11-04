#include <util/preview-stats.h>

#include <stdio.h>
#include <stdlib.h>

static void check_counter(uint64_t actual, uint64_t expected)
{
	if (actual != expected) {
		fprintf(stderr, "Unexpected preview statistic\n");
		abort();
	}
}

static void test_interval_accumulation(void)
{
	struct preview_present_counters totals = {
		.produced = 60,
		.skipped = 1,
		.submitted = 59,
		.no_candidate = 10,
		.expired = 2,
		.dropped = 3,
		.gpu_pending = 4,
		.busy = 5,
		.age_total = 590000000,
		.age_max = 20000000,
		.present_total = 590000,
		.present_max = 30000,
		.gap_max = 20000000,
		.lock_total = 600000,
		.lock_max = 40000,
		.lock_samples = 60,
	};
	const struct preview_present_counters interval = {
		.produced = 60,
		.skipped = 2,
		.submitted = 58,
		.no_candidate = 20,
		.expired = 3,
		.dropped = 4,
		.gpu_pending = 5,
		.busy = 6,
		.age_total = 580000000,
		.age_max = 19000000,
		.present_total = 580000,
		.present_max = 50000,
		.gap_max = 30000000,
		.lock_total = 700000,
		.lock_max = 30000,
		.lock_samples = 61,
	};
	preview_stats_accumulate(&totals, &interval);
	check_counter(totals.produced, 120);
	check_counter(totals.skipped, 3);
	check_counter(totals.submitted, 117);
	check_counter(totals.no_candidate, 30);
	check_counter(totals.expired, 5);
	check_counter(totals.dropped, 7);
	check_counter(totals.gpu_pending, 9);
	check_counter(totals.busy, 11);
	check_counter(totals.age_total, 1170000000);
	check_counter(totals.age_max, 20000000);
	check_counter(totals.present_total, 1170000);
	check_counter(totals.present_max, 50000);
	check_counter(totals.gap_max, 30000000);
	check_counter(totals.lock_total, 1300000);
	check_counter(totals.lock_max, 40000);
	check_counter(totals.lock_samples, 121);
	check_counter(interval.submitted, 58);

	const struct preview_present_counters empty = {0};
	preview_stats_accumulate(&totals, &empty);
	check_counter(totals.submitted, 117);
	check_counter(totals.present_max, 50000);
}

static void test_live_snapshot_and_reset(void)
{
	struct preview_present_counters totals = {.submitted = 60, .present_max = 100000};
	const struct preview_present_counters pending = {.submitted = 30, .present_max = 200000};
	for (size_t index = 0; index < 3; index++) {
		struct preview_present_counters snapshot = totals;
		preview_stats_accumulate(&snapshot, &pending);
		check_counter(snapshot.submitted, 90);
		check_counter(snapshot.present_max, 200000);
		check_counter(totals.submitted, 60);
	}
	preview_stats_accumulate(&totals, &pending);
	check_counter(totals.submitted, 90);
	check_counter(totals.present_max, 200000);
	totals = (struct preview_present_counters){0};
	check_counter(totals.submitted, 0);
	check_counter(totals.present_max, 0);
}

int main(void)
{
	test_interval_accumulation();
	test_live_snapshot_and_reset();
	puts("Preview statistics tests passed");
	return 0;
}
