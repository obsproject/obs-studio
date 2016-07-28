/******************************************************************************
    Copyright (C) 2016 by João Portela <email@joaoportela.net>

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 2 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.
******************************************************************************/

/* Functions to setup obs input, encoders and ouput. */

#pragma once

#include<vector>
#include<string>
#include<obs.hpp>

struct Outputs {
	OBSEncoder video_encoder;
	OBSEncoder audio_encoder;
	std::vector<OBSOutput> outputs;
};

/**
 * Setup input to capture monitor \p monitor.
 */
OBSSource setup_input(int monitor);

/**
 * Setup output to multiple files using the specified encoder and bitrate.
 *
 * Outputs are not started inside this function. Remember to do obs_output_start
 * to start the actual recording. Destroying an OBSOutput reference will stop the
 * recording if you have started it.
 *
 * @return references to the new outputs.
 */
Outputs setup_outputs(std::string video_encoder_id, int video_bitrate, std::vector<std::string> output_paths);
