#include "d3d11-present.hpp"

#include <algorithm>
#include <cinttypes>
#include <new>

#include <util/platform.h>
#include <util/preview-frame-selection.h>
#include <util/util_uint64.h>

void gs_swap_chain::InitWaitable()
{
	if (!(swapDesc.Flags & DXGI_SWAP_CHAIN_FLAG_FRAME_LATENCY_WAITABLE_OBJECT)) {
		return;
	}
	ComQIPtr<IDXGISwapChain2> swap2 = swap;
	if (buffered_present) {
		const HRESULT result = swap2->SetMaximumFrameLatency(1);
		if (FAILED(result)) {
			throw HRError("Failed to set preview frame latency", result);
		}
	}
	hWaitable = swap2->GetFrameLatencyWaitableObject();
	if (!hWaitable) {
		throw HRError("Failed to GetFrameLatencyWaitableObject", HRESULT_FROM_WIN32(GetLastError()));
	}
}

void gs_swap_chain::ResetBufferedPresent()
{
	if (buffered_present) {
		buffered_present->Reset();
	}
}

void gs_buffered_present::Reset()
{
	for (auto &frame : frames) {
		frame.ready_query.Clear();
		frame.created_time = 0;
		frame.sequence = 0;
		frame.pending = false;
	}
	writing = SIZE_MAX;
	displayed = SIZE_MAX;
	last_sequence = 0;
	stats_sync_time = 0;
	stats_sync_count = 0;
	stats_present_count = 0;
	stats_present_refresh_count = 0;
	last_present_time = 0;
	monitor = nullptr;
	statistics_valid = false;
	refresh_measured = false;
	allocation_failed = false;
	ResetStats();
}

void gs_buffered_present::ResetStats()
{
	stats = {};
	totals = {};
	stats_start = os_gettime_ns();
	stats_generation = stats_start;
	last_present_time = 0;
}

void gs_buffered_present::UpdateTiming(gs_swap_chain *swapchain)
{
	const HMONITOR current_monitor = MonitorFromWindow(swapchain->hwnd, MONITOR_DEFAULTTONEAREST);
	if (current_monitor != monitor) {
		monitor = current_monitor;
		refresh_measured = false;
		stats_sync_time = 0;
		stats_sync_count = 0;
		MONITORINFOEXW monitor_info = {};
		monitor_info.cbSize = sizeof(monitor_info);
		DEVMODEW mode = {};
		mode.dmSize = sizeof(mode);
		if (GetMonitorInfoW(monitor, reinterpret_cast<MONITORINFO *>(&monitor_info)) &&
		    EnumDisplaySettingsW(monitor_info.szDevice, ENUM_CURRENT_SETTINGS, &mode) &&
		    mode.dmDisplayFrequency > 1) {
			refresh_interval = 1000000000ULL / mode.dmDisplayFrequency;
		}
	}

	DXGI_FRAME_STATISTICS current = {};
	const HRESULT result = swapchain->swap->GetFrameStatistics(&current);
	statistics_valid = SUCCEEDED(result) && current.SyncQPCTime.QuadPart > 0;
	if (!statistics_valid) {
		if (result == DXGI_ERROR_FRAME_STATISTICS_DISJOINT) {
			refresh_measured = false;
			stats_sync_time = 0;
			stats_sync_count = 0;
		}
		return;
	}

	LARGE_INTEGER frequency;
	QueryPerformanceFrequency(&frequency);
	const uint64_t current_sync = util_mul_div64(current.SyncQPCTime.QuadPart, 1000000000ULL, frequency.QuadPart);
	const uint32_t refreshes = current.SyncRefreshCount - stats_sync_count;
	if (stats_sync_time && current_sync > stats_sync_time && refreshes && refreshes < 10000) {
		const uint64_t measured = (current_sync - stats_sync_time) / refreshes;
		if (measured >= 1000000ULL && measured <= 100000000ULL) {
			refresh_interval = measured;
			refresh_measured = true;
		}
	}
	stats_sync_time = current_sync;
	stats_sync_count = current.SyncRefreshCount;
	stats_present_count = current.PresentCount;
	stats_present_refresh_count = current.PresentRefreshCount;
}

