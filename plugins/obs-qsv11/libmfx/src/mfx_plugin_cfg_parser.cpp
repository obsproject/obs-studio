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

#if !defined(_WIN32) && !defined(_WIN64)

#include "mfx_plugin_cfg_parser.h"
#include "mfx_dispatcher_log.h"
#include <stdlib.h>
#include <ctype.h>

const int guidLen = 16;

// In-place strip trailing whitespace chars
static char* Strip(char* s)
{
    char* p = s + strlen(s);
    while (p > s && isspace(*--p))
    {
        *p = 0;
    }
    return s;
}

// Search for the first non-whitespace char
static char* SkipWhitespace(char* s)
{
    while (*s && isspace(*s))
    {
        s++;
    }
    return s;
}

// Return pointer to first char c or ';' comment in given string, or pointer to
// null at end of string if neither found. ';' must be prefixed by a whitespace
// character to register as a comment.
static char* FindCharOrComment(char* s, char c)
{
    int whitespaceFound = 0;
    while (*s && *s != c && !(whitespaceFound && *s == ';'))
    {
        whitespaceFound = isspace(*s);
        s++;
    }
    return s;
}

// Version of strncpy that ensures dest (size bytes) is null-terminated.
static char* strncpy0(char* dest, const char* src, size_t size)
{
    strncpy(dest, src, size);
    dest[size - 1] = 0;
    return dest;
}

enum
{
    MAX_SECTION = 4096
};

namespace MFX
{

bool parseGUID(const char* src, mfxU8* guid)
{
    mfxU32 p[guidLen];
    int res = sscanf(src, 
        "%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X",
        p, p + 1, p + 2, p + 3, p + 4, p + 5, p + 6, p + 7, 
        p + 8, p + 9, p + 10, p + 11, p + 12, p + 13, p + 14, p + 15);
    if (res != guidLen)
        return false;

    for (int i = 0; i < guidLen; i++)
        guid[i] = (mfxU8)p[i];
    return true;
}

PluginConfigParser::PluginConfigParser(const char * name)
    : sectionStart()
{
    cfgFile = fopen(name, "rt");
}

PluginConfigParser::~PluginConfigParser()
{
    if (cfgFile)
    {
        fclose(cfgFile);
    }
}

// Returns current section name if any
bool PluginConfigParser::GetCurrentPluginName(char * pluginName, int nChars)
{
    if (!cfgFile)
        return false;

    char line[MAX_PLUGIN_NAME];
    char section[MAX_SECTION] = "";
    bool foundSection = false;
    char * start;
    char * end;

    if (fgets(line, MAX_PLUGIN_NAME, cfgFile))
    {
        start = SkipWhitespace(Strip(line));
        if (*start == '[')
        {
            // A "[section]" line
            end = FindCharOrComment(start + 1, ']');
            if (*end == ']') {
                *end = '\0';
                strncpy0(pluginName, start + 1, nChars);
                foundSection = true;
            }
        }
    }
    fsetpos(cfgFile, &sectionStart);

    return foundSection;
}

// Tries to advance to the next section in config file
bool PluginConfigParser::AdvanceToNextPlugin()
{
    if (!cfgFile)
        return false;

    char line[MAX_PLUGIN_NAME];
    char section[MAX_SECTION] = "";
    bool foundSection = false;
    char * start;
    char * end;

    fgetpos(cfgFile, &sectionStart);
    // advance one line from current section
    if (!fgets(line, MAX_PLUGIN_NAME, cfgFile))
        return false;
    
    fpos_t lastReadLine = sectionStart;
    while (!foundSection && fgets(line, MAX_PLUGIN_NAME, cfgFile))
    {
        start = SkipWhitespace(Strip(line));

        if (*start == '[')
        {
            // A "[section]" line            
            end = FindCharOrComment(start + 1, ']');
            if (*end == ']') {
                foundSection = true;
                sectionStart = lastReadLine;
            }
        }
        fgetpos(cfgFile, &lastReadLine);
    }

    fsetpos(cfgFile, &sectionStart);
    return foundSection;
}

// Return to first ini file section
bool PluginConfigParser::Rewind()
{
    if (!cfgFile)
        return false;

    fseek(cfgFile, 0, SEEK_SET);
    fgetpos(cfgFile, &sectionStart);

    return true;
}

// Enumerates sections in currect file
int PluginConfigParser::GetPluginCount()
{
    if (!cfgFile)
        return -1;

    Rewind();
    
    int counter = 0;

    do 
    { 
        counter++;
    } while (AdvanceToNextPlugin());

    // special case - plugin.cfg without section header
    if (counter == 0)
    {
        int size = fseek(cfgFile, 0, SEEK_END);
        if (size > 0)
            counter = 1;
    }

    Rewind();

    return counter;
}


bool PluginConfigParser::ParseSingleParameter(const char * name, char * value, PluginDescriptionRecord & dst, mfxU32 & parsedFields)
{
    if (0 == strcmp(name, "Type"))
    {
        dst.Type = atoi(value);
        parsedFields |= PARSED_TYPE;
        return true;
    }
    if (0 == strcmp(name, "CodecID"))
    {
        const int fourccLen = 4;
        if (strlen(value) == 0 || strlen(value) > fourccLen)
            return false;

        dst.CodecId = MFX_MAKEFOURCC(' ',' ',' ',' ');
        char *codecID = reinterpret_cast<char*>(&dst.CodecId);
        for (size_t i = 0; i < strlen(value); i++)
            codecID[i] = value[i];

        parsedFields |= PARSED_CODEC_ID;
        return true;
    }
    if (0 == strcmp(name, "GUID"))
    {
        if (!parseGUID(value, dst.PluginUID.Data))
            return false;

        parsedFields |= PARSED_UID;
        return true;
    }
    if (0 == strcmp(name, "Path") ||
#ifdef LINUX64        
        0 == strcmp(name, "FileName64"))
#else
        0 == strcmp(name, "FileName32"))
#endif
    {
        // strip quotes
        const int lastCharIndex = strlen(value) - 1;
        if (value[0] == '"' && value[lastCharIndex] == '"')
        {
            value[lastCharIndex] = '\0';
            value = value + 1;
        }
        if (strlen(dst.sPath) + strlen("/") + strlen(value) >= MAX_PLUGIN_PATH)
            return false;
        strcpy(dst.sPath + strlen(dst.sPath), "/");
        strcpy(dst.sPath + strlen(dst.sPath), value);
        parsedFields |= PARSED_PATH;
        return true;
    }
    if (0 == strcmp(name, "Default"))
    {
        dst.Default = (0 != atoi(value));
        parsedFields |= PARSED_DEFAULT;
        return true;
    }
    if (0 == strcmp(name, "PluginVersion"))
    {
        dst.PluginVersion = atoi(value);        
        parsedFields |= PARSED_VERSION;
        return true;
    }
    if (0 == strcmp(name, "APIVersion"))
    {
        mfxU32 APIVersion = atoi(value);
        dst.APIVersion.Minor = static_cast<mfxU16> (APIVersion & 0xff);
        dst.APIVersion.Major = static_cast<mfxU16> (APIVersion >> 8);
        parsedFields |= PARSED_API_VERSION;
        return true;
    }

