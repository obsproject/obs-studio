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


#if !defined(_WIN32) && !defined(_WIN64)

#include <sys/types.h>
#include <sys/stat.h>
#include <dirent.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <mfx_library_iterator.h>

#include <mfx_dispatcher.h>
#include <mfx_dispatcher_log.h>

#define MFX_PCI_DIR "/sys/bus/pci/devices"
#define MFX_PCI_DISPLAY_CONTROLLER_CLASS 0x03

#ifndef __APPLE__
#if defined(LINUX64)
    static const char mfx_so_hw_base_name[] = "libmfxhw64-p.so";
    static const char mfx_so_sw_base_name[] = "libmfxsw64-p.so";
#else
    static const char mfx_so_hw_base_name[] = "libmfxhw32-p.so";
    static const char mfx_so_sw_base_name[] = "libmfxsw32-p.so";
#endif

#else
#if defined(X86_64)
static const char mfx_so_hw_base_name[] = "libmfxhw64.dylib";
static const char mfx_so_sw_base_name[] = "libmfxsw64.dylib";
#else
static const char mfx_so_hw_base_name[] = "libmfxhw32.dylib";
static const char mfx_so_sw_base_name[] = "libmfxsw32.dylib";
#endif
#endif  //ifndef __APPLE__

static int mfx_dir_filter(const struct dirent* dir_ent)
{
    if (!dir_ent) return 0;
    if (!strcmp(dir_ent->d_name, ".")) return 0;
    if (!strcmp(dir_ent->d_name, "..")) return 0;
    return 1;
}

typedef int (*fsort)(const struct dirent**, const struct dirent**);

static mfxU32 mfx_init_adapters(struct mfx_disp_adapters** p_adapters)
{
    mfxU32 adapters_num = 0;
    int i = 0;
    struct mfx_disp_adapters* adapters = NULL;
    struct dirent** dir_entries = NULL;
    int entries_num = scandir(MFX_PCI_DIR, &dir_entries, mfx_dir_filter, (fsort)alphasort);

    // sizeof(MFX_PCI_DIR) = 20, sizeof(dirent::d_name) <= 256, sizeof("class"|"vendor"|"device") = 6
    char file_name[300] = {};
    // sizeof("0xzzzzzz") = 8
    char str[16] = {0};
    FILE* file = NULL;

    for (i = 0; i < entries_num; ++i)
    {
        long int class_id = 0, vendor_id = 0, device_id = 0;

        if (!dir_entries[i]) continue;
        // obtaining device class id
        snprintf(file_name, sizeof(file_name)/sizeof(file_name[0]), "%s/%s/%s", MFX_PCI_DIR, dir_entries[i]->d_name, "class");
        file = fopen(file_name, "r");
        if (file)
        {
            if (fgets(str, sizeof(str), file))
            {
                class_id = strtol(str, NULL, 16);
            }
            fclose(file);

            if (MFX_PCI_DISPLAY_CONTROLLER_CLASS == (class_id >> 16))
            {
                // obtaining device vendor id
                snprintf(file_name, sizeof(file_name)/sizeof(file_name[0]), "%s/%s/%s", MFX_PCI_DIR, dir_entries[i]->d_name, "vendor");
                file = fopen(file_name, "r");
                if (file)
                {
                    if (fgets(str, sizeof(str), file))
                    {
                        vendor_id = strtol(str, NULL, 16);
                    }
                    fclose(file);
                }
                // obtaining device id
                snprintf(file_name, sizeof(file_name)/sizeof(file_name[0]), "%s/%s/%s", MFX_PCI_DIR, dir_entries[i]->d_name, "device");
                file = fopen(file_name, "r");
                if (file)
                {
                    if (fgets(str, sizeof(str), file))
                    {
                        device_id = strtol(str, NULL, 16);
                    }
                    fclose(file);
                }
                // adding valid adaptor to the list
                if (vendor_id && device_id)
                {
                    struct mfx_disp_adapters* tmp_adapters = NULL;

                    tmp_adapters = (mfx_disp_adapters*)realloc(adapters,
                                                               (adapters_num+1)*sizeof(struct mfx_disp_adapters));

                    if (tmp_adapters)
                    {
                        adapters = tmp_adapters;
                        adapters[adapters_num].vendor_id = vendor_id;
                        adapters[adapters_num].device_id = device_id;
                        ++adapters_num;
                    }
                }
            }
        }
        free(dir_entries[i]);
    }
    if (entries_num) free(dir_entries);
    if (p_adapters) *p_adapters = adapters;
    return adapters_num;
}

