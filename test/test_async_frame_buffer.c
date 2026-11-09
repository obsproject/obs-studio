#include <util/async-frame-buffer.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define CHECK(condition) do { if (!(condition)) { fprintf(stderr, "%s:%d: %s\n", __FILE__, __LINE__, #condition); abort(); } } while (0)

struct test_queue {
	struct obs_async_buffer_config config;
	struct async_frame_buffer buffer;
	uint64_t timestamps[OBS_ASYNC_BUFFER_MAX_FRAMES + 1];
	uint64_t sequences[OBS_ASYNC_BUFFER_MAX_FRAMES + 1];
	size_t count;
	size_t overflows;
	size_t dropped;
	uint64_t last_output;
};

static struct obs_async_buffer_config default_config(bool rebuild)
{
	struct obs_async_buffer_config config = {true, 2, 5, rebuild, 60, 1};
	return config;
}

static void erase_frames(struct test_queue *queue, size_t count)
{
	CHECK(count <= queue->count);
	queue->count -= count;
	memmove(queue->timestamps, queue->timestamps + count, queue->count * sizeof(uint64_t));
	memmove(queue->sequences, queue->sequences + count, queue->count * sizeof(uint64_t));
}

static void enqueue_frame(struct test_queue *queue, uint64_t timestamp, uint64_t sequence)
{
	if (async_frame_buffer_arrived(&queue->buffer, timestamp, queue->config.rebuild_timestamps)) {
		erase_frames(queue, queue->count);
		queue->buffer.playing = false;
	}
	CHECK(queue->count <= OBS_ASYNC_BUFFER_MAX_FRAMES);
	queue->timestamps[queue->count] = timestamp;
	queue->sequences[queue->count++] = sequence;
	const size_t discard = async_frame_buffer_trim(&queue->config, queue->count);
	if (discard) {
		queue->overflows++;
		queue->dropped += discard;
		erase_frames(queue, discard);
		queue->buffer.playing = false;
	}
}

static bool select_frame(struct test_queue *queue, uint64_t timestamp, uint64_t interval)
{
	const struct async_frame_buffer_selection selection = async_frame_buffer_select(
		&queue->buffer, &queue->config, queue->timestamps, queue->count, timestamp, interval);
	queue->dropped += selection.discard;
	erase_frames(queue, selection.discard);
	if (!selection.output)
		return false;
	CHECK(queue->count > 0);
	CHECK(queue->sequences[0] > queue->last_output);
	queue->last_output = queue->sequences[0];
	erase_frames(queue, 1);
	return true;
}

static void test_prefill_underrun_and_restart(bool rebuild)
{
	struct test_queue queue = {0};
	queue.config = default_config(rebuild);
	const uint64_t now = 90000000000ULL;
	const uint64_t interval = 16666667;
	enqueue_frame(&queue, 5000000000ULL, 1);
	CHECK(!select_frame(&queue, now, interval));
	CHECK(!queue.buffer.playing && queue.count == 1);
	enqueue_frame(&queue, 5016666666ULL, 2);
	CHECK(select_frame(&queue, now, interval));
	CHECK(queue.last_output == 1 && queue.count == 1);
	CHECK(queue.buffer.output_anchor == now);
	CHECK(!select_frame(&queue, now + 5000000, interval));
	CHECK(select_frame(&queue, now + interval, interval));
	CHECK(queue.last_output == 2 && queue.count == 0 && !queue.buffer.playing);
	CHECK(!select_frame(&queue, now + 2 * interval, interval));
	enqueue_frame(&queue, 5033333333ULL, 3);
	CHECK(!select_frame(&queue, now + 3 * interval, interval));
	enqueue_frame(&queue, 5050000000ULL, 4);
	CHECK(select_frame(&queue, now + 4 * interval, interval));
	CHECK(queue.last_output == 3 && queue.buffer.output_anchor == now + 4 * interval);
}

