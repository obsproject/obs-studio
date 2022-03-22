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

#include <windows.h>
#include <stringapiset.h>

#include <new>
#include <memory>

#include "mfx_dispatcher.h"
#include "mfx_load_dll.h"
#include "mfx_dispatcher_log.h"
#include "mfx_library_iterator.h"
#include "mfx_critical_section.h"

#if defined(MEDIASDK_UWP_DISPATCHER)
#include "mfx_dispatcher_uwp.h"
#endif

#include <string.h> /* for memset on Linux */

#include <stdlib.h> /* for qsort on Linux */
#include "mfx_load_plugin.h"
#include "mfx_plugin_hive.h"

// module-local definitions
namespace
{

    const
    struct
    {
        // instance implementation type
        eMfxImplType implType;
        // real implementation
        mfxIMPL impl;
        // adapter numbers
        mfxU32 adapterID;

    } implTypes[] =
    {
        // MFX_IMPL_AUTO case
        {MFX_LIB_HARDWARE, MFX_IMPL_HARDWARE,  0},
        {MFX_LIB_SOFTWARE, MFX_IMPL_SOFTWARE,  0},

        // MFX_IMPL_ANY case
        {MFX_LIB_HARDWARE, MFX_IMPL_HARDWARE,  0},
        {MFX_LIB_HARDWARE, MFX_IMPL_HARDWARE2, 1},
        {MFX_LIB_HARDWARE, MFX_IMPL_HARDWARE3, 2},
        {MFX_LIB_HARDWARE, MFX_IMPL_HARDWARE4, 3},
        {MFX_LIB_SOFTWARE, MFX_IMPL_SOFTWARE,  0},
        {MFX_LIB_SOFTWARE, MFX_IMPL_SOFTWARE | MFX_IMPL_AUDIO,  0},
#if (MFX_VERSION >= MFX_VERSION_NEXT)
        //MFX_SINGLE_THREAD case
        {MFX_LIB_HARDWARE, MFX_IMPL_HARDWARE | MFX_IMPL_EXTERNAL_THREADING, 0},
        {MFX_LIB_HARDWARE, MFX_IMPL_HARDWARE2 | MFX_IMPL_EXTERNAL_THREADING, 1},
        {MFX_LIB_HARDWARE, MFX_IMPL_HARDWARE3 | MFX_IMPL_EXTERNAL_THREADING, 2},
        {MFX_LIB_HARDWARE, MFX_IMPL_HARDWARE4 | MFX_IMPL_EXTERNAL_THREADING, 3},
#endif
    };

    const
    struct
    {
        // start index in implTypes table for specified implementation
        mfxU32 minIndex;
        // last index in implTypes table for specified implementation
        mfxU32 maxIndex;

    } implTypesRange[] =
    {
        {0, 1},  // MFX_IMPL_AUTO
        {1, 1},  // MFX_IMPL_SOFTWARE
        {0, 0},  // MFX_IMPL_HARDWARE
        {2, 6},  // MFX_IMPL_AUTO_ANY
        {2, 5},  // MFX_IMPL_HARDWARE_ANY
        {3, 3},  // MFX_IMPL_HARDWARE2
        {4, 4},  // MFX_IMPL_HARDWARE3
        {5, 5},  // MFX_IMPL_HARDWARE4
        {2, 6},  // MFX_IMPL_RUNTIME, same as MFX_IMPL_HARDWARE_ANY
#if (MFX_VERSION >= MFX_VERSION_NEXT)
        {8, 11},  // MFX_SINGLE_THREAD,
#endif
        {7, 7}   // MFX_IMPL_AUDIO
    };

    MFX::mfxCriticalSection dispGuard = 0;

} // namespace

using namespace MFX;

#if !defined(MEDIASDK_UWP_DISPATCHER)

//
// Implement DLL exposed functions. MFXInit and MFXClose have to do
// slightly more than other. They require to be implemented explicitly.
// All other functions are implemented implicitly.
//

typedef MFXVector<MFX_DISP_HANDLE_EX*> HandleVector;
typedef MFXVector<mfxStatus>        StatusVector;

struct VectorHandleGuard
{
    VectorHandleGuard(HandleVector& aVector): m_vector(aVector) {}
    ~VectorHandleGuard()
    {
        HandleVector::iterator it = m_vector.begin(),
                               et = m_vector.end();
        for ( ; it != et; ++it)
        {
            delete *it;
        }
    }

    HandleVector& m_vector;
private:
    void operator=(const VectorHandleGuard&);
};


static int HandleSort (const void * plhs, const void * prhs)
{
    const MFX_DISP_HANDLE_EX * lhs = *(const MFX_DISP_HANDLE_EX **)plhs;
    const MFX_DISP_HANDLE_EX * rhs = *(const MFX_DISP_HANDLE_EX **)prhs;

    // prefer HW implementation
    if (lhs->implType != MFX_LIB_HARDWARE && rhs->implType == MFX_LIB_HARDWARE)
    {
        return 1;
    }
    if (lhs->implType == MFX_LIB_HARDWARE && rhs->implType != MFX_LIB_HARDWARE)
    {
        return -1;
    }

    // prefer integrated GPU
    if (lhs->mediaAdapterType != MFX_MEDIA_INTEGRATED && rhs->mediaAdapterType == MFX_MEDIA_INTEGRATED)
    {
        return 1;
    }
    if (lhs->mediaAdapterType == MFX_MEDIA_INTEGRATED && rhs->mediaAdapterType != MFX_MEDIA_INTEGRATED)
    {
        return -1;
    }

    // prefer dll with lower API version
    if (lhs->actualApiVersion < rhs->actualApiVersion)
    {
        return -1;
    }
    if (rhs->actualApiVersion < lhs->actualApiVersion)
    {
        return 1;
    }

    // if versions are equal prefer library with HW
    if (lhs->loadStatus == MFX_WRN_PARTIAL_ACCELERATION && rhs->loadStatus == MFX_ERR_NONE)
    {
        return 1;
    }
    if (lhs->loadStatus == MFX_ERR_NONE && rhs->loadStatus == MFX_WRN_PARTIAL_ACCELERATION)
    {
        return -1;
    }

    return 0;
}