void gs_buffered_present::Log(gs_swap_chain *swapchain, uint64_t now)
{
	if (!stats_start) {
		stats_start = now;
	}
	const uint64_t elapsed = now - stats_start;
	if (elapsed < 1000000000ULL) {
		return;
	}
	const double seconds = double(elapsed) / 1000000000.0;
	const double age_average = stats.submitted ? double(stats.age_total) / stats.submitted / 1000000.0 : 0.0;
	const double present_average = stats.submitted ? double(stats.present_total) / stats.submitted / 1000000.0
						       : 0.0;
	const double lock_average = stats.lock_samples ? double(stats.lock_total) / stats.lock_samples / 1000000.0
						       : 0.0;
	size_t queued = 0;
	for (const auto &frame : frames) {
		queued += frame.pending;
	}
	blog(LOG_DEBUG,
	     "[obs-preview-sync] swap=%p, interval=%.3fs, refresh_hz=%.3f, statistics_valid=%d, "
	     "produced=%" PRIu64 " (%.2f/s), submitted=%" PRIu64 " (%.2f/s), no_candidate_checks=%" PRIu64
	     ", dropped=%" PRIu64 ", expired=%" PRIu64 ", producer_skipped=%" PRIu64 ", gpu_pending_checks=%" PRIu64
	     ", busy=%" PRIu64 ", queued=%zu, submit_age_ms(avg/max)=%.3f/%.3f, "
	     "present_ms(avg/max)=%.3f/%.3f, submit_gap_ms(max)=%.3f, lock_ms(avg/max)=%.3f/%.3f, "
	     "dxgi_present_count=%u, dxgi_present_refresh_count=%u, dxgi_sync_refresh_count=%u",
	     static_cast<void *>(swapchain), seconds, 1000000000.0 / refresh_interval, statistics_valid, stats.produced,
	     stats.produced / seconds, stats.submitted, stats.submitted / seconds, stats.no_candidate, stats.dropped,
	     stats.expired, stats.skipped, stats.gpu_pending, stats.busy, queued, age_average,
	     double(stats.age_max) / 1000000.0, present_average, double(stats.present_max) / 1000000.0,
	     double(stats.gap_max) / 1000000.0, lock_average, double(stats.lock_max) / 1000000.0, stats_present_count,
	     stats_present_refresh_count, stats_sync_count);
	preview_stats_accumulate(&totals, &stats);
	stats = {};
	stats_start = now;
}

bool device_swapchain_get_present_stats(gs_device_t *device, gs_swapchain_t *swapchain,
					gs_swapchain_present_stats *stats, bool reset)
{
	UNUSED_PARAMETER(device);
	if (!swapchain->buffered_present) {
		return false;
	}
	auto &queue = *swapchain->buffered_present;
	if (reset || !queue.stats_generation) {
		queue.ResetStats();
	}
	*stats = {};
	stats->counters = queue.totals;
	preview_stats_accumulate(&stats->counters, &queue.stats);
	stats->generation = queue.stats_generation;
	stats->sample_time = os_gettime_ns();
	stats->refresh_interval = queue.refresh_interval;
	stats->timing_valid = queue.statistics_valid && queue.refresh_measured;
	for (const auto &frame : queue.frames) {
		stats->queued += frame.pending;
	}
	return true;
}

bool device_swapchain_set_buffered_present(gs_device_t *device, gs_swapchain_t *swapchain, bool enabled)
{
	if (!enabled) {
		device_set_render_target_with_color_space(device, nullptr, nullptr, swapchain->space);
		swapchain->buffered_present.reset();
		return true;
	}
	if (!swapchain->hWaitable) {
		return false;
	}
	ComQIPtr<IDXGISwapChain2> swap2 = swapchain->swap;
	if (!swap2 || FAILED(swap2->SetMaximumFrameLatency(1))) {
		return false;
	}
	if (!swapchain->buffered_present) {
		auto queue = std::unique_ptr<gs_buffered_present>(new (std::nothrow) gs_buffered_present);
		if (!queue) {
			return false;
		}
		queue->ResetStats();
		swapchain->buffered_present = std::move(queue);
	}
	return true;
}

