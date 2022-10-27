/******************************************************************************
    Copyright (C) 2022-2026 pkv <pkv@obsproject.com>

    This file is part of win-asio. It adapts to C the IASIO interface.
    It uses the Steinberg ASIO SDK, which is licensed under the GNU GPL v3.

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.
******************************************************************************/

#ifndef IASIODRV_H
#define IASIODRV_H
#pragma once
#include "asio.h"
#include <windows.h>
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

#define ASIO_QueryInterface(dev, riid, ppv) (dev)->asio->lpVtbl->QueryInterface((dev)->asio,(riid),(ppv))
#define ASIO_AddRef(dev)                  (dev)->asio->lpVtbl->AddRef((dev)->asio)
#define ASIO_Release(dev)                 (dev)->asio->lpVtbl->Release((dev)->asio)

#define ASIO_Init(dev, sys)                (dev)->asio->lpVtbl->init((dev)->asio,(sys))
#define ASIO_GetDriverName(dev, name)      (dev)->asio->lpVtbl->getDriverName((dev)->asio,(name))
#define ASIO_GetDriverVersion(dev)        (dev)->asio->lpVtbl->getDriverVersion((dev)->asio)
#define ASIO_GetErrorMessage(dev, str)     (dev)->asio->lpVtbl->getErrorMessage((dev)->asio,(str))

#define ASIO_Start(dev)                   (dev)->asio->lpVtbl->start((dev)->asio)
#define ASIO_Stop(dev)                    (dev)->asio->lpVtbl->stop((dev)->asio)

#define ASIO_GetChannels(dev, in, out)      (dev)->asio->lpVtbl->getChannels((dev)->asio,(in),(out))
#define ASIO_GetLatencies(dev, in, out)     (dev)->asio->lpVtbl->getLatencies((dev)->asio,(in),(out))

#define ASIO_GetBufferSize(dev, min, max, pref, gran)     (dev)->asio->lpVtbl->getBufferSize((dev)->asio,(min),(max),(pref),(gran))

#define ASIO_CanSampleRate(dev, rate)      (dev)->asio->lpVtbl->canSampleRate((dev)->asio,(rate))
#define ASIO_GetSampleRate(dev, rate)      (dev)->asio->lpVtbl->getSampleRate((dev)->asio,(rate))
#define ASIO_SetSampleRate(dev, rate)      (dev)->asio->lpVtbl->setSampleRate((dev)->asio,(rate))

#define ASIO_GetClockSources(dev, clocks, num)     (dev)->asio->lpVtbl->getClockSources((dev)->asio,(clocks),(num))

#define ASIO_SetClockSource(dev, idx)      (dev)->asio->lpVtbl->setClockSource((dev)->asio,(idx))

#define ASIO_GetSamplePosition(dev, pos, ts)     (dev)->asio->lpVtbl->getSamplePosition((dev)->asio,(pos),(ts))

#define ASIO_GetChannelInfo(dev, info)     (dev)->asio->lpVtbl->getChannelInfo((dev)->asio,(info))

#define ASIO_CreateBuffers(dev, infos, n, size, cb) (dev)->asio->lpVtbl->createBuffers((dev)->asio,(infos),(n),(size),(cb))

#define ASIO_DisposeBuffers(dev)          (dev)->asio->lpVtbl->disposeBuffers((dev)->asio)

#define ASIO_ControlPanel(dev)            (dev)->asio->lpVtbl->controlPanel((dev)->asio)

#define ASIO_Future(dev, sel, opt)          (dev)->asio->lpVtbl->future((dev)->asio,(sel),(opt))

#define ASIO_OutputReady(dev)             (dev)->asio->lpVtbl->outputReady((dev)->asio)

#ifdef __cplusplus
}
#endif

#endif // IASIODRV_H
