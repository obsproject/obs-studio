/******************************************************************************
    Copyright (C) 2014 by Ruwen Hahn <palana@stunned.de>

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

#include "../util/c99defs.h"
#include "video-io.h"

enum video_format video_format_from_fourcc(uint32_t fourcc)
{
	switch (fourcc) {
		case 'UYVY':
		case 'HDYC':
		case 'UYNV':
		case 'UYNY':
		case 'uyv1':
		case '2vuy':
		case '2Vuy':
			return VIDEO_FORMAT_UYVY;

		case 'YUY2':
		case 'Y422':
		case 'V422':
		case 'VYUY':
		case 'YUNV':
		case 'yuv2':
		case 'yuvs':
			return VIDEO_FORMAT_YUY2;

		case 'YVYU':
			return VIDEO_FORMAT_YVYU;
		
	}
	return VIDEO_FORMAT_NONE;
}

