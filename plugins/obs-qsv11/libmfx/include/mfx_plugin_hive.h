// Copyright (c) 2013-2019 Intel Corporation
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

#include "mfx_dispatcher_defs.h"
#include "mfxplugin.h"
#include "mfx_win_reg_key.h"
#include "mfx_vector.h"
#include <string.h>
#include <memory>
#include <stdio.h>

struct MFX_DISP_HANDLE;

namespace MFX {

    inline bool operator == (const mfxPluginUID &lhs, const mfxPluginUID & rhs) 
    {
        return !memcmp(lhs.Data, rhs.Data, sizeof(mfxPluginUID));
    }
    
    inline bool operator != (const mfxPluginUID &lhs, const mfxPluginUID & rhs) 
    {
        return !(lhs == rhs);
    }
#ifdef _WIN32
    //warning C4351: new behavior: elements of array 'MFX::PluginDescriptionRecord::sName' will be default initialized
    #pragma warning (disable: 4351)
#endif
    class PluginDescriptionRecord :  public mfxPluginParam 
    {
    public:
        msdk_disp_char sPath[MAX_PLUGIN_PATH];
        char sName[MAX_PLUGIN_NAME];
        //used for FS plugins that has poor description
        bool onlyVersionRegistered;
        bool Default;
        PluginDescriptionRecord()
            : mfxPluginParam()
            , sPath()
            , sName()
            , onlyVersionRegistered()
            , Default()
        {
        }
    };

    typedef MFXVector<PluginDescriptionRecord> MFXPluginStorage;

    class  MFXPluginStorageBase : public MFXPluginStorage 
    {
    protected:
        mfxVersion mCurrentAPIVersion;
    protected:
        MFXPluginStorageBase(mfxVersion currentAPIVersion) 
            : mCurrentAPIVersion(currentAPIVersion)
        {
        }
        void ConvertAPIVersion( mfxU32 APIVersion, PluginDescriptionRecord &descriptionRecord) const
        {
            descriptionRecord.APIVersion.Minor = static_cast<mfxU16> (APIVersion & 0x0ff);
            descriptionRecord.APIVersion.Major = static_cast<mfxU16> (APIVersion >> 8);
        }
    };

    //populated from registry
    class MFXPluginsInHive : public MFXPluginStorageBase
    {
    public:
        MFXPluginsInHive(int mfxStorageID, const msdk_disp_char *msdkLibSubKey, mfxVersion currentAPIVersion);
    };

#if defined(MEDIASDK_USE_CFGFILES) || (!defined(MEDIASDK_UWP_LOADER) && !defined(MEDIASDK_UWP_PROCTABLE))
    //plugins are loaded from FS close to executable
    class MFXPluginsInFS : public MFXPluginStorageBase
    {
        bool mIsVersionParsed;
        bool mIsAPIVersionParsed;
    public:
        MFXPluginsInFS(mfxVersion currentAPIVersion);
    private:
        bool ParseFile(FILE * f, PluginDescriptionRecord & des);
        bool ParseKVPair( msdk_disp_char *key, msdk_disp_char * value, PluginDescriptionRecord & des);
    };
#endif //#if defined(MEDIASDK_USE_CFGFILES) || (!defined(MEDIASDK_UWP_LOADER) && !defined(MEDIASDK_UWP_PROCTABLE))

    //plugins are loaded from FS close to Runtime library
    class MFXDefaultPlugins : public MFXPluginStorageBase
    {
    public:
#if !defined(OPEN_SOURCE)
#if !defined(MEDIASDK_DFP_LOADER)
        MFXDefaultPlugins(mfxVersion currentAPIVersion, MFX_DISP_HANDLE * hdl, int implType);
#else
        MFXDefaultPlugins(mfxVersion currentAPIVersion, int implType);
#endif
#else
        MFXDefaultPlugins(mfxVersion currentAPIVersion, MFX_DISP_HANDLE * hdl, int implType);
#endif
    private:
    };

}
