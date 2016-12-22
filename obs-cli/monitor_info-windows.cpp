#include "monitor_info.hpp"

#include <iostream>
#include <Windows.h>

namespace {
	std::vector<MonitorInfo> all_monitors;
};

const MonitorInfo MonitorInfo::NotFound{ 0, 0, 0, 0, 0 };

// callback function called by EnumDisplayMonitors for each enabled monitor
BOOL CALLBACK MonitorEnumProc(HMONITOR hMonitor, // handle to display monitor
	HDC hdcMonitor, // handle to monitor DC
	LPRECT, // lprcMonitor monitor intersection rectangle
	LPARAM dwData // data
	)
{
	MONITORINFO mi;
	mi.cbSize = sizeof(mi);
	GetMonitorInfo(hMonitor, &mi);

	MonitorInfo info;
	info.monitor_id = all_monitors.size();

	info.width = mi.rcMonitor.right - mi.rcMonitor.left;
	info.height = mi.rcMonitor.bottom - mi.rcMonitor.top;
	info.left = mi.rcMonitor.left;
	info.top = mi.rcMonitor.top;

	all_monitors.push_back(info);

	UNREFERENCED_PARAMETER(dwData);
	UNREFERENCED_PARAMETER(hdcMonitor);

	UNREFERENCED_PARAMETER(dwData);
	return TRUE;
}

MonitorInfo monitor_at_index(int m) {
	if (m >= 0 && m < all_monitors.size())
		return all_monitors[m];

	return MonitorInfo::NotFound;
}

bool detect_monitors() {
	all_monitors.clear();
	return !!EnumDisplayMonitors(NULL, NULL, MonitorEnumProc, 0);
}

void print_monitors_info() {
	for (auto monitor : all_monitors) {
		std::cout << "Monitor: " << monitor.monitor_id << " "
			<< "" << monitor.width << "x"
			<< "" << monitor.height << " "
			<< std::endl;
	}
}

