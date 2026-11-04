#include <util/preview-frame-selection.h>

#include <stdio.h>
#include <stdlib.h>

static void check_selection(size_t actual, size_t expected)
{
	if (actual != expected) {
		fprintf(stderr, "Expected %zu, got %zu\n", expected, actual);
		abort();
	}
}

static void test_oldest_in_window(void)
{
	const struct preview_frame_candidate frames[] = {
		{96000000, 3, true},
		{78000000, 1, true},
		{80000000, 2, true},
	};
	check_selection(preview_frame_select(frames, 3, 100000000, 16666667, 0), 2);
	check_selection(preview_frame_select(frames, 3, 100000000, 16666667, 2), 0);
	check_selection(preview_frame_select(frames, 3, 100000000, 16666667, 3), SIZE_MAX);
}

static void test_window_boundaries_and_readiness(void)
{
	const uint64_t interval = 16666667;
	const uint64_t now = 100000000;
	const uint64_t maximum_age = interval + interval / 4;
	const struct preview_frame_candidate frames[] = {
		{now - maximum_age - 1, 1, true},
		{now - maximum_age, 2, true},
		{now - 10000000, 3, false},
		{now, 4, true},
		{now + 1, 5, true},
	};
	check_selection(preview_frame_select(frames, 5, now, interval, 0), 1);
	check_selection(preview_frame_select(frames, 5, now, interval, 2), 3);
	check_selection(preview_frame_select(frames, 5, now, interval, 4), SIZE_MAX);
	check_selection(preview_frame_select(NULL, 0, now, interval, 0), SIZE_MAX);
	check_selection(preview_frame_expired(now - interval - interval / 2, now, interval), false);
	check_selection(preview_frame_expired(now - interval - interval / 2 - 1, now, interval), true);
	check_selection(preview_frame_expired(now + 1, now, interval), false);
}

static void test_consumed_frame_cannot_return(void)
{
	const struct preview_frame_candidate frames[] = {
		{1000000000, 1, true},
		{1016666667, 2, false},
	};
	const uint64_t interval = 16666667;
	check_selection(preview_frame_select(frames, 2, 1002000000, interval, 0), 0);
	check_selection(preview_frame_select(frames, 2, 1002000000, interval, 0), 0);
	check_selection(preview_frame_select(frames, 2, 1018666667, interval, 1), SIZE_MAX);
	struct preview_frame_candidate completed[] = {frames[0], frames[1]};
	completed[1].ready = true;
	check_selection(preview_frame_select(completed, 2, 1018666667, interval, 1), 1);
}

static void test_ring_order_and_equal_timestamps(void)
{
	const struct preview_frame_candidate frames[] = {
		{100, 4, true},
		{80, 2, true},
		{100, 5, true},
		{90, 3, true},
	};
	check_selection(preview_frame_select(frames, 4, 100, 20, 1), 1);
	check_selection(preview_frame_select(frames, 4, 100, 20, 3), 0);
	check_selection(preview_frame_select(frames, 4, 100, 20, 4), 2);
	check_selection(preview_frame_select(frames, 4, 100, 20, 5), SIZE_MAX);
}

struct simulation_result {
	size_t selected;
	size_t no_candidate;
	size_t skipped;
};

static struct simulation_result simulate(unsigned producer_fps, uint64_t phase, uint64_t jitter,
					 uint64_t completion_delay)
{
	const uint64_t interval = 16666667;
	const uint64_t base = 1000000000;
	uint64_t last_sequence = 0;
	struct simulation_result result = {0};
	for (size_t refresh = 0; refresh < 720; refresh++) {
		const uint64_t nominal = base + (refresh + 120) * interval + phase;
		const uint64_t now = refresh % 3 == 0 ? nominal - jitter : nominal + jitter;
		const uint64_t newest = ((now - base + 1) * producer_fps - 1) / 1000000000;
		struct preview_frame_candidate frames[8];
		for (size_t index = 0; index < 8; index++) {
			const uint64_t sequence = newest - 7 + index + 1;
			const uint64_t timestamp = base + (sequence - 1) * 1000000000 / producer_fps;
			frames[index] = (struct preview_frame_candidate){
				.timestamp = timestamp,
				.sequence = sequence,
				.ready = timestamp <= now && now - timestamp >= completion_delay,
			};
		}
		const size_t selected = preview_frame_select(frames, 8, now, interval, last_sequence);
		if (selected == SIZE_MAX) {
			result.no_candidate += refresh >= 120;
			continue;
		}
		const uint64_t sequence = frames[selected].sequence;
		if (sequence <= last_sequence) {
			fputs("Selected a previously consumed frame\n", stderr);
			abort();
		}
		if (refresh >= 120) {
			result.selected++;
			result.skipped += sequence - last_sequence - 1;
		}
		last_sequence = sequence;
	}
	return result;
}

static void test_small_jitter_phase_sweep(void)
{
	for (size_t phase = 0; phase < 64; phase++) {
		for (size_t jitter = 0; jitter < 2; jitter++) {
			const uint64_t offset = phase * 16666667ULL / 64;
			const struct simulation_result result = simulate(60, offset, jitter * 200000, 2000000);
			check_selection(result.selected, 600);
			check_selection(result.no_candidate, 0);
			check_selection(result.skipped, 0);
		}
	}
	puts("60/60: all 64 phases at +/-0.2 ms jitter, 2 ms GPU delay: 600/600 new frames");
}

static void test_different_producer_rates(void)
{
	const unsigned rates[] = {30, 120, 144};
	for (size_t index = 0; index < 3; index++) {
		for (size_t phase = 0; phase < 64; phase++) {
			const struct simulation_result result =
				simulate(rates[index], phase * 16666667ULL / 64, 200000, 2000000);
			const size_t expected = rates[index] == 30 ? 300 : 600;
			check_selection(result.selected, expected);
			check_selection(result.no_candidate, 600 - expected);
		}
	}
	puts("30/60: 300/600 new frames; 120/60 and 144/60: 600/600 new frames");
}

static void test_hard_age_limit_under_larger_jitter(void)
{
	const struct simulation_result result = simulate(60, 15 * 16666667ULL / 64, 2000000, 2000000);
	check_selection(result.selected, 400);
	check_selection(result.no_candidate, 200);
	check_selection(result.skipped, 200);
	puts("Known limit: 60/60, +/-2 ms jitter, 2 ms GPU delay: 400/600 new frames at one phase");
}

int main(void)
{
	test_oldest_in_window();
	test_window_boundaries_and_readiness();
	test_consumed_frame_cannot_return();
	test_ring_order_and_equal_timestamps();
	test_small_jitter_phase_sweep();
	test_different_producer_rates();
	test_hard_age_limit_under_larger_jitter();
	puts("Preview frame selection tests passed");
	return 0;
}
