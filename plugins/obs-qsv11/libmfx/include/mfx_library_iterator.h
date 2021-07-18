// Copyright (c) 2012-2020 Intel Corporation
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

#if !defined(__MFX_LIBRARY_ITERATOR_H)
#define __MFX_LIBRARY_ITERATOR_H


#include <mfxvideo.h>

#if !defined(MEDIASDK_UWP_DISPATCHER)
#include "mfx_win_reg_key.h"
#endif

#include "mfx_driver_store_loader.h"

#include "mfx_dispatcher.h"

namespace MFX
{

// declare desired storage ID
enum
{
#if defined (MFX_TRACER_WA_FOR_DS)
    MFX_UNKNOWN_KEY             = -1,
    MFX_TRACER                  = 0,
    MFX_DRIVER_STORE            = 1,
    MFX_CURRENT_USER_KEY        = 2,
    MFX_LOCAL_MACHINE_KEY       = 3,
    MFX_APP_FOLDER              = 4,
    MFX_PATH_MSDK_FOLDER        = 5,
    MFX_STORAGE_ID_FIRST = MFX_TRACER,
    MFX_STORAGE_ID_LAST = MFX_PATH_MSDK_FOLDER
#else
    MFX_UNKNOWN_KEY             = -1,
    MFX_DRIVER_STORE            = 0,
    MFX_CURRENT_USER_KEY        = 1,
    MFX_LOCAL_MACHINE_KEY       = 2,
    MFX_APP_FOLDER              = 3,
    MFX_PATH_MSDK_FOLDER        = 4,
    MFX_STORAGE_ID_FIRST    = MFX_DRIVER_STORE,
    MFX_STORAGE_ID_LAST     = MFX_PATH_MSDK_FOLDER
#endif
};

// Try to initialize using given implementation type. Select appropriate type automatically in case of MFX_IMPL_VIA_ANY.
// Params: adapterNum - in, pImplInterface - in/out, pVendorID - out, pDeviceID - out
mfxStatus SelectImplementationType(const mfxU32 adapterNum, mfxIMPL *pImplInterface, mfxU32 *pVendorID, mfxU32 *pDeviceID);

const mfxU32 msdk_disp_path_len = 1024;

class MFXLibraryIterator
{
public:
    // Default constructor
    MFXLibraryIterator(void);
    // Destructor
    ~MFXLibraryIterator(void);

    // Initialize the iterator
    mfxStatus Init(eMfxImplType implType, mfxIMPL implInterface, const mfxU32 adapterNum, int storageID);

    // Get the next library path
    mfxStatus SelectDLLVersion(wchar_t *pPath, size_t pathSize,
                               eMfxImplType *pImplType, mfxVersion minVersion);

    // Return interface type on which Intel adapter was found (if any): D3D9 or D3D11
    mfxIMPL GetImplementationType();

    // Retrun registry subkey name on which dll was selected after sucesfull call to selectDllVesion
    bool GetSubKeyName(wchar_t *subKeyName, size_t length) const;

    int  GetStorageID() const { return m_StorageID; }
protected:

    // Release the iterator
    void Release(void);

    // Initialize the registry iterator
    mfxStatus InitRegistry(int storageID);
#if defined(MFX_TRACER_WA_FOR_DS)
    // Initialize the registry iterator for searching for tracer
    mfxStatus InitRegistryTracer();
#endif
    // Initialize the app/module folder iterator
    mfxStatus InitFolder(eMfxImplType implType, const wchar_t * path, const int storageID);


    eMfxImplType m_implType;                                    // Required library implementation
    mfxIMPL m_implInterface;                                    // Required interface (D3D9, D3D11)

    mfxU32 m_vendorID;                                          // (mfxU32) property of used graphic card
    mfxU32 m_deviceID;                                          // (mfxU32) property of used graphic card
    bool   m_bIsSubKeyValid;
    wchar_t m_SubKeyName[MFX_MAX_REGISTRY_KEY_NAME];            // registry subkey for selected module loaded
    int    m_StorageID;

#if !defined(MEDIASDK_UWP_DISPATCHER)
    WinRegKey m_baseRegKey;                                     // (WinRegKey) main registry key
#endif

    mfxU32 m_lastLibIndex;                                      // (mfxU32) index of previously returned library
    mfxU32 m_lastLibMerit;                                      // (mfxU32) merit of previously returned library

    wchar_t  m_path[msdk_disp_path_len];

    DriverStoreLoader m_driverStoreLoader;                      // for loading MediaSDK from DriverStore

private:
    // unimplemented by intent to make this class non-copyable
    MFXLibraryIterator(const MFXLibraryIterator &);
    void operator=(const MFXLibraryIterator &);
};

} // namespace MFX

#endif // __MFX_LIBRARY_ITERATOR_H
