// Copyright (C) 2022 DEV47APPS, github.com/dev47apps
#pragma once

int run_command(const char *command);
bool module_loaded(const char *module);

bool audio_possible();
void audio_stop(void *data);
void *audio_start(obs_output_t *output);

bool video_possible();
void video_stop(void *data);
void *video_start(obs_output_t *output);