mfxStatus MFXInitEx(mfxInitParam par, mfxSession *session)
{
    MFX::MFXAutomaticCriticalSection guard(&dispGuard);

    DISPATCHER_LOG_BLOCK( ("MFXInitEx (impl=%s, pVer=%d.%d, ExternalThreads=%d session=0x%p\n"
        , DispatcherLog_GetMFXImplString(par.Implementation).c_str()
        , par.Version.Major
        , par.Version.Minor
        , par.ExternalThreads
        , session));

    mfxStatus mfxRes = MFX_ERR_UNSUPPORTED;
    HandleVector allocatedHandle;
    VectorHandleGuard handleGuard(allocatedHandle);

    MFX_DISP_HANDLE_EX *pHandle;
    wchar_t dllName[MFX_MAX_DLL_PATH] = { 0 };
    MFX::MFXLibraryIterator libIterator;

    // there iterators are used only if the caller specified implicit type like AUTO
    mfxU32 curImplIdx, maxImplIdx;
    // implementation method masked from the input parameter
    // special case for audio library
    const mfxIMPL implMethod = (par.Implementation & MFX_IMPL_AUDIO) ? (sizeof(implTypesRange) / sizeof(implTypesRange[0]) - 1) : (par.Implementation & (MFX_IMPL_VIA_ANY - 1));

    // implementation interface masked from the input parameter
    mfxIMPL implInterface = par.Implementation & -MFX_IMPL_VIA_ANY;
#if (MFX_VERSION >= MFX_VERSION_NEXT)
    bool isSingleThread = (implInterface & MFX_IMPL_EXTERNAL_THREADING) > 0;
    implInterface &= ~MFX_IMPL_EXTERNAL_THREADING;
#endif
    mfxIMPL implInterfaceOrig = implInterface;
    mfxVersion requiredVersion = {{MFX_VERSION_MINOR, MFX_VERSION_MAJOR}};

    // check error(s)
    if (NULL == session)
    {
        return MFX_ERR_NULL_PTR;
    }

#if (MFX_VERSION >= MFX_VERSION_NEXT)
    if (((MFX_IMPL_AUTO > implMethod) || (MFX_IMPL_SINGLE_THREAD < implMethod)) && !(par.Implementation & MFX_IMPL_AUDIO))
#else
    if (((MFX_IMPL_AUTO > implMethod) || (MFX_IMPL_RUNTIME < implMethod)) && !(par.Implementation & MFX_IMPL_AUDIO))
#endif
    {
        return MFX_ERR_UNSUPPORTED;
    }

    // set the minimal required version
    requiredVersion = par.Version;

    try
    {
        // reset the session value
        *session = 0;

        // allocate the dispatching handle and call-table
        pHandle = new MFX_DISP_HANDLE_EX(requiredVersion);
    }
    catch(...)
    {
        return MFX_ERR_MEMORY_ALLOC;
    }

    DISPATCHER_LOG_INFO((("Required API version is %u.%u\n"), requiredVersion.Major, requiredVersion.Minor));
    // particular implementation value
    mfxIMPL curImpl;

    // Load HW library or RT from system location
    curImplIdx = implTypesRange[implMethod].minIndex;
    maxImplIdx = implTypesRange[implMethod].maxIndex;
    do
    {
#if (MFX_VERSION >= MFX_VERSION_NEXT)
        if (isSingleThread && implTypes[curImplIdx].implType != MFX_LIB_HARDWARE)
            continue;
#endif

        int currentStorage = MFX::MFX_STORAGE_ID_FIRST;
        implInterface = implInterfaceOrig;
        do
        {
            // this storage will be checked below
            if (currentStorage == MFX::MFX_APP_FOLDER)
            {
                currentStorage += 1;
                continue;
            }

            // initialize the library iterator
            mfxRes = libIterator.Init(implTypes[curImplIdx].implType,
                implInterface,
                implTypes[curImplIdx].adapterID,
                currentStorage);

            // look through the list of installed SDK version,
            // looking for a suitable library with higher merit value.
            if (MFX_ERR_NONE == mfxRes)
            {

                if (
                    MFX_LIB_HARDWARE == implTypes[curImplIdx].implType
                    && (!implInterface
                    || MFX_IMPL_VIA_ANY == implInterface))
                {
                    implInterface = libIterator.GetImplementationType();
                }

                do
                {
                    eMfxImplType implType = implTypes[curImplIdx].implType;

                    // select a desired DLL
                    mfxRes = libIterator.SelectDLLVersion(dllName,
                        sizeof(dllName) / sizeof(dllName[0]),
                        &implType,
                        pHandle->apiVersion);
                    if (MFX_ERR_NONE != mfxRes)
                    {
                        break;
                    }
                    DISPATCHER_LOG_INFO((("loading library %S\n"), dllName));
                    // try to load the selected DLL
                    curImpl = implTypes[curImplIdx].impl;
#if (MFX_VERSION >= MFX_VERSION_NEXT)
                    if (isSingleThread)
                        curImpl |= MFX_IMPL_EXTERNAL_THREADING;
#endif
                    mfxRes = pHandle->LoadSelectedDLL(dllName, implType, curImpl, implInterface, par);
                    // unload the failed DLL
                    if (MFX_ERR_NONE != mfxRes)
                    {
                        pHandle->Close();
                        continue;
                    }

                    mfxPlatform platform = { MFX_PLATFORM_UNKNOWN, 0, MFX_MEDIA_UNKNOWN };
                    if (pHandle->callTable[eMFXVideoCORE_QueryPlatform])
                    {
                        mfxRes = MFXVideoCORE_QueryPlatform((mfxSession)pHandle, &platform);
                        if (MFX_ERR_NONE != mfxRes)
                        {
                            DISPATCHER_LOG_WRN(("MFXVideoCORE_QueryPlatform failed, rejecting loaded library\n"));
                            pHandle->Close();
                            continue;
                        }
                    }
                    pHandle->mediaAdapterType = platform.MediaAdapterType;
                    DISPATCHER_LOG_INFO((("media adapter type is %d\n"), pHandle->mediaAdapterType));

                    libIterator.GetSubKeyName(pHandle->subkeyName, sizeof(pHandle->subkeyName) / sizeof(pHandle->subkeyName[0]));
                    pHandle->storageID = libIterator.GetStorageID();
                    allocatedHandle.push_back(pHandle);
                    pHandle = new MFX_DISP_HANDLE_EX(requiredVersion);

                } while (MFX_ERR_NONE != mfxRes);
            }

            // select another place for loading engine
            currentStorage += 1;

        } while ((MFX_ERR_NONE != mfxRes) && (MFX::MFX_STORAGE_ID_LAST >= currentStorage));

    } while (++curImplIdx <= maxImplIdx);

    curImplIdx = implTypesRange[implMethod].minIndex;
    maxImplIdx = implTypesRange[implMethod].maxIndex;

    // Load RT from app folder (libmfxsw64 with API >= 1.10)
    do
    {
#if (MFX_VERSION >= MFX_VERSION_NEXT)
        if (isSingleThread && implTypes[curImplIdx].implType != MFX_LIB_HARDWARE)
            continue;
#endif

        implInterface = implInterfaceOrig;
        // initialize the library iterator
        mfxRes = libIterator.Init(implTypes[curImplIdx].implType,
            implInterface,
            implTypes[curImplIdx].adapterID,
            MFX::MFX_APP_FOLDER);

        if (MFX_ERR_NONE == mfxRes)
        {

            if (
                MFX_LIB_HARDWARE == implTypes[curImplIdx].implType
                && (!implInterface
                || MFX_IMPL_VIA_ANY == implInterface))
            {
                implInterface = libIterator.GetImplementationType();
            }

            do
            {
                eMfxImplType implType;

                // select a desired DLL
                mfxRes = libIterator.SelectDLLVersion(dllName,
                    sizeof(dllName) / sizeof(dllName[0]),
                    &implType,
                    pHandle->apiVersion);
                if (MFX_ERR_NONE != mfxRes)
                {
                    break;
                }
                DISPATCHER_LOG_INFO((("loading library %S\n"), dllName));

                // try to load the selected DLL
                curImpl = implTypes[curImplIdx].impl;
#if (MFX_VERSION >= MFX_VERSION_NEXT)
                if (isSingleThread)
                    curImpl |= MFX_IMPL_EXTERNAL_THREADING;
#endif
                mfxRes = pHandle->LoadSelectedDLL(dllName, implType, curImpl, implInterface, par);
                // unload the failed DLL
                if (MFX_ERR_NONE != mfxRes)
                {
                    pHandle->Close();
                }
                else
                {
                    if (pHandle->actualApiVersion.Major == 1 && pHandle->actualApiVersion.Minor <= 9)
                    {
                        // this is not RT, skip it
                        mfxRes = MFX_ERR_ABORTED;
                        break;
                    }
                    pHandle->storageID = MFX::MFX_UNKNOWN_KEY;
                    allocatedHandle.push_back(pHandle);
                    pHandle = new MFX_DISP_HANDLE_EX(requiredVersion);
                }

            } while (MFX_ERR_NONE != mfxRes);
        }
    } while ((MFX_ERR_NONE != mfxRes) && (++curImplIdx <= maxImplIdx));

    // Load HW and SW libraries using legacy default DLL search mechanism
    // set current library index again
    curImplIdx = implTypesRange[implMethod].minIndex;
    do
    {
#if (MFX_VERSION >= MFX_VERSION_NEXT)
        if (isSingleThread && implTypes[curImplIdx].implType != MFX_LIB_HARDWARE)
            continue;
#endif

        implInterface = implInterfaceOrig;

        if (par.Implementation & MFX_IMPL_AUDIO)
        {
            mfxRes = MFX::mfx_get_default_audio_dll_name(dllName,
                sizeof(dllName) / sizeof(dllName[0]),
                implTypes[curImplIdx].implType);
        }
        else
        {
            mfxRes = MFX::mfx_get_default_dll_name(dllName,
                sizeof(dllName) / sizeof(dllName[0]),
                implTypes[curImplIdx].implType);
        }

        if (MFX_ERR_NONE == mfxRes)
        {
            DISPATCHER_LOG_INFO((("loading default library %S\n"), dllName))

                // try to load the selected DLL using default DLL search mechanism
                if (MFX_LIB_HARDWARE == implTypes[curImplIdx].implType)
                {
                    if (!implInterface)
                    {
                        implInterface = MFX_IMPL_VIA_ANY;
                    }
                    mfxU32 curVendorID = 0, curDeviceID = 0;
                    mfxRes = MFX::SelectImplementationType(implTypes[curImplIdx].adapterID, &implInterface, &curVendorID, &curDeviceID);
                    if (curVendorID != INTEL_VENDOR_ID)
                        mfxRes = MFX_ERR_UNKNOWN;
                }
                if (MFX_ERR_NONE == mfxRes)
                {
                    curImpl = implTypes[curImplIdx].impl;
#if (MFX_VERSION >= MFX_VERSION_NEXT)
                    if (isSingleThread)
                        curImpl |= MFX_IMPL_EXTERNAL_THREADING;
#endif
                    // try to load the selected DLL using default DLL search mechanism
                    mfxRes = pHandle->LoadSelectedDLL(dllName,
                        implTypes[curImplIdx].implType,
                        curImpl,
                        implInterface,
                        par);
                }
                // unload the failed DLL
                if ((MFX_ERR_NONE != mfxRes) &&
                    (MFX_WRN_PARTIAL_ACCELERATION != mfxRes))
                {
                    pHandle->Close();
                }
                else
                {
                    pHandle->storageID = MFX::MFX_UNKNOWN_KEY;
                    allocatedHandle.push_back(pHandle);
                    pHandle = new MFX_DISP_HANDLE_EX(requiredVersion);
                }
        }
    }
    while ((MFX_ERR_NONE > mfxRes) && (++curImplIdx <= maxImplIdx));
    delete pHandle;

    if (allocatedHandle.size() == 0)
        return MFX_ERR_UNSUPPORTED;

    { // sort candidate list
        bool NeedSort = false;
        HandleVector::iterator first = allocatedHandle.begin(),
            it = allocatedHandle.begin(),
            et = allocatedHandle.end();
        for (it++; it != et; ++it)
            if (HandleSort(&(*first), &(*it)) != 0)
                NeedSort = true;

        // sort allocatedHandle so that the most preferred dll is at the beginning
        if (NeedSort)
            qsort(&(*allocatedHandle.begin()), allocatedHandle.size(), sizeof(MFX_DISP_HANDLE_EX*), &HandleSort);
    }
    HandleVector::iterator candidate = allocatedHandle.begin();
    // check the final result of loading
    try
    {
        pHandle = *candidate;
        //pulling up current mediasdk version, that required to match plugin version
        mfxVersion apiVerActual = { { 0, 0 } };
        mfxStatus stsQueryVersion = MFXQueryVersion((mfxSession)pHandle, &apiVerActual);

        if (MFX_ERR_NONE !=  stsQueryVersion)
        {
            DISPATCHER_LOG_ERROR((("MFXQueryVersion returned: %d, cannot load plugins\n"), mfxRes))
        }
        else
        {
            MFX::MFXPluginStorage & hive = pHandle->pluginHive;

            HandleVector::iterator it = allocatedHandle.begin(),
                                   et = allocatedHandle.end();
            for (; it != et; ++it)
            {
                // Registering default plugins set
                MFX::MFXDefaultPlugins defaultPugins(apiVerActual, *it, (*it)->implType);
                hive.insert(hive.end(), defaultPugins.begin(), defaultPugins.end());

                if ((*it)->storageID != MFX::MFX_UNKNOWN_KEY)
                {
                    // Scan HW plugins in subkeys of registry library
                    MFX::MFXPluginsInHive plgsInHive((*it)->storageID, (*it)->subkeyName, apiVerActual);
                    hive.insert(hive.end(), plgsInHive.begin(), plgsInHive.end());
                }
            }

            //setting up plugins records
            for(int i = MFX::MFX_STORAGE_ID_FIRST; i <= MFX::MFX_STORAGE_ID_LAST; i++)
            {
                MFX::MFXPluginsInHive plgsInHive(i, NULL, apiVerActual);
                hive.insert(hive.end(), plgsInHive.begin(), plgsInHive.end());
            }

            // SOLID dispatcher also loads plug-ins from file system
            MFX::MFXPluginsInFS plgsInFS(apiVerActual);
            hive.insert(hive.end(), plgsInFS.begin(), plgsInFS.end());
        }

        pHandle->callPlugInsTable[eMFXVideoUSER_Load] = (mfxFunctionPointer)MFXVideoUSER_Load;
        pHandle->callPlugInsTable[eMFXVideoUSER_LoadByPath] = (mfxFunctionPointer)MFXVideoUSER_LoadByPath;
        pHandle->callPlugInsTable[eMFXVideoUSER_UnLoad] = (mfxFunctionPointer)MFXVideoUSER_UnLoad;
        pHandle->callPlugInsTable[eMFXAudioUSER_Load] = (mfxFunctionPointer)MFXAudioUSER_Load;
        pHandle->callPlugInsTable[eMFXAudioUSER_UnLoad] = (mfxFunctionPointer)MFXAudioUSER_UnLoad;

    }
    catch(...)
    {
        DISPATCHER_LOG_ERROR((("unknown exception while loading plugins\n")))
    }

    // everything is OK. Save pointers to the output variable
    *candidate = 0; // keep this one safe from guard destructor


    //===================================

    // MFXVideoCORE_QueryPlatform call creates d3d device handle, so we have handle right after MFXInit and can't accept external handle
    // This is a workaround which calls close-init to remove that handle

    mfxFunctionPointer *actualTable = (pHandle->impl & MFX_IMPL_AUDIO) ? pHandle->callAudioTable : pHandle->callTable;
    mfxFunctionPointer pFunc;

    pFunc = actualTable[eMFXClose];
    mfxRes = (*(mfxStatus(MFX_CDECL *) (mfxSession)) pFunc) (pHandle->session);
    if (mfxRes != MFX_ERR_NONE)
        return mfxRes;

    pHandle->session = 0;
    bool callOldInit = (pHandle->impl & MFX_IMPL_AUDIO) || !actualTable[eMFXInitEx];
    pFunc = actualTable[(callOldInit) ? eMFXInit : eMFXInitEx];

    mfxVersion version(pHandle->apiVersion);
    if (callOldInit)
    {
        pHandle->loadStatus = (*(mfxStatus(MFX_CDECL *) (mfxIMPL, mfxVersion *, mfxSession *)) pFunc) (pHandle->impl | pHandle->implInterface, &version, &pHandle->session);
    }
    else
    {
        mfxInitParam initPar = par;
        initPar.Implementation = pHandle->impl | pHandle->implInterface;
        initPar.Version = version;
        pHandle->loadStatus = (*(mfxStatus(MFX_CDECL *) (mfxInitParam, mfxSession *)) pFunc) (initPar, &pHandle->session);
    }

    //===================================

    *((MFX_DISP_HANDLE_EX **) session) = pHandle;

    return pHandle->loadStatus;

} // mfxStatus MFXInitEx(mfxIMPL impl, mfxVersion *ver, mfxSession *session)

