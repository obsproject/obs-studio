// SPDX-FileCopyrightText: 2020 Faeez Kadiri <faeez.kadiri@mediamagictechnologies.com>
//
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

struct dacast_ingest {
	const char *url;
	const char *username;
	const char *password;
	const char *streamkey;
};

void init_dacast_data(void);
void unload_dacast_data(void);

void dacast_ingests_load_data(const char *server, const char *key);
struct dacast_ingest *dacast_ingest(const char *key);