void *device_swapchain_get_present_wait_handle(gs_device_t *device, gs_swapchain_t *swapchain)
{
	UNUSED_PARAMETER(device);
	HANDLE duplicate = nullptr;
	if (!swapchain->hWaitable || !DuplicateHandle(GetCurrentProcess(), swapchain->hWaitable, GetCurrentProcess(),
						      &duplicate, 0, FALSE, DUPLICATE_SAME_ACCESS)) {
		return nullptr;
	}
	return duplicate;
}

bool device_swapchain_begin_buffered_frame(gs_device_t *device, gs_swapchain_t *swapchain)
{
	auto &queue = *swapchain->buffered_present;
	const uint64_t now = os_gettime_ns();
	queue.Log(swapchain, now);
	if (queue.allocation_failed || !swapchain->target.width || !swapchain->target.height) {
		return false;
	}
	size_t selected = SIZE_MAX;
	for (size_t index = 0; index < queue.frames.size(); index++) {
		auto &frame = queue.frames[index];
		if (index == queue.displayed) {
			continue;
		}
		if (frame.pending && preview_frame_expired(frame.created_time, now, queue.refresh_interval)) {
			frame.pending = false;
			queue.stats.expired++;
			queue.stats.dropped++;
		}
		if (!frame.pending) {
			selected = index;
			break;
		}
		if (device->context->GetData(frame.ready_query, nullptr, 0, D3D11_ASYNC_GETDATA_DONOTFLUSH) == S_OK &&
		    (selected == SIZE_MAX || frame.sequence < queue.frames[selected].sequence)) {
			selected = index;
		}
	}
	if (selected == SIZE_MAX) {
		queue.stats.skipped++;
		return false;
	}
	auto &frame = queue.frames[selected];
	if (frame.pending) {
		queue.stats.dropped++;
	}
	frame.pending = false;
	const gs_color_format format = ConvertDXGITextureFormat(swapchain->target.dxgiFormatResource);
	try {
		if (!frame.texture || frame.texture->width != swapchain->target.width ||
		    frame.texture->height != swapchain->target.height || frame.texture->format != format) {
			frame.texture = std::make_unique<gs_texture_2d>(device, swapchain->target.width,
									swapchain->target.height, format, 1, nullptr,
									GS_RENDER_TARGET, GS_TEXTURE_2D, false);
		}
		if (!frame.ready_query) {
			const D3D11_QUERY_DESC description = {D3D11_QUERY_EVENT, 0};
			const HRESULT result = device->device->CreateQuery(&description, frame.ready_query.Assign());
			if (FAILED(result)) {
				throw HRError("Failed to create preview readiness query", result);
			}
		}
	} catch (const HRError &error) {
		blog(LOG_ERROR, "[obs-preview-sync] Cannot allocate preview buffer: %s (%08lX)", error.str, error.hr);
		queue.allocation_failed = true;
		return false;
	} catch (const std::bad_alloc &) {
		blog(LOG_ERROR, "[obs-preview-sync] Cannot allocate preview buffer: out of memory");
		queue.allocation_failed = true;
		return false;
	}
	queue.writing = selected;
	frame.created_time = now;
	device_set_render_target_with_color_space(device, frame.texture.get(), &swapchain->zs, swapchain->space);
	return true;
}

void device_swapchain_end_buffered_frame(gs_device_t *device, gs_swapchain_t *swapchain)
{
	auto &queue = *swapchain->buffered_present;
	if (queue.writing == SIZE_MAX) {
		return;
	}
	auto &frame = queue.frames[queue.writing];
	device->context->End(frame.ready_query);
	frame.sequence = queue.next_sequence++;
	frame.pending = true;
	queue.writing = SIZE_MAX;
	queue.stats.produced++;
	device_set_render_target_with_color_space(device, nullptr, nullptr, swapchain->space);
	device->context->Flush();
}