mfxStatus MFXClose(mfxSession session)
{
    MFX::MFXAutomaticCriticalSection guard(&dispGuard);

    mfxStatus mfxRes = MFX_ERR_INVALID_HANDLE;
    MFX_DISP_HANDLE *pHandle = (MFX_DISP_HANDLE *) session;

    // check error(s)
    if (pHandle)
    {
        try
        {
            // unload the DLL library
            mfxRes = pHandle->Close();

            // it is possible, that there is an active child session.
            // can't unload library in that case.
            if (MFX_ERR_UNDEFINED_BEHAVIOR != mfxRes)
            {
                // release the handle
                delete pHandle;
            }
        }
        catch(...)
        {
            mfxRes = MFX_ERR_INVALID_HANDLE;
        }
    }

    return mfxRes;

} // mfxStatus MFXClose(mfxSession session)

mfxStatus MFXVideoUSER_Load(mfxSession session, const mfxPluginUID *uid, mfxU32 version)
{
    mfxStatus sts = MFX_ERR_NONE;
    bool ErrFlag = false;
    if (!session)
    {
        DISPATCHER_LOG_ERROR((("MFXVideoUSER_Load: session=NULL\n")));
        return MFX_ERR_NULL_PTR;
    }
    MFX_DISP_HANDLE &pHandle = *(MFX_DISP_HANDLE *) session;
    if (!uid)
    {
        DISPATCHER_LOG_ERROR((("MFXVideoUSER_Load: uid=NULL\n")));
        return MFX_ERR_NULL_PTR;
    }
    DISPATCHER_LOG_INFO((("MFXVideoUSER_Load: uid=" MFXGUIDTYPE()" version=%d\n")
        , MFXGUIDTOHEX(uid)
        , version))
        size_t pluginsChecked = 0;

    for (MFX::MFXPluginStorage::iterator i = pHandle.pluginHive.begin();i != pHandle.pluginHive.end(); i++, pluginsChecked++)
    {
        if (i->PluginUID != *uid)
        {
            continue;
        }
        //check rest in records
        if (i->PluginVersion < version)
        {
            DISPATCHER_LOG_INFO((("MFXVideoUSER_Load: registered \"Plugin Version\" for GUID=" MFXGUIDTYPE()" is %d, that is smaller that requested\n")
                , MFXGUIDTOHEX(uid)
                , i->PluginVersion))
                continue;
        }
        try
        {
            sts = pHandle.pluginFactory.Create(*i);
            if( MFX_ERR_NONE != sts)
            {
                ErrFlag = (ErrFlag || (sts == MFX_ERR_UNDEFINED_BEHAVIOR));
                continue;
            }
            return MFX_ERR_NONE;
        }
        catch(...)
        {
            continue;
        }
    }

    // Specified UID was not found among individually registed plugins, now try load it from default sets if any
    for (MFX::MFXPluginStorage::iterator i = pHandle.pluginHive.begin();i != pHandle.pluginHive.end(); i++, pluginsChecked++)
    {
        if (!i->Default)
            continue;

        i->PluginUID = *uid;
        i->PluginVersion = (mfxU16)version;
        try
        {
            sts = pHandle.pluginFactory.Create(*i);
            if( MFX_ERR_NONE != sts)
            {
                ErrFlag = (ErrFlag || (sts == MFX_ERR_UNDEFINED_BEHAVIOR));
                continue;
            }
            return MFX_ERR_NONE;
        }
        catch(...)
        {
            continue;
        }
    }

    DISPATCHER_LOG_ERROR((("MFXVideoUSER_Load: cannot find registered plugin with requested UID, total plugins available=%d\n"), pHandle.pluginHive.size()));
    if (ErrFlag)
        return MFX_ERR_UNDEFINED_BEHAVIOR;
    else
        return MFX_ERR_NOT_FOUND;
}