static mfxU32 mfx_list_libraries(const char* path, bool search_hw, struct mfx_libs** p_libs)
{
    mfxU32 libs_num = 0;
    size_t len = 0;
    int i = 0;
    struct mfx_libs* libs = NULL;
    struct dirent** dir_entries = NULL;
    int entries_num = scandir(path, &dir_entries, mfx_dir_filter, (fsort)alphasort);

    for (i = 0; i < entries_num; ++i)
    {
        unsigned long int major = 0, minor = 0;
        char* p_major = NULL;
        char* p_minor = NULL;
        char* p = NULL;
        bool b_skip = false;

        if (!dir_entries[i]) continue;

        len = strlen(dir_entries[i]->d_name);
        if (len < MFX_MIN_REAL_LIBNAME) goto skip;
        if (len > MFX_MAX_REAL_LIBNAME) goto skip;

        if (search_hw)
        {
            if (strncmp(dir_entries[i]->d_name, mfx_so_hw_base_name, MFX_SO_BASE_NAME_LEN)) goto skip;
        }
        else
        {
            if (strncmp(dir_entries[i]->d_name, mfx_so_sw_base_name, MFX_SO_BASE_NAME_LEN)) goto skip;
        }

        for (p = &(dir_entries[i]->d_name[MFX_SO_BASE_NAME_LEN]); !b_skip && *p; ++p)
        {
            if ('.' == *p)
            {
                if (!p_major) p_major = p;
                else if (!p_minor) p_minor = p;
                else b_skip = true;
            }
            else if (!strchr("0123456789", *p))
            {
                b_skip = true;
            }
        }
        if (b_skip) goto skip;

        if (!p_major || !p_minor) goto skip;
        if (p_major != &(dir_entries[i]->d_name[MFX_SO_BASE_NAME_LEN])) goto skip;
        ++p_major;
        if (p_major == p_minor) goto skip;
        ++p_minor;
        if (!(*p_minor)) goto skip;
        
        major = strtoul(p_major, NULL, 10);
        minor = strtoul(p_minor, NULL, 10);
        if ((major <= 0xFFFF) && (minor <= 0xFFFF))
        {
            struct mfx_libs* tmp_libs = NULL;
            tmp_libs = (mfx_libs*)realloc(libs,
                                          (libs_num+1)*sizeof(struct mfx_libs));
            if (tmp_libs)
            {
                libs = tmp_libs;
                strncpy(libs[libs_num].name, dir_entries[i]->d_name, MFX_MAX_REAL_LIBNAME);
                libs[libs_num].name[MFX_MAX_REAL_LIBNAME] = 0;
                libs[libs_num].version.Major = (mfxU16)major;
                libs[libs_num].version.Minor = (mfxU16)minor;
                ++libs_num;
            }
        }

    skip:
        free(dir_entries[i]);
    }
    if (entries_num) free(dir_entries);
    if (p_libs) *p_libs = libs;
    return libs_num;
}