static void test_overflow_and_limits(void)
{
	struct test_queue queue = {0};
	queue.config = default_config(true);
	for (uint64_t sequence = 1; sequence <= 5; sequence++)
		enqueue_frame(&queue, sequence * 16666667, sequence);
	CHECK(queue.count == 5 && queue.overflows == 0);
	enqueue_frame(&queue, 100000000, 6);
	CHECK(queue.count == 2 && queue.overflows == 1 && queue.dropped == 4);
	CHECK(queue.sequences[0] == 5 && !queue.buffer.playing);
	CHECK(select_frame(&queue, 1000000000, 16666667));
	CHECK(queue.last_output == 5 && queue.buffer.output_anchor == 1000000000);

	memset(&queue, 0, sizeof(queue));
	queue.config = default_config(true);
	queue.config.target_frames = queue.config.max_frames = OBS_ASYNC_BUFFER_MAX_FRAMES;
	for (uint64_t sequence = 1; sequence <= OBS_ASYNC_BUFFER_MAX_FRAMES + 1; sequence++)
		enqueue_frame(&queue, sequence, sequence);
	CHECK(queue.count == OBS_ASYNC_BUFFER_MAX_FRAMES && queue.sequences[0] == 2);
	CHECK(select_frame(&queue, 1000000000, 16666667));
	CHECK(queue.last_output == 2);

	memset(&queue, 0, sizeof(queue));
	queue.config = default_config(true);
	queue.config.target_frames = queue.config.max_frames = 1;
	enqueue_frame(&queue, 0, 1);
	CHECK(select_frame(&queue, 0, 16666667));
	CHECK(!queue.buffer.playing);
	enqueue_frame(&queue, 1, 2);
	enqueue_frame(&queue, 2, 3);
	CHECK(queue.count == 1 && queue.overflows == 1);
	CHECK(select_frame(&queue, 1000000000, 16666667));
	CHECK(queue.last_output == 3);
}

static void test_age_boundaries_and_oldest_selection(bool rebuild)
{
	const uint64_t now = 90000000000ULL;
	const uint64_t interval = 20000000;
	const uint64_t timestamps[] = {1000000000, 1020000000, 1040000000};
	for (uint64_t extra = 0; extra < 2; extra++) {
		struct async_frame_buffer buffer = {0};
		struct obs_async_buffer_config config = default_config(rebuild);
		config.fps_numerator = 50;
		struct async_frame_buffer_selection selection =
			async_frame_buffer_select(&buffer, &config, timestamps, 3, now, interval);
		CHECK(selection.output && selection.discard == 0);
		selection = async_frame_buffer_select(&buffer, &config, timestamps + 1, 2, now + 32000000 + extra,
						      interval);
		CHECK(selection.output && selection.discard == extra);
	}
	struct async_frame_buffer buffer = {0};
	struct obs_async_buffer_config config = default_config(rebuild);
	config.fps_numerator = 50;
	CHECK(async_frame_buffer_select(&buffer, &config, timestamps, 3, now, interval).output);
	struct async_frame_buffer_selection selection =
		async_frame_buffer_select(&buffer, &config, timestamps + 1, 2, now + 7999999, interval);
	CHECK(!selection.output && selection.discard == 0 && buffer.playing);
	selection = async_frame_buffer_select(&buffer, &config, timestamps + 1, 2, now + 8000000, interval);
	CHECK(selection.output && selection.discard == 0);
	selection = async_frame_buffer_select(&buffer, &config, timestamps + 2, 1, now + 40000000, interval);
	CHECK(selection.output && selection.discard == 0 && !buffer.playing);
	async_frame_buffer_reset(&buffer);
	CHECK(async_frame_buffer_select(&buffer, &config, timestamps, 3, now, interval).output);
	selection = async_frame_buffer_select(&buffer, &config, timestamps + 1, 2, now + 60000000, interval);
	CHECK(!selection.output && selection.discard == 2 && !buffer.playing);
	CHECK(!async_frame_buffer_select(&buffer, &config, timestamps + 2, 1, now + 70000000, interval).output);
}

