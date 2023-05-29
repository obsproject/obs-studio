// SPDX-FileCopyrightText: 2023 tytan652 <tytan652@tytanium.xyz>
//
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include <vector>
#include <memory>

#include "service-instance.hpp"

class ServicesManager {
	std::vector<std::shared_ptr<ServiceInstance>> services;

public:
	ServicesManager(){};
	inline ~ServicesManager() { services.clear(); };

	bool RegisterServices();

	static bool Initialize();
	static void Finalize();
};