mfxStatus MFXVideoUSER_LoadByPath(mfxSession session, const mfxPluginUID *uid, mfxU32 version, const mfxChar *path, mfxU32 len)
{
    if (!session)
    {
        DISPATCHER_LOG_ERROR((("MFXVideoUSER_LoadByPath: session=NULL\n")));
        return MFX_ERR_NULL_PTR;
    }
    MFX_DISP_HANDLE &pHandle = *(MFX_DISP_HANDLE *) session;
    if (!uid)
    {
        DISPATCHER_LOG_ERROR((("MFXVideoUSER_LoadByPath: uid=NULL\n")));
        return MFX_ERR_NULL_PTR;
    }

    DISPATCHER_LOG_INFO((("MFXVideoUSER_LoadByPath: %S uid=" MFXGUIDTYPE()" version=%d\n")
        , path
        , MFXGUIDTOHEX(uid)
        , version))

    PluginDescriptionRecord record;
    record.sName[0] = 0;

    wchar_t wPath[MAX_PLUGIN_PATH];
    int res = ::MultiByteToWideChar(CP_UTF8, MB_ERR_INVALID_CHARS, path, len, wPath, MAX_PLUGIN_PATH-1);

    if (!res)
    {
        DISPATCHER_LOG_ERROR((("MFXVideoUSER_LoadByPath: can't convert UTF-8 path to UTF-16\n")));
        return MFX_ERR_NOT_FOUND;
    }
    wPath[res]=0;

    wcscpy_s(record.sPath, MAX_PLUGIN_PATH, wPath);

    record.PluginUID = *uid;
    record.PluginVersion = (mfxU16)version;
    record.Default = true;

    try
    {
        return pHandle.pluginFactory.Create(record);
    }
    catch(...)
    {
        return MFX_ERR_NOT_FOUND;
    }
}


