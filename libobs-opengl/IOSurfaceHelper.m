#include "IOSurfaceHelper.h"

#include <OpenGL/OpenGL.h>

#import <AppKit/AppKit.h>

void saveCGImageToFile(CGImageRef image, NSString *filePath, CFStringRef fileType)
{
    if (!image || !filePath || !fileType) {
        NSLog(@"Invalid parameters. Cannot save CGImage.");
        return;
    }

    CFURLRef url = (__bridge CFURLRef) [NSURL fileURLWithPath:filePath];
    if (!url) {
        NSLog(@"Failed to create URL for file path.");
        return;
    }

    CGImageDestinationRef destination = CGImageDestinationCreateWithURL(url, fileType, 1, NULL);
    if (!destination) {
        NSLog(@"Failed to create CGImageDestination.");
        return;
    }

    CGImageDestinationAddImage(destination, image, NULL);

    if (!CGImageDestinationFinalize(destination)) {
        NSLog(@"Failed to write the image to file.");
    } else {
        NSLog(@"Image saved successfully to %@", filePath);
    }

    // Cleanup
    CFRelease(destination);
}

void writeIOSurfaceContents(IOSurfaceRef surface, NSString *filePath)
{
    if (!surface) {
        NSLog(@"Invalid IOSurfaceRef.");
        return;
    }

    kern_return_t result = IOSurfaceLock(surface, kIOSurfaceLockReadOnly, NULL);
    if (result != KERN_SUCCESS) {
        NSLog(@"Failed to lock IOSurface.");
        return;
    }

    size_t width = IOSurfaceGetWidth(surface);
    size_t height = IOSurfaceGetHeight(surface);
    size_t bytesPerRow = IOSurfaceGetBytesPerRow(surface);
    void *baseAddress = IOSurfaceGetBaseAddress(surface);

    NSLog(@"IOSurface Properties:");
    NSLog(@"Width: %zu, Height: %zu, Bytes Per Row: %zu", width, height, bytesPerRow);

    // Assuming 4 bytes per pixel (e.g., RGBA)
    /*
    uint8_t *pixels = (uint8_t *)baseAddress;
    for (size_t y = 0; y < height; y++) {
	for (size_t x = 0; x < width; x++) {
	    size_t pixelOffset = y * bytesPerRow + x * 4; // 4 bytes per pixel (e.g., RGBA)
	    uint8_t red = pixels[pixelOffset];
	    uint8_t green = pixels[pixelOffset + 1];
	    uint8_t blue = pixels[pixelOffset + 2];
	    uint8_t alpha = pixels[pixelOffset + 3];

	    NSLog(@"Pixel (%zu, %zu): R=%u G=%u B=%u A=%u", x, y, red, green, blue, alpha);
	}
    }*/
    CGColorSpaceRef colorSpace = CGColorSpaceCreateWithName(kCGColorSpaceSRGB);
    CGDataProviderRef provider = CGDataProviderCreateWithData(NULL, baseAddress, height * bytesPerRow, NULL);

    CGImageRef image = CGImageCreate(width, height, 8, 32, bytesPerRow, colorSpace,
                                     kCGImageAlphaPremultipliedFirst | kCGBitmapByteOrder32Little, provider, NULL,
                                     false, kCGRenderingIntentDefault);

    saveCGImageToFile(image, filePath, kUTTypePNG);

    CGDataProviderRelease(provider);
    CGColorSpaceRelease(colorSpace);
    CGImageRelease(image);

    IOSurfaceUnlock(surface, kIOSurfaceLockReadOnly, NULL);

    NSLog(@"Completed examining IOSurface.");
}

