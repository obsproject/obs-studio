//
//  TestCard.h
//  dal-plugin
//
//  Created by John Boiles  on 5/8/20.
//

#import <Foundation/Foundation.h>
#import <CoreGraphics/CoreGraphics.h>

void DrawTestCardWithFrame(CGContextRef context, NSRect frame);
void DrawDialWithFrame(NSRect frame, CGFloat rotation);

NSImage *ImageOfTestCardWithSize(NSSize imageSize);
