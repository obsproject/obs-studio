libobs-metal
============

This is an alpha quality implementation of a Metal renderer backend for OBS exclusive to Apple Silicon Macs. It supports all default source types, filters, and transitions provided by OBS Studio

## Overview

* The renderer backend is implemented entirely in Swift
* A C interface header is generated automatically via the `-emit-objc-header` compile flag and `@cdecl("<FUNCTION NAME>")` decorators are used to expose desired functions to `libobs`
* Only Metal Version 3 is supported (this is by design)
* Only Apple Silicon Macs are supported (this is by design)

## Implemented functionality

* Default source types are supported:
    * Color Source
    * Image Source
    * Media Source
    * SCK Capture Source
    * Browser Source
    * Capture Card and Video Capture Device Source
    * Text (Freetype 2)
* Default transitions are supported:
    * Cut
    * Fade
    * Stinger
    * Fade To Color
    * Luma Wipe
* Default filters are supported:
    * Apply LUT
    * Chroma Key
    * Color Correction
    * Crop/Pad
    * Image Mask/Blend
    * Luma Key
    * Scaling/Aspect Ratio
    * Scroll
    * Sharpen
* sRGB-aware rendering is enabled by default
* HDR output in previews and projectors is supported on screens which have EDR support
* HDR output is not tonemapped by OBS - if the screen has EDR support, the previews will always output content in their actual format
* Recording and streaming with VideoToolbox encoders works
* Preview, separate projectors, and multi-view all work (with caveats, see below)

## Known Issues

* Previews can stutter or be stuck with low FPS - will not be fully fixed before alpha release (see below)
* Not all possible encoder configurations have been tested
* Performance is not optimized (see below)

## The State Of Previews

To manually render contents into a window using Metal one has to use a `CAMetalLayer` that is set to be a `NSView`'s backing layer. This layer can provide a `CAMetalDrawable` object which the compositor will use when it renders a new frame of the desktop. This drawable can provide a texture that OBS Studio can render into to generate output like the main preview.

Because Metal is much more integrated with macOS than OpenGL and designed with energy efficiency in mind, a `CAMetalLayer` will never provide more drawables than necessary, which means that there can be at most 3 drawables "in flight". If all available drawables are in use (either by OBS Studio to render into or by the compositor to render the desktop output) a request for a new drawable will block until an old drawable expires and a new one has been generated.

This means that if OBS renders at a higher framerate than the operating system's compositor, it will exhaust this budget and OBS Studio's renderer will be stalled and will have to wait until a new drawable is available. This effectively means that OBS Studio's maximum frame rate is limited to the operating system's screen refresh interval.

The current implementation avoids the issue of stalling OBS Studio's video render framerate, at the cost of possible framerate issues with the preview itself. OBS will always render a preview at its own framerate (which can be higher but also lower than the operating system's refresh interval) and callback provided to macOS will be used instead to copy (or "blit") this preview texture into a drawable that is only kept around as short as necessary to finish this copy operation.

This decouples the update of previews from the rendering of their contents, but obviously makes this blit operation dependent on a projector having finished rendering, as otherwise the callback might blit an incomplete preview or multi-view. It is this synchronization that can lead to slow and "choppy" frame rates if the refresh interval of the operating system and the interval at which OBS can finish rendering a preview are too misaligned.

**Note:** This is a known issue and work on a fix or better implementation of preview rendering is in progress. As the way `CAMetalLayer` works is the opposite of the way `DXGISwapChain`s work, it requires a lot more resource management and housekeeping in the Metal backend to get right.

## On Performance

Compiled in Release configuration the Metal renderer already has about the same CPU impact and render times as the OpenGL renderer on an M1 Mac even though neither the Swift code nor the Metal code has been optimized in any way. The late generation (and switches) of pipeline states and buffers is a costly operation and the way OBS Studio's renderer operates puts a natural ceiling on the performance improvements the Metal renderer could achieve (as it does lots of small render operations but with a lot of context switching between CPU and GPU).

In Debug mode the performance is a bit worse, but that's in part due to Xcode using the debug variant of the Metal framework, which allows inspection and reflection on all Metal types, including live previews of textures, buffers, debugging of shaders, and more.

Usually one would prefer to upload all data in big batches (preferably into a big `MTLHeap` object) and then pick and choose elements for each render pass to limit the switch between CPU and GPU, but this is not compatible with how OBS Studio's renderer works at this moment.

**Note:** All these observations are based on OBS Studio's own CPU and render time statistics which are flawed as the clock speeds of either CPU and GPU are not taken into account.

## Required Fixes and Workarounds

* Metal Shader Language is stricter than HLSL and GLSL and does not allow type punning or implicit casting - all type conversions have to be explicit - and commonly allows only a specific set of types for vector data, colour data, or UV coordinates
    * The transpiler has to force conversions to unsigned integers and unsigned integer vectors for texture `Load` calls because `libobs` shaders depend on the implicit conversion of a 32-bit float vector to integer values when passed to the texture's load command (`read` in MSL)
    * Metal has no support for BGRX/RGBX formats, color always has to be specified using a vector of 4 floats, some `libobs` shaders assume BGRX and only provide a `float3` value in their pixel shaders. Transpiled Metal shaders instead return a `float4` with a `1.0` alpha value
    * This might not be exhaustive, as other - so far untested - shaders might depend on other implicit conversions of HLSL/GLSL and will require additional workarounds and wrapping of existing code to return the correct types expected by MSL
* Metal does not support unpacking `UInt32` values into a `float4` in vertex data provided via the `[[stage_in]]` attribute to benefit from vertex fetch (where the pipeline itself is made aware of the buffer layout via a vertex descriptor and thus fetches the data from the buffer as needed) vs. the classic "vertex push" method
    * This is commonly used in `libobs` to provide color buffer data - to fix this, the values are unpacked and converted into a `float4` when the GPU buffers are created for a vertex buffer
* There is no explicit clear command in Metal, as clears are implemented as a command that is run when a render target (or more precisely a tile of the render target) is loaded into tile memory for a render pass. If no render pass occurs, no load command is executed and the render target is not cleared. OBS Studio depends on a "clear" call actually clearing the texture, thus an explicit (but lightweight) draw call is scheduled to ensure that the render target is loaded, cleared, and stored.
