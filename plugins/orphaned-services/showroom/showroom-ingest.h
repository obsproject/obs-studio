// SPDX-FileCopyrightText: 2020 Toasterapp <s.toaster.app@gmail.com>
//
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

struct showroom_ingest {
	const char *url;
	const char *key;
};

struct showroom_ingest *showroom_get_ingest(const char *server,
					    const char *access_key);

void free_showroom_data();