static void test_window_near_unsigned_limits(void)
{
	const uint64_t anchors[] = {0, UINT64_MAX - 1000000000};
	const uint64_t timestamps[] = {1000000000, 1020000000};
	for (size_t index = 0; index < sizeof(anchors) / sizeof(anchors[0]); index++) {
		struct async_frame_buffer buffer = {0};
		struct obs_async_buffer_config config = default_config(true);
		config.fps_numerator = 50;
		CHECK(async_frame_buffer_select(&buffer, &config, timestamps, 2, anchors[index], 20000000).output);
		CHECK(!async_frame_buffer_select(&buffer, &config, timestamps + 1, 1, anchors[index] + 7999999,
						 20000000)
			       .output);
		CHECK(async_frame_buffer_select(&buffer, &config, timestamps + 1, 1, anchors[index] + 8000000, 20000000)
			      .output);
	}
	struct async_frame_buffer buffer = {0};
	struct obs_async_buffer_config config = default_config(true);
	config.target_frames = 1;
	config.fps_numerator = 50;
	CHECK(async_frame_buffer_select(&buffer, &config, timestamps, 2, 0, UINT64_MAX).output);
	const struct async_frame_buffer_selection selection =
		async_frame_buffer_select(&buffer, &config, timestamps + 1, 1, UINT64_MAX, UINT64_MAX);
	CHECK(!selection.output && selection.discard == 1 && !buffer.playing);
}

static void test_rebuilt_clock_ignores_device_spacing(void)
{
	struct async_frame_buffer buffer = {0};
	struct obs_async_buffer_config config = default_config(true);
	config.fps_numerator = 50;
	const uint64_t timestamps[] = {3000000000, 0, 123};
	const uint64_t original[] = {3000000000, 0, 123};
	CHECK(async_frame_buffer_select(&buffer, &config, timestamps, 3, 1000000000, 20000000).output);
	CHECK(buffer.next_timestamp == 1020000000);
	CHECK(!async_frame_buffer_select(&buffer, &config, timestamps + 1, 2, 1007999999, 20000000).output);
	CHECK(buffer.next_timestamp == 1020000000);
	CHECK(async_frame_buffer_select(&buffer, &config, timestamps + 1, 2, 1008000000, 20000000).output);
	CHECK(buffer.next_timestamp == 1040000000);
	CHECK(!memcmp(timestamps, original, sizeof(original)));
}

static void test_fractional_clock_no_accumulated_rounding(void)
{
	const uint32_t numerators[] = {60, 60000, 30000, 24000, 1000000, 1};
	const uint32_t denominators[] = {1, 1001, 1001, 1001, 1000000, 1000000};
	for (size_t rate = 0; rate < sizeof(numerators) / sizeof(numerators[0]); rate++) {
		struct async_frame_buffer buffer = {0};
		struct obs_async_buffer_config config = default_config(true);
		config.fps_numerator = numerators[rate];
		config.fps_denominator = denominators[rate];
		const uint64_t duration = 1000000000ULL * config.fps_denominator;
		const uint64_t safe_steps = UINT64_MAX / (duration / config.fps_numerator + 1);
		const uint64_t steps = safe_steps < 100000 ? safe_steps : 100000;
		for (uint64_t frame = 1; frame <= steps; frame++) {
			async_frame_buffer_advance(&buffer, &config);
			CHECK(buffer.next_timestamp ==
			      frame * (duration / config.fps_numerator) +
				      frame * (duration % config.fps_numerator) / config.fps_numerator);
		}
	}
}

static void test_native_discontinuity_and_reset(void)
{
	struct test_queue queue = {0};
	queue.config = default_config(false);
	enqueue_frame(&queue, 1000000000, 1);
	enqueue_frame(&queue, 1016666667, 2);
	CHECK(select_frame(&queue, 90000000000ULL, 16666667));
	enqueue_frame(&queue, 1000, 3);
	CHECK(queue.count == 1 && !queue.buffer.playing);
	CHECK(!select_frame(&queue, 90016666667ULL, 16666667));
	enqueue_frame(&queue, 16667667, 4);
	CHECK(select_frame(&queue, 90033333334ULL, 16666667));
	CHECK(queue.last_output == 3);
	enqueue_frame(&queue, 4000000000, 5);
	CHECK(queue.count == 1 && !queue.buffer.playing);
	async_frame_buffer_reset(&queue.buffer);
	CHECK(!queue.buffer.playing && !queue.buffer.arrival_valid && queue.buffer.remainder == 0);
	queue.config.rebuild_timestamps = true;
	enqueue_frame(&queue, 0, 6);
	CHECK(queue.count == 2);
	CHECK(select_frame(&queue, 100000000000ULL, 16666667));
}

