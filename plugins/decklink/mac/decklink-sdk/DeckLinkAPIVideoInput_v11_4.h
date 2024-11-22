/* -LICENSE-START-
** Copyright (c) 2019 Blackmagic Design
**
** Permission is hereby granted, free of charge, to any person or organization
** obtaining a copy of the software and accompanying documentation covered by
** this license (the "Software") to use, reproduce, display, distribute,
** execute, and transmit the Software, and to prepare derivative works of the
** Software, and to permit third-parties to whom the Software is furnished to
** do so, all subject to the following:
**
** The copyright notices in the Software and this entire statement, including
** the above license grant, this restriction and the following disclaimer,
** must be included in all copies of the Software, in whole or in part, and
** all derivative works of the Software, unless such copies or derivative
** works are solely in the form of machine-executable object code generated by
** a source language processor.
**
** THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
** IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
** FITNESS FOR A PARTICULAR PURPOSE, TITLE AND NON-INFRINGEMENT. IN NO EVENT
** SHALL THE COPYRIGHT HOLDERS OR ANYONE DISTRIBUTING THE SOFTWARE BE LIABLE
** FOR ANY DAMAGES OR OTHER LIABILITY, WHETHER IN CONTRACT, TORT OR OTHERWISE,
** ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
** DEALINGS IN THE SOFTWARE.
** -LICENSE-END-
*/

#ifndef BMD_DECKLINKAPIVIDEOINPUT_v11_4_H
#define BMD_DECKLINKAPIVIDEOINPUT_v11_4_H

#include "DeckLinkAPI.h"
#include "DeckLinkAPIVideoInput_v11_5_1.h"

// Type Declarations

BMD_CONST REFIID IID_IDeckLinkInput_v11_4                               = /* 2A88CF76-F494-4216-A7EF-DC74EEB83882 */ { 0x2A,0x88,0xCF,0x76,0xF4,0x94,0x42,0x16,0xA7,0xEF,0xDC,0x74,0xEE,0xB8,0x38,0x82 };

/* Interface IDeckLinkInput_v11_4 - Created by QueryInterface from IDeckLink. */

class BMD_PUBLIC IDeckLinkInput_v11_4 : public IUnknown
{
public:
    virtual HRESULT DoesSupportVideoMode (/* in */ BMDVideoConnection connection /* If a value of 0 is specified, the caller does not care about the connection */, /* in */ BMDDisplayMode requestedMode, /* in */ BMDPixelFormat requestedPixelFormat, /* in */ BMDSupportedVideoModeFlags flags, /* out */ bool* supported) = 0;
    virtual HRESULT GetDisplayMode (/* in */ BMDDisplayMode displayMode, /* out */ IDeckLinkDisplayMode** resultDisplayMode) = 0;
    virtual HRESULT GetDisplayModeIterator (/* out */ IDeckLinkDisplayModeIterator** iterator) = 0;
    virtual HRESULT SetScreenPreviewCallback (/* in */ IDeckLinkScreenPreviewCallback* previewCallback) = 0;

    /* Video Input */

    virtual HRESULT EnableVideoInput (/* in */ BMDDisplayMode displayMode, /* in */ BMDPixelFormat pixelFormat, /* in */ BMDVideoInputFlags flags) = 0;
    virtual HRESULT DisableVideoInput (void) = 0;
    virtual HRESULT GetAvailableVideoFrameCount (/* out */ uint32_t* availableFrameCount) = 0;
    virtual HRESULT SetVideoInputFrameMemoryAllocator (/* in */ IDeckLinkMemoryAllocator* theAllocator) = 0;

    /* Audio Input */

    virtual HRESULT EnableAudioInput (/* in */ BMDAudioSampleRate sampleRate, /* in */ BMDAudioSampleType sampleType, /* in */ uint32_t channelCount) = 0;
    virtual HRESULT DisableAudioInput (void) = 0;
    virtual HRESULT GetAvailableAudioSampleFrameCount (/* out */ uint32_t* availableSampleFrameCount) = 0;

    /* Input Control */

    virtual HRESULT StartStreams (void) = 0;
    virtual HRESULT StopStreams (void) = 0;
    virtual HRESULT PauseStreams (void) = 0;
    virtual HRESULT FlushStreams (void) = 0;
    virtual HRESULT SetCallback (/* in */ IDeckLinkInputCallback_v11_5_1* theCallback) = 0;

    /* Hardware Timing */

    virtual HRESULT GetHardwareReferenceClock (/* in */ BMDTimeScale desiredTimeScale, /* out */ BMDTimeValue* hardwareTime, /* out */ BMDTimeValue* timeInFrame, /* out */ BMDTimeValue* ticksPerFrame) = 0;

protected:
    virtual ~IDeckLinkInput_v11_4 () {} // call Release method to drop reference count
};

#endif /* defined(BMD_DECKLINKAPIVIDEOINPUT_v11_4_H) */