mfxStatus MFXVideoUSER_UnLoad(mfxSession session, const mfxPluginUID *uid)
{
    if (!session)
    {
        DISPATCHER_LOG_ERROR((("MFXVideoUSER_UnLoad: session=NULL\n")));
        return MFX_ERR_NULL_PTR;
    }
    MFX_DISP_HANDLE &rHandle = *(MFX_DISP_HANDLE *) session;
    if (!uid)
    {
        DISPATCHER_LOG_ERROR((("MFXVideoUSER_UnLoad: uid=NULL\n")));
        return MFX_ERR_NULL_PTR;
    }

    bool bDestroyed = rHandle.pluginFactory.Destroy(*uid);
    if (bDestroyed)
    {
        DISPATCHER_LOG_INFO((("MFXVideoUSER_UnLoad : plugin with GUID=" MFXGUIDTYPE()" unloaded\n"), MFXGUIDTOHEX(uid)));
    } else
    {
        DISPATCHER_LOG_ERROR((("MFXVideoUSER_UnLoad : plugin with GUID=" MFXGUIDTYPE()" not found\n"), MFXGUIDTOHEX(uid)));
    }

    return bDestroyed ? MFX_ERR_NONE : MFX_ERR_NOT_FOUND;
}

mfxStatus MFXAudioUSER_Load(mfxSession session, const mfxPluginUID *uid, mfxU32 version)
{
    if (!session)
    {
        DISPATCHER_LOG_ERROR((("MFXAudioUSER_Load: session=NULL\n")));
        return MFX_ERR_NULL_PTR;
    }
    MFX_DISP_HANDLE &pHandle = *(MFX_DISP_HANDLE *) session;
    if (!uid)
    {
        DISPATCHER_LOG_ERROR((("MFXAudioUSER_Load: uid=NULL\n")));
        return MFX_ERR_NULL_PTR;
    }
    DISPATCHER_LOG_INFO((("MFXAudioUSER_Load: uid=" MFXGUIDTYPE()" version=%d\n")
        , MFXGUIDTOHEX(uid)
        , version))
        size_t pluginsChecked = 0;
    PluginDescriptionRecord defaultPluginRecord;
    for (MFX::MFXPluginStorage::iterator i = pHandle.pluginHive.begin();i != pHandle.pluginHive.end(); i++, pluginsChecked++)
    {
        if (i->PluginUID != *uid)
        {
            if (i->Default) // PluginUID == 0 for default set
            {
                defaultPluginRecord = *i;
            }
            continue;
        }
        //check rest in records
        if (i->PluginVersion < version)
        {
            DISPATCHER_LOG_INFO((("MFXAudioUSER_Load: registered \"Plugin Version\" for GUID=" MFXGUIDTYPE()" is %d, that is smaller that requested\n")
                , MFXGUIDTOHEX(uid)
                , i->PluginVersion))
                continue;
        }
        try {
            return pHandle.pluginFactory.Create(*i);
        }
        catch(...) {
            return MFX_ERR_UNKNOWN;
        }
    }

    // Specified UID was not found among individually registed plugins, now try load it from default set if any
    if (defaultPluginRecord.Default)
    {
        defaultPluginRecord.PluginUID = *uid;
        defaultPluginRecord.onlyVersionRegistered = true;
        defaultPluginRecord.PluginVersion = (mfxU16)version;
        try {
            return pHandle.pluginFactory.Create(defaultPluginRecord);
        }
        catch(...) {
            return MFX_ERR_UNKNOWN;
        }
    }

    DISPATCHER_LOG_ERROR((("MFXAudioUSER_Load: cannot find registered plugin with requested UID, total plugins available=%d\n"), pHandle.pluginHive.size()));
    return MFX_ERR_NOT_FOUND;
}