int device_swapchain_present_buffered_frame(gs_device_t *device, gs_swapchain_t *swapchain, uint64_t lock_wait_ns)
{
	auto &queue = *swapchain->buffered_present;
	const uint64_t now = os_gettime_ns();
	queue.stats.lock_samples++;
	queue.stats.lock_total += lock_wait_ns;
	queue.stats.lock_max = std::max(queue.stats.lock_max, lock_wait_ns);
	queue.UpdateTiming(swapchain);
	queue.Log(swapchain, now);
	std::array<preview_frame_candidate, 8> candidates = {};
	for (size_t index = 0; index < queue.frames.size(); index++) {
		auto &frame = queue.frames[index];
		if (frame.pending && preview_frame_expired(frame.created_time, now, queue.refresh_interval)) {
			frame.pending = false;
			queue.stats.expired++;
			queue.stats.dropped++;
		}
		if (!frame.pending || frame.sequence <= queue.last_sequence) {
			continue;
		}
		const HRESULT ready_result =
			device->context->GetData(frame.ready_query, nullptr, 0, D3D11_ASYNC_GETDATA_DONOTFLUSH);
		if (FAILED(ready_result) && FAILED(device->device->GetDeviceRemovedReason())) {
			device->RebuildDevice();
			return -1;
		}
		const bool ready = ready_result == S_OK;
		candidates[index] = {frame.created_time, frame.sequence, ready};
		if (!ready) {
			queue.stats.gpu_pending++;
		}
	}
	const uint64_t selection_time = os_gettime_ns();
	const size_t selected = preview_frame_select(candidates.data(), candidates.size(), selection_time,
						     queue.refresh_interval, queue.last_sequence);
	if (selected == SIZE_MAX) {
		queue.stats.no_candidate++;
		return 0;
	}
	auto &frame = queue.frames[selected];
	if (!frame.texture || frame.texture->width != swapchain->target.width ||
	    frame.texture->height != swapchain->target.height ||
	    frame.texture->dxgiFormatView != swapchain->target.dxgiFormatView) {
		return 0;
	}
	device_load_swapchain(device, swapchain);
	device->context->OMSetRenderTargets(0, nullptr, nullptr);
	device->curFramebufferInvalidate = true;
	device->context->CopyResource(swapchain->target.texture, frame.texture->texture);
	const uint64_t present_start = os_gettime_ns();
	const HRESULT result = swapchain->swap->Present(1, DXGI_PRESENT_DO_NOT_WAIT);
	const uint64_t present_end = os_gettime_ns();
	if (result == DXGI_ERROR_WAS_STILL_DRAWING) {
		queue.stats.busy++;
		return 0;
	}
	if (result == DXGI_ERROR_DEVICE_REMOVED || result == DXGI_ERROR_DEVICE_RESET) {
		device->RebuildDevice();
		return -1;
	}
	if (FAILED(result)) {
		blog(LOG_WARNING, "[obs-preview-sync] Present failed (%08lX)", result);
		return -1;
	}
	queue.stats.submitted++;
	const uint64_t age = present_end - frame.created_time;
	queue.stats.age_total += age;
	queue.stats.age_max = std::max(queue.stats.age_max, age);
	const uint64_t present_duration = present_end - present_start;
	queue.stats.present_total += present_duration;
	queue.stats.present_max = std::max(queue.stats.present_max, present_duration);
	if (queue.last_present_time) {
		queue.stats.gap_max = std::max(queue.stats.gap_max, present_end - queue.last_present_time);
	}
	queue.last_present_time = present_end;
	queue.displayed = selected;
	queue.last_sequence = frame.sequence;
	for (auto &queued_frame : queue.frames) {
		if (queued_frame.pending && queued_frame.sequence <= frame.sequence) {
			queue.stats.dropped += queued_frame.sequence != frame.sequence;
			queued_frame.pending = false;
		}
	}
	return result == DXGI_STATUS_OCCLUDED ? 2 : 1;
}