    return false;
}

bool PluginConfigParser::ParsePluginParams(PluginDescriptionRecord & dst, mfxU32 & parsedFields)
{
    if (!cfgFile)
        return false;

    char line[MAX_PLUGIN_NAME];

    char* start;
    char* end;
    char* name;
    char* value;
    bool error = false;
    
    int parsedHeaders = 0;
    fgetpos(cfgFile, &sectionStart);

    // Scan through file line by line 
    while (fgets(line, MAX_PLUGIN_NAME, cfgFile))
    {
        start = SkipWhitespace(Strip(line));

        if (*start == ';' || *start == '#')
        {
            // Allow '#' and ';' comments at start of line
        }
        else if (*start == '[')
        {
            if (++parsedHeaders == 1)
            {
                // no interest in section header here
                continue;
            }
            else
            {
                // we found next header
                break;
            }
        }
        else if (*start && *start != ';')
        {
            // do not allow header in the middle of plugin description
            parsedHeaders = 1;
            // Not a comment, must be a name[=:]value pair
            end = FindCharOrComment(start, '=');
            if (*end != '=')
            {
                end = FindCharOrComment(start, ':');
            }
            if (*end == '=' || *end == ':')
            {
                *end = 0;
                name = Strip(start);
                value = SkipWhitespace(end + 1);
                end = FindCharOrComment(value, 0);
                if (*end == ';')
                {
                    *end = 0;
                }
                Strip(value);

                // Valid name[=:]value pair found, call handler
                ParseSingleParameter(name, value, dst, parsedFields);
            }
            else if (!error) 
            {
                // No '=' or ':' found on name[=:]value line
                error = true;
            }
        }
        // Store section start for next iteration
        // fgetpos(cfgFile, &sectionStart);
    }

    // restore previous position in file
    fsetpos(cfgFile, &sectionStart);

    return !error && (parsedFields != 0);
}

} // namespace MFX

#endif // !defined(_WIN32) && !defined(_WIN64)