mfxStatus MFXAudioUSER_UnLoad(mfxSession session, const mfxPluginUID *uid)
{
    if (!session)
    {
        DISPATCHER_LOG_ERROR((("MFXAudioUSER_UnLoad: session=NULL\n")));
        return MFX_ERR_NULL_PTR;
    }
    MFX_DISP_HANDLE &rHandle = *(MFX_DISP_HANDLE *) session;
    if (!uid)
    {
        DISPATCHER_LOG_ERROR((("MFXAudioUSER_Load: uid=NULL\n")));
        return MFX_ERR_NULL_PTR;
    }

    bool bDestroyed = rHandle.pluginFactory.Destroy(*uid);
    if (bDestroyed)
    {
        DISPATCHER_LOG_INFO((("MFXAudioUSER_UnLoad : plugin with GUID=" MFXGUIDTYPE()" unloaded\n"), MFXGUIDTOHEX(uid)));
    } else
    {
        DISPATCHER_LOG_ERROR((("MFXAudioUSER_UnLoad : plugin with GUID=" MFXGUIDTYPE()" not found\n"), MFXGUIDTOHEX(uid)));
    }

    return bDestroyed ? MFX_ERR_NONE : MFX_ERR_NOT_FOUND;
}
#else // relates to !defined (MEDIASDK_UWP_DISPATCHER), i.e. #else part as if MEDIASDK_UWP_DISPATCHER defined

