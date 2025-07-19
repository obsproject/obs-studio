//
//  libobs-metal-Bridging-Header.h
//  libobs-metal
//
//  Created by Patrick Heyer on 16.04.24.
//

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
