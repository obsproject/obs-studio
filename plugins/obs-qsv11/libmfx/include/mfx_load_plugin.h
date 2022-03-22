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
#include "mfxplugin.h"
#include "mfx_dispatcher_defs.h"
#include "mfx_plugin_hive.h"

namespace MFX
{
    typedef mfxStatus (MFX_CDECL *CreatePluginPtr_t)(mfxPluginUID uid, mfxPlugin* plugin);

    class PluginModule
    {
        mfxModuleHandle mHmodule;
        CreatePluginPtr_t mCreatePluginPtr;
        wchar_t mPath[MAX_PLUGIN_PATH];
        
    public:
        PluginModule();
        PluginModule(const wchar_t * path);
        PluginModule(const PluginModule & that) ;
        PluginModule & operator = (const PluginModule & that);
        bool Create(mfxPluginUID guid, mfxPlugin&);
        ~PluginModule(void);

    private:
        void Tidy();
    };

    class MFXPluginFactory {
        struct FactoryRecord {
            mfxPluginParam plgParams;
            PluginModule module;
            mfxPlugin plugin;
            FactoryRecord ()
                : plgParams(), plugin()
            {}
            FactoryRecord(const mfxPluginParam &plgParams,
                          PluginModule &module,
                          mfxPlugin plugin) 
                : plgParams(plgParams) 
                , module(module)
                , plugin(plugin) {
            }
        };
        MFXVector<FactoryRecord> mPlugins;
        mfxU32 nPlugins;
        mfxSession mSession;
    public:
        MFXPluginFactory(mfxSession session);
        void Close();
        mfxStatus Create(const PluginDescriptionRecord &);
        bool Destroy(const mfxPluginUID &);
        
        ~MFXPluginFactory();
    protected:
        void DestroyPlugin( FactoryRecord & );
        static bool RunVerification( const mfxPlugin & plg, const PluginDescriptionRecord &dsc, mfxPluginParam &pluginParams );
        static bool VerifyEncoder( const mfxVideoCodecPlugin &videoCodec );
        static bool VerifyAudioEncoder( const mfxAudioCodecPlugin &audioCodec );
        static bool VerifyEnc( const mfxVideoCodecPlugin &videoEnc );
        static bool VerifyVpp( const mfxVideoCodecPlugin &videoCodec );
        static bool VerifyDecoder( const mfxVideoCodecPlugin &videoCodec );
        static bool VerifyAudioDecoder( const mfxAudioCodecPlugin &audioCodec );
        static bool VerifyCodecCommon( const mfxVideoCodecPlugin & Video );
    };
}