static mfxModuleHandle hModule;

// for the UWP_DISPATCHER purposes implementation of MFXinitEx is calling
// InitialiseMediaSession() implemented in intel_gfx_api.dll
mfxStatus MFXInitEx(mfxInitParam par, mfxSession *session)
{
#if defined(MEDIASDK_ARM_LOADER)

    return MFX_ERR_UNSUPPORTED;

#else

    wchar_t IntelGFXAPIdllName[MFX_MAX_DLL_PATH] = { 0 };
    mfxI32 adapterNum = -1;

    switch (par.Implementation & 0xf)
    {
    case MFX_IMPL_SOFTWARE:
#if (MFX_VERSION >= MFX_VERSION_NEXT)
    case MFX_IMPL_SINGLE_THREAD:
#endif
        return MFX_ERR_UNSUPPORTED;
    case MFX_IMPL_AUTO:
    case MFX_IMPL_HARDWARE:
        adapterNum = 0;
        break;
    case MFX_IMPL_HARDWARE2:
        adapterNum = 1;
        break;
    case MFX_IMPL_HARDWARE3:
        adapterNum = 2;
        break;
    case MFX_IMPL_HARDWARE4:
        adapterNum = 3;
        break;
    default:
        return GfxApiInitPriorityIntegrated(par, session, hModule);
    }

    return GfxApiInitByAdapterNum(par, adapterNum, session, hModule);

#endif
}

// for the UWP_DISPATCHER purposes implementation of MFXClose is calling
// DisposeMediaSession() implemented in intel_gfx_api.dll
mfxStatus MFXClose(mfxSession session)
{
    if (NULL == session) {
        return MFX_ERR_INVALID_HANDLE;
    }

    mfxStatus sts = MFX_ERR_NONE;

#if defined(MEDIASDK_ARM_LOADER)

    sts = MFX_ERR_UNSUPPORTED;

#else

    sts = GfxApiClose(session, hModule);

#endif

    session = (mfxSession)NULL;
    return sts;
}

