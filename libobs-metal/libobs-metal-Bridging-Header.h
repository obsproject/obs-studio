/******************************************************************************
 Copyright (C) 2024 by Patrick Heyer <PatTheMav@users.noreply.github.com>

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

#import <util/base.h>
#import <util/cf-parser.h>
#import <util/cf-lexer.h>

#import <obs.h>

#import <graphics/graphics.h>
#import <graphics/device-exports.h>
#import <graphics/vec2.h>
#import <graphics/matrix3.h>
#import <graphics/matrix4.h>
#import <graphics/shader-parser.h>

static const char *const device_name = "Metal";
static const char *const preprocessor_name = "_Metal";
