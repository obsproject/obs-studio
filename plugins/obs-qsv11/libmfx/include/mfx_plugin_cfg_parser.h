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

#if !defined(__MFX_PLUGIN_CFG_PARSER_H)
#define __MFX_PLUGIN_CFG_PARSER_H

#include "mfx_dispatcher_defs.h"
#include "mfxplugin.h"
#include "mfx_vector.h"
#include "mfx_plugin_hive.h"
#include <string.h>
#include <memory>
#include <stdio.h>

#pragma once

namespace MFX
{
    class PluginConfigParser
    {
    public:

        enum 
        {
            PARSED_TYPE        = 1,
            PARSED_CODEC_ID    = 2,
            PARSED_UID         = 4,
            PARSED_PATH        = 8,
            PARSED_DEFAULT     = 16,
            PARSED_VERSION     = 32,
            PARSED_API_VERSION = 64,
            PARSED_NAME        = 128,
        };

        explicit PluginConfigParser(const char * name);
        ~PluginConfigParser();

        // Returns current section name if any
        bool GetCurrentPluginName(char * pluginName, int nChars);

        template <size_t N>
        bool GetCurrentPluginName(char (& pluginName)[N])
        {
            return this->GetCurrentPluginName(pluginName, N);
        }

        // Tries to advance to the next section in config file
        bool AdvanceToNextPlugin();
        // Return to first line of the file
        bool Rewind();
        // Enumerates sections in currect file (no section headers - 1 section)
        int GetPluginCount();
        // Parses plugin parameters from current section
        bool ParsePluginParams(PluginDescriptionRecord & dst, mfxU32 & parsedFields);

    private:
        FILE * cfgFile;
        fpos_t sectionStart;

        bool ParseSingleParameter(const char * name, char * value, PluginDescriptionRecord & dst, mfxU32 & parsedFields);
    };

    bool parseGUID(const char* src, mfxU8* guid);
}

#endif // __MFX_PLUGIN_CFG_PARSER_H