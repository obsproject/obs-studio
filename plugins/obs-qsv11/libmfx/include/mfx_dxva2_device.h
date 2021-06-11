// Copyright (c) 2012-2019 Intel Corporation
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

#if !defined(__MFX_DXVA2_DEVICE_H)
#define __MFX_DXVA2_DEVICE_H

#include <windows.h>

#define TOSTRING(L) #L
#define STRINGIFY(L) TOSTRING(L)

#if defined(MEDIASDK_UWP_DISPATCHER)
    #if defined(MFX_D3D9_ENABLED) && !defined(MFX_FORCE_D3D9_ENABLED)
        #undef MFX_D3D9_ENABLED
        #pragma message("\n\nATTENTION:\nin file\n\t" __FILE__ " (" STRINGIFY(__LINE__) "):\nUsing of D3D9 disabled for UWP!\n\n")
    #endif
    #if defined(MFX_FORCE_D3D9_ENABLED)
        #define MFX_D3D9_ENABLED
    #endif
#else
    #define MFX_D3D9_ENABLED
    #pragma message("\n\nATTENTION:\nin file\n\t" __FILE__ " (" STRINGIFY(__LINE__) "):\nUsing of D3D9 enabled!\n\n")
#endif

#include <mfxdefs.h>

#ifdef DXVA2DEVICE_LOG
#include <stdio.h>
#define DXVA2DEVICE_TRACE(expr) printf expr;
#define DXVA2DEVICE_TRACE_OPERATION(expr) expr;
#else
#define DXVA2DEVICE_TRACE(expr)
#define DXVA2DEVICE_TRACE_OPERATION(expr)
#endif

namespace MFX
{

class DXDevice
{
public:
    // Default constructor
    DXDevice(void);
    // Destructor
    virtual
    ~DXDevice(void) = 0;

    // Initialize device using DXGI 1.1 or VAAPI interface
    virtual
    bool Init(const mfxU32 adapterNum) = 0;

    // Obtain graphic card's parameter
    mfxU32 GetVendorID(void) const;
    mfxU32 GetDeviceID(void) const;
    mfxU64 GetDriverVersion(void) const;
    mfxU64 GetLUID(void) const;

    // Provide the number of available adapters
    mfxU32 GetAdapterCount(void) const;

    // Close the object
    virtual
    void Close(void);

    // Load the required DLL module
    void LoadDLLModule(const wchar_t *pModuleName);

protected:

    // Free DLL module
    void UnloadDLLModule(void);

    // Handle to the DLL library
    HMODULE m_hModule;

    // Number of adapters available
    mfxU32 m_numAdapters;

    // Vendor ID
    mfxU32 m_vendorID;
    // Device ID
    mfxU32 m_deviceID;
    // x.x.x.x each x of two bytes
    mfxU64 m_driverVersion;
    // LUID
    mfxU64 m_luid;

private:
    // unimplemented by intent to make this class and its descendants non-copyable
    DXDevice(const DXDevice &);
    void operator=(const DXDevice &);
};

#ifdef MFX_D3D9_ENABLED
class D3D9Device : public DXDevice
{
public:
    // Default constructor
    D3D9Device(void);
    // Destructor
    virtual
        ~D3D9Device(void);

    // Initialize device using D3D v9 interface
    virtual
        bool Init(const mfxU32 adapterNum);

    // Close the object
    virtual
        void Close(void);

protected:

    // Pointer to the D3D v9 interface
    void *m_pD3D9;
    // Pointer to the D3D v9 extended interface
    void *m_pD3D9Ex;

};
#endif // MFX_D3D9_ENABLED

class DXGI1Device : public DXDevice
{
public:
    // Default constructor
    DXGI1Device(void);
    // Destructor
    virtual
    ~DXGI1Device(void);

    // Initialize device
    virtual
    bool Init(const mfxU32 adapterNum);

    // Close the object
    virtual
    void Close(void);

protected:

    // Pointer to the DXGI1 factory
    void *m_pDXGIFactory1;
    // Pointer to the current DXGI1 adapter
    void *m_pDXGIAdapter1;

};

class DXVA2Device
{
public:
    // Default constructor
    DXVA2Device(void);
    // Destructor
    ~DXVA2Device(void);

    // Initialize device using D3D v9 interface
    bool InitD3D9(const mfxU32 adapterNum);

    // Initialize device using DXGI 1.1 interface
    bool InitDXGI1(const mfxU32 adapterNum);

    // Obtain graphic card's parameter
    mfxU32 GetVendorID(void) const;
    mfxU32 GetDeviceID(void) const;
    mfxU64 GetDriverVersion(void) const;

    // Provide the number of available adapters
    mfxU32 GetAdapterCount(void) const;

    void Close(void);

protected:

#ifdef MFX_D3D9_ENABLED
    // Get vendor & device IDs by alternative way (D3D9 in Remote Desktop sessions)
    void UseAlternativeWay(const D3D9Device *pD3D9Device);
#endif // MFX_D3D9_ENABLED
    // Number of adapters available
    mfxU32 m_numAdapters;

    // Vendor ID
    mfxU32 m_vendorID;
    // Device ID
    mfxU32 m_deviceID;
    //x.x.x.x
    mfxU64 m_driverVersion;

private:
    // unimplemented by intent to make this class non-copyable
    DXVA2Device(const DXVA2Device &);
    void operator=(const DXVA2Device &);
};

} // namespace MFX

#endif // __MFX_DXVA2_DEVICE_H
