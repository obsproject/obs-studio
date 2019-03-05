// Copyright (c) 2017-2019 Intel Corporation
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in all
// copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
// SOFTWARE.

#pragma once

#ifndef __INTEL_API_FACTORY_H
#define __INTEL_API_FACTORY_H

#if !defined(OPEN_SOURCE)
#if defined(MEDIASDK_UWP_PROCTABLE) && !defined(MEDIASDK_DFP_LOADER) && !defined(MEDIASDK_ARM_LOADER)

#ifdef __cplusplus
extern "C"
{
#endif /* __cplusplus */

HRESULT APIENTRY InitialiseMediaSession(_Out_ HANDLE* handle, _In_ LPVOID lpParam, _Reserved_ LPVOID lpReserved);
HRESULT APIENTRY DisposeMediaSession(_In_ const HANDLE handle);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif // defined(MEDIASDK_UWP_PROCTABLE) && !defined(MEDIASDK_DFP_LOADER) && !defined(MEDIASDK_ARM_LOADER)
#else // !defined(OPEN_SOURCE)
#if defined(MEDIASDK_UWP_PROCTABLE) && !defined(MEDIASDK_ARM_LOADER)

#ifdef __cplusplus
extern "C"
{
#endif /* __cplusplus */

    HRESULT APIENTRY InitialiseMediaSession(_Out_ HANDLE* handle, _In_ LPVOID lpParam, _Reserved_ LPVOID lpReserved);
    HRESULT APIENTRY DisposeMediaSession(_In_ const HANDLE handle);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif //defined(MEDIASDK_UWP_PROCTABLE) && !defined(MEDIASDK_ARM_LOADER)
#endif // !defined(OPEN_SOURCE)

#endif // __INTEL_API_FACTORY_H