static void test_invalid_configuration(void)
{
	struct obs_async_buffer_config config = {true, 0, 0, true, 0, 0};
	config = async_frame_buffer_normalize(config);
	CHECK(config.target_frames == 1 && config.max_frames == 1);
	CHECK(config.fps_numerator == 60 && config.fps_denominator == 1);
	config.target_frames = 100;
	config.max_frames = 1;
	config.fps_numerator = config.fps_denominator = UINT32_MAX;
	config = async_frame_buffer_normalize(config);
	CHECK(config.target_frames == OBS_ASYNC_BUFFER_MAX_FRAMES && config.max_frames == config.target_frames);
	CHECK(config.fps_numerator == 60 && config.fps_denominator == 1);
	config.target_frames = 2;
	config.max_frames = UINT32_MAX;
	CHECK(async_frame_buffer_normalize(config).max_frames == OBS_ASYNC_BUFFER_MAX_FRAMES);
}

static void test_cadence(uint32_t capture_num, uint32_t capture_den, uint64_t expected, bool rebuild)
{
	struct test_queue queue = {0};
	queue.config = default_config(rebuild);
	queue.config.fps_numerator = capture_num;
	queue.config.fps_denominator = capture_den;
	uint64_t received = 0;
	uint64_t selected = 0;
	for (uint64_t tick = 0; tick < 720; tick++) {
		const uint64_t now = tick * 1000000000ULL / 60;
		while (received * 1000000000ULL * capture_den / capture_num <= now) {
			enqueue_frame(&queue, 50000000000ULL + received * 1000000000ULL * capture_den / capture_num,
				      received + 1);
			received++;
		}
		const bool output = select_frame(&queue, 90000000000ULL + now, 16666666);
		if (tick >= 120 && output)
			selected++;
		CHECK(queue.count <= queue.config.max_frames);
	}
	if (selected != expected) {
		fprintf(stderr, "cadence %u/%u, rebuild=%d: selected %llu, expected %llu\n", capture_num, capture_den,
			rebuild, (unsigned long long)selected, (unsigned long long)expected);
		abort();
	}
}

static void test_arrival_jitter(bool rebuild, bool unstable_device_timestamps)
{
	struct test_queue queue = {0};
	queue.config = default_config(rebuild);
	uint64_t received = 0;
	uint64_t selected = 0;
	for (uint64_t tick = 0; tick < 720; tick++) {
		const int64_t now = (int64_t)(tick * 1000000000ULL / 60);
		for (;;) {
			const int64_t generated = (int64_t)(received * 1000000000ULL / 60);
			const int64_t jitter = received % 3 == 0 ? -2000000 : 2000000;
			if (generated + jitter > now)
				break;
			const int64_t timestamp_error =
				unstable_device_timestamps ? ((int64_t)(received % 3) - 1) * 20000000 : 0;
			enqueue_frame(&queue, (uint64_t)(50000000000LL + generated + timestamp_error), received + 1);
			received++;
		}
		const bool output = select_frame(&queue, 90000000000ULL + now, 16666666);
		if (tick >= 120 && output)
			selected++;
	}
	CHECK(selected == 600);
	CHECK(queue.overflows == 0);
}

int main(void)
{
	test_prefill_underrun_and_restart(false);
	test_prefill_underrun_and_restart(true);
	test_overflow_and_limits();
	test_age_boundaries_and_oldest_selection(false);
	test_age_boundaries_and_oldest_selection(true);
	test_window_near_unsigned_limits();
	test_rebuilt_clock_ignores_device_spacing();
	test_fractional_clock_no_accumulated_rounding();
	test_native_discontinuity_and_reset();
	test_invalid_configuration();
	test_arrival_jitter(false, false);
	test_arrival_jitter(true, false);
	test_arrival_jitter(true, true);
	test_cadence(60, 1, 600, false);
	test_cadence(60, 1, 600, true);
	test_cadence(120, 1, 600, false);
	test_cadence(120, 1, 600, true);
	test_cadence(30, 1, 300, false);
	test_cadence(30, 1, 300, true);
	test_cadence(60000, 1001, 599, false);
	test_cadence(60000, 1001, 599, true);
	test_cadence(30000, 1001, 299, false);
	test_cadence(30000, 1001, 299, true);
	puts("async frame buffer tests passed");
	return 0;
}
