#pragma once

#include "d3d11-subsystem.hpp"

#include <array>

struct buffered_present_frame {
	std::unique_ptr<gs_texture_2d> texture;
	ComPtr<ID3D11Query> ready_query;
	uint64_t created_time = 0;
	uint64_t sequence = 0;
	bool pending = false;
};

struct gs_buffered_present {
	std::array<buffered_present_frame, 8> frames;
	size_t writing = SIZE_MAX;
	size_t displayed = SIZE_MAX;
	uint64_t next_sequence = 1;
	uint64_t last_sequence = 0;
	uint64_t refresh_interval = 16666667;
	uint64_t stats_sync_time = 0;
	uint32_t stats_sync_count = 0;
	uint32_t stats_present_count = 0;
	uint32_t stats_present_refresh_count = 0;
	uint64_t last_present_time = 0;
	HMONITOR monitor = nullptr;
	bool statistics_valid = false;
	bool refresh_measured = false;
	bool allocation_failed = false;
	preview_present_counters stats = {};
	preview_present_counters totals = {};
	uint64_t stats_start = 0;
	uint64_t stats_generation = 0;

	void Reset();
	void ResetStats();
	void UpdateTiming(gs_swap_chain *swapchain);
	void Log(gs_swap_chain *swapchain, uint64_t now);
};