#undef FUNCTION
#define FUNCTION(return_value, func_name, formal_param_list, actual_param_list) \
    return_value func_name formal_param_list \
{ \
    mfxStatus mfxRes = MFX_ERR_INVALID_HANDLE; \
\
    _mfxSession *pHandle = (_mfxSession *) session; \
\
    /* get the function's address and make a call */ \
    if (pHandle) \
{ \
    mfxFunctionPointer pFunc = pHandle->callPlugInsTable[e##func_name]; \
    if (pFunc) \
{ \
    /* pass down the call */ \
    mfxRes = (*(mfxStatus (MFX_CDECL  *) formal_param_list) pFunc) actual_param_list; \
} \
} \
    return mfxRes; \
}

FUNCTION(mfxStatus, MFXVideoUSER_Load, (mfxSession session, const mfxPluginUID *uid, mfxU32 version), (session, uid, version))
FUNCTION(mfxStatus, MFXVideoUSER_LoadByPath, (mfxSession session, const mfxPluginUID *uid, mfxU32 version, const mfxChar *path, mfxU32 len), (session, uid, version, path, len))
FUNCTION(mfxStatus, MFXVideoUSER_UnLoad, (mfxSession session, const mfxPluginUID *uid), (session, uid))
FUNCTION(mfxStatus, MFXAudioUSER_Load, (mfxSession session, const mfxPluginUID *uid, mfxU32 version), (session, uid, version))
FUNCTION(mfxStatus, MFXAudioUSER_UnLoad, (mfxSession session, const mfxPluginUID *uid), (session, uid))

#endif //!defined(MEDIASDK_UWP_DISPATCHER)

mfxStatus MFXJoinSession(mfxSession session, mfxSession child_session)
{
    mfxStatus mfxRes = MFX_ERR_INVALID_HANDLE;
    MFX_DISP_HANDLE *pHandle = (MFX_DISP_HANDLE *)session;
    MFX_DISP_HANDLE *pChildHandle = (MFX_DISP_HANDLE *)child_session;

    // get the function's address and make a call
    if ((pHandle) && (pChildHandle) && (pHandle->actualApiVersion == pChildHandle->actualApiVersion))
    {
        /* check whether it is audio session or video */
        int tableIndex = eMFXJoinSession;
        mfxFunctionPointer pFunc;
        if (pHandle->impl & MFX_IMPL_AUDIO)
        {
            pFunc = pHandle->callAudioTable[tableIndex];
        }
        else
        {
            pFunc = pHandle->callTable[tableIndex];
        }

        if (pFunc)
        {
            // pass down the call
            mfxRes = (*(mfxStatus(MFX_CDECL *) (mfxSession, mfxSession)) pFunc) (pHandle->session,
                pChildHandle->session);
        }
    }

    return mfxRes;

} // mfxStatus MFXJoinSession(mfxSession session, mfxSession child_session)

mfxStatus MFXCloneSession(mfxSession session, mfxSession *clone)
{
    mfxStatus mfxRes = MFX_ERR_INVALID_HANDLE;
    MFX_DISP_HANDLE *pHandle = (MFX_DISP_HANDLE *)session;
    mfxVersion apiVersion;
    mfxIMPL impl;

    // check error(s)
    if (pHandle)
    {
        // initialize the clone session
        apiVersion = pHandle->apiVersion;
        impl = pHandle->impl | pHandle->implInterface;
        mfxRes = MFXInit(impl, &apiVersion, clone);
        if (MFX_ERR_NONE != mfxRes)
        {
            return mfxRes;
        }

        // join the sessions
        mfxRes = MFXJoinSession(session, *clone);
        if (MFX_ERR_NONE != mfxRes)
        {
            MFXClose(*clone);
            *clone = NULL;
            return mfxRes;
        }
    }

    return mfxRes;

} // mfxStatus MFXCloneSession(mfxSession session, mfxSession *clone)

mfxStatus MFXInit(mfxIMPL impl, mfxVersion *pVer, mfxSession *session)
{
    mfxInitParam par = {};

    par.Implementation = impl;
    if (pVer)
    {
        par.Version = *pVer;
    }
    else
    {
        par.Version.Major = DEFAULT_API_VERSION_MAJOR;
        par.Version.Minor = DEFAULT_API_VERSION_MINOR;
    }
    par.ExternalThreads = 0;

    return MFXInitEx(par, session);
}

//
//
// implement all other calling functions.
// They just call a procedure of DLL library from the table.
//

// define for common functions (from mfxsession.h)
#undef FUNCTION
#define FUNCTION(return_value, func_name, formal_param_list, actual_param_list) \
    return_value func_name formal_param_list \
{ \
    mfxStatus mfxRes = MFX_ERR_INVALID_HANDLE; \
     _mfxSession *pHandle = (_mfxSession *) session; \
    /* get the function's address and make a call */ \
    if (pHandle) \
{ \
    /* check whether it is audio session or video */ \
    int tableIndex = e##func_name; \
    mfxFunctionPointer pFunc; \
    if (pHandle->impl & MFX_IMPL_AUDIO) \
{ \
    pFunc = pHandle->callAudioTable[tableIndex]; \
} \
        else \
{ \
    pFunc = pHandle->callTable[tableIndex]; \
} \
    if (pFunc) \
{ \
    /* get the real session pointer */ \
    session = pHandle->session; \
    /* pass down the call */ \
    mfxRes = (*(mfxStatus (MFX_CDECL  *) formal_param_list) pFunc) actual_param_list; \
} \
} \
    return mfxRes; \
}

FUNCTION(mfxStatus, MFXQueryIMPL, (mfxSession session, mfxIMPL *impl), (session, impl))
FUNCTION(mfxStatus, MFXQueryVersion, (mfxSession session, mfxVersion *version), (session, version))

// these functions are not necessary in LOADER part of dispatcher and
// need to be included only in in SOLID dispatcher or PROCTABLE part of dispatcher

FUNCTION(mfxStatus, MFXDisjoinSession, (mfxSession session), (session))
FUNCTION(mfxStatus, MFXSetPriority, (mfxSession session, mfxPriority priority), (session, priority))
FUNCTION(mfxStatus, MFXGetPriority, (mfxSession session, mfxPriority *priority), (session, priority))

#undef FUNCTION
#define FUNCTION(return_value, func_name, formal_param_list, actual_param_list) \
    return_value func_name formal_param_list \
{ \
    mfxStatus mfxRes = MFX_ERR_INVALID_HANDLE; \
     _mfxSession *pHandle = (_mfxSession *) session;\
    /* get the function's address and make a call */ \
    if (pHandle) \
{ \
    mfxFunctionPointer pFunc = pHandle->callTable[e##func_name]; \
    if (pFunc) \
{ \
    /* get the real session pointer */ \
    session = pHandle->session; \
    /* pass down the call */ \
    mfxRes = (*(mfxStatus (MFX_CDECL  *) formal_param_list) pFunc) actual_param_list; \
} \
} \
    return mfxRes; \
}

#include "mfx_exposed_functions_list.h"
#undef FUNCTION
#define FUNCTION(return_value, func_name, formal_param_list, actual_param_list) \
    return_value func_name formal_param_list \
{ \
    mfxStatus mfxRes = MFX_ERR_INVALID_HANDLE; \
     _mfxSession *pHandle = (_mfxSession *) session; \
    /* get the function's address and make a call */ \
    if (pHandle) \
{ \
    mfxFunctionPointer pFunc = pHandle->callAudioTable[e##func_name]; \
    if (pFunc) \
{ \
    /* get the real session pointer */ \
    session = pHandle->session; \
    /* pass down the call */ \
    mfxRes = (*(mfxStatus (MFX_CDECL  *) formal_param_list) pFunc) actual_param_list; \
} \
} \
    return mfxRes; \
}

#include "mfxaudio_exposed_functions_list.h"