namespace MFX
{

mfxStatus SelectImplementationType(const mfxU32 adapterNum, mfxIMPL *pImplInterface, mfxU32 *pVendorID, mfxU32 *pDeviceID)
{
    mfx_disp_adapters* adapters = NULL;
    unsigned int adapters_num = mfx_init_adapters(&adapters);
    if (pVendorID && pDeviceID && adapterNum < adapters_num)
    {
        *pVendorID = adapters[adapterNum].vendor_id;
        *pDeviceID = adapters[adapterNum].device_id;
    }
    if (adapters_num) free(adapters);

    if (adapterNum >= adapters_num)
        return MFX_ERR_UNSUPPORTED;

    if ((*pImplInterface != MFX_IMPL_VIA_ANY) &&
        (*pImplInterface != MFX_IMPL_VIA_VAAPI) )
        return MFX_ERR_UNSUPPORTED;

    *pImplInterface = MFX_IMPL_VIA_VAAPI;
    return MFX_ERR_NONE;
}

MFXLibraryIterator::MFXLibraryIterator(void)
    : m_implType(MFX_LIB_PSEUDO)
    , m_implInterface(MFX_IMPL_UNSUPPORTED)
    , m_vendorID(0)
    , m_deviceID(0)
    , m_bIsSubKeyValid(false)
    , m_StorageID(0)
    , m_lastLibIndex(-1)
    , m_adapters(NULL)
    , m_selected_adapter(0)
    , m_libs_num(0)
    , m_libs(NULL)
{
    m_SubKeyName[0] = 0;
    m_path[0] = 0;

    m_adapters_num = mfx_init_adapters(&m_adapters);
}

MFXLibraryIterator::~MFXLibraryIterator(void)
{
    Release();
    if (m_adapters_num) free(m_adapters);
}

void MFXLibraryIterator::Release(void)
{
    m_implType = MFX_LIB_PSEUDO;

    m_vendorID = 0;
    m_deviceID = 0;

    m_lastLibIndex = -1;
    if (m_libs)
    {
        free(m_libs);
        m_libs = NULL;
    }
    m_libs_num = 0;
}

mfxStatus MFXLibraryIterator::Init(eMfxImplType implType, mfxIMPL impl, const mfxU32 adapter_num, int storageID)
{
    // release the object before initialization
    Release();

    // check error(s)
    if (MFX_LIB_HARDWARE == implType)
    {
        if (!m_adapters_num || (adapter_num >= m_adapters_num))
        {
            return MFX_ERR_UNSUPPORTED;
        }
        m_selected_adapter = adapter_num;
        m_vendorID = m_adapters[m_selected_adapter].vendor_id;
        m_deviceID = m_adapters[m_selected_adapter].device_id;
    }
    else if (MFX_LIB_SOFTWARE != implType)
    {
        return MFX_ERR_UNSUPPORTED;
    }
    if (MFX_STORAGE_ID_OPT != storageID)
    {
        return MFX_ERR_UNSUPPORTED;
    }

    // set the required library's implementation type
    m_implType = implType;

    snprintf(m_path, sizeof(m_path)/sizeof(m_path[0]),
             "%s", MFX_MODULES_DIR);

    m_libs_num = mfx_list_libraries(m_path, (MFX_LIB_HARDWARE == implType), &m_libs);
    
    if (!m_libs_num)
    {
        Release();
        return MFX_ERR_UNSUPPORTED;
    }

    return MFX_ERR_NONE;
}

mfxStatus MFXLibraryIterator::SelectDLLVersion(char *pPath, size_t pathSize,
                                               eMfxImplType* pImplType, mfxVersion minVersion)
{
    if (m_lastLibIndex < 0)
    {
        for (int i = m_libs_num - 1; i >= 0; i--)
        {
            if (m_libs[i].version.Major == minVersion.Major && m_libs[i].version.Minor >= minVersion.Minor)
            {
                    m_lastLibIndex = i;
                    break;
            }
        }
    }
    else
        m_lastLibIndex--;

    if (m_lastLibIndex < 0)
        return MFX_ERR_NOT_FOUND;
    
    if (m_libs[m_lastLibIndex].version.Major != minVersion.Major ||
        m_libs[m_lastLibIndex].version.Minor < minVersion.Minor)
    {
        m_lastLibIndex = -1;
        return MFX_ERR_NOT_FOUND;
    }

    snprintf(pPath, pathSize, "%s/%s", m_path, m_libs[m_lastLibIndex].name);

    if (pImplType) *pImplType = (!m_vendorID && !m_deviceID)? MFX_LIB_SOFTWARE: MFX_LIB_HARDWARE;

    return MFX_ERR_NONE;
}

mfxIMPL MFXLibraryIterator::GetImplementationType()
{
    if (m_selected_adapter < 0 || static_cast<unsigned int>(m_selected_adapter) >= m_adapters_num)
        return MFX_ERR_UNSUPPORTED;

    return MFX_IMPL_VIA_VAAPI;
}

bool MFXLibraryIterator::GetSubKeyName(msdk_disp_char *subKeyName, size_t length) const
{
    return false;
}

} // namespace MFX

#endif // #if !defined(_WIN32) && !defined(_WIN64)
