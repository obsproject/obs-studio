/* Copyright (c) 2022-2025 pkv <pkv@obsproject.com>
 * This file is part of win-asio.
 *
 * This file uses the Steinberg ASIO SDK, which is licensed under the GNU GPL v3.
 *
 * This file and all modifications by pkv <pkv@obsproject.com> are licensed under
 * the GNU General Public License, version 3 or later, to comply with the SDK license.
 */

// this header adapts to C the IASIO interface
#ifndef IASIODRV_H
#define IASIODRV_H

#include <windows.h>
#include "asio.h"
#include <basetyps.h>

#ifdef __cplusplus
extern "C" {
#endif

extern const IID IID_IASIO;

typedef struct IASIO IASIO;

typedef struct IASIOVtbl {
	HRESULT(STDMETHODCALLTYPE *QueryInterface)(IASIO *This, REFIID riid, void **ppvObject);
	ULONG(STDMETHODCALLTYPE *AddRef)(IASIO *This);
	ULONG(STDMETHODCALLTYPE *Release)(IASIO *This);

	ASIOBool(STDMETHODCALLTYPE *init)(IASIO *This, void *sysHandle);
	void(STDMETHODCALLTYPE *getDriverName)(IASIO *This, char *name);
	long(STDMETHODCALLTYPE *getDriverVersion)(IASIO *This);
	void(STDMETHODCALLTYPE *getErrorMessage)(IASIO *This, char *string);
	ASIOError(STDMETHODCALLTYPE *start)(IASIO *This);
	ASIOError(STDMETHODCALLTYPE *stop)(IASIO *This);
	ASIOError(STDMETHODCALLTYPE *getChannels)(IASIO *This, long *numInputChannels, long *numOutputChannels);
	ASIOError(STDMETHODCALLTYPE *getLatencies)(IASIO *This, long *inputLatency, long *outputLatency);
	ASIOError(STDMETHODCALLTYPE *getBufferSize)(IASIO *This, long *minSize, long *maxSize, long *preferredSize,
						    long *granularity);
	ASIOError(STDMETHODCALLTYPE *canSampleRate)(IASIO *This, double sampleRate);
	ASIOError(STDMETHODCALLTYPE *getSampleRate)(IASIO *This, double *sampleRate);
	ASIOError(STDMETHODCALLTYPE *setSampleRate)(IASIO *This, double sampleRate);
	ASIOError(STDMETHODCALLTYPE *getClockSources)(IASIO *This, void *clocks, long *numSources);
	ASIOError(STDMETHODCALLTYPE *setClockSource)(IASIO *This, long reference);
	ASIOError(STDMETHODCALLTYPE *getSamplePosition)(IASIO *This, void *sPos, void *tStamp);
	ASIOError(STDMETHODCALLTYPE *getChannelInfo)(IASIO *This, void *info);
	ASIOError(STDMETHODCALLTYPE *createBuffers)(IASIO *This, void *bufferInfos, long numChannels, long bufferSize,
						    void *callbacks);
	ASIOError(STDMETHODCALLTYPE *disposeBuffers)(IASIO *This);
	ASIOError(STDMETHODCALLTYPE *controlPanel)(IASIO *This);
	ASIOError(STDMETHODCALLTYPE *future)(IASIO *This, long selector, void *opt);
	ASIOError(STDMETHODCALLTYPE *outputReady)(IASIO *This);
} IASIOVtbl;

struct IASIO {
	const IASIOVtbl *lpVtbl;
};

#ifdef __cplusplus
}
#endif

#endif // IASIODRV_H
