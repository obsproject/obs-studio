/* SPDX-License-Identifier: MIT */
/**
    @file		linux/infoimpl.cpp
    @brief		Implements the AJASystemInfoImpl class on the Linux platform.
	@copyright	(C) 2009-2021 AJA Video Systems, Inc.  All rights reserved.
**/

#include "ajabase/system/file_io.h"
#include "ajabase/system/info.h"
#include "ajabase/system/linux/infoimpl.h"
#include "ajabase/system/system.h"

#include <cstdlib>
#include <stdexcept>
#include <unistd.h>

std::string aja_cmd(const char* cmd)
{
    const int maxbufsize = 256;
    char buffer[maxbufsize];
    std::string result = "";
    FILE* pipe = popen(cmd, "r");
    if (!pipe)
    {
        throw std::runtime_error("popen() failed!");
    }

    try
    {
        while (!feof(pipe))
        {
            if (fgets(buffer, maxbufsize, pipe) != NULL)
            {
                try
                {
                    result += buffer;
                }
                catch (...)
                {
                    pclose(pipe);
                    throw;
                }
            }
        }
    }
    catch (...)
    {
        pclose(pipe);
        throw;
    }
    pclose(pipe);
    return result;
}

std::string aja_procfs(const char* procfs_file, const char* value_key)
{
    // cat /proc/cpuinfo outputs something like:
    // vendor_id     : GenuineIntel
    // model name    : Intel(R) Xeon(R) CPU       X5650  @ 2.67GHz
    // ...
    // so use grep to get the lines matching key
    // use head to only select the first line returned
    // use cut to get just the part after ':'
    // use xargs to remove leading and trailing spaces
    // use tr to remove linefeeds
    // use tr to remove repeated spaces
    std::ostringstream oss;
    oss << "cat /proc/" << procfs_file << " | grep '" << value_key << "' | head -n 1 | cut -d ':' -f 2 | xargs | tr -d '\n' | tr -s ' '";

    return aja_cmd(oss.str().c_str());
}

std::string aja_uptime()
{
    // Centos6 / Redhat 6 does not have the uptime -s command so
    // first try using /proc/uptime

    std::string out;
    std::ostringstream oss;
    oss << "date -d \"`cut -f1 -d. /proc/uptime` seconds ago\" \"+%Y-%m-%d %H:%M:%S\"";
    out = aja_cmd(oss.str().c_str());
    out = aja::strip(out);

    if (out.empty())
    {
        out = aja_cmd("uptime -s 2>/dev/null");
        out = aja::strip(out);
    }

    return out;
}

std::string aja_productname()
{
    std::string out;
    out = aja_cmd("lsb_release -d -s 2>/dev/null");
    out = aja::strip(out);
    out = aja::strip(out, "\"");

    if (out.empty())
    {
        AJAFileIO f;

        if (f.FileExists("/etc/redhat-release"))
        {
            out = aja_cmd("cat /etc/redhat-release 2>/dev/null");
        }
        else if(f.FileExists("/etc/os-release"))
        {
            out = aja_cmd("cat /etc/os-release 2>/dev/null | grep 'PRETTY_NAME' | head -n 1 | cut -d '=' -f 2 | tr -d '\"' | tr -d '\n'");
        }
    }

    out = aja::strip(out);
    return out;
}

std::string aja_osversion()
{
    std::string out;
    out = aja_cmd("lsb_release -r -s 2>/dev/null");

    if (out.empty())
    {
        AJAFileIO f;

        if(f.FileExists("/etc/os-release"))
        {
            out = aja_cmd("cat /etc/os-release 2>/dev/null | grep 'VERSION_ID' | head -n 1 | cut -d '=' -f 2 | tr -d '\"' | tr -d '\n'");
        }
    }

    out = aja::strip(out);
    return out;
}

void get_vendor_and_device(std::map<std::string, std::string>& inDevicePart,
                           std::vector<std::string>& outFoundDevices)
{
    if (inDevicePart.size() >= 2)
    {
        std::string vend;
        std::string device;

        if (inDevicePart.find("SVendor") != inDevicePart.end())
            vend = inDevicePart.at("SVendor");
        else if (inDevicePart.find("Vendor") != inDevicePart.end())
            vend = inDevicePart.at("Vendor");

        if (inDevicePart.find("SDevice") != inDevicePart.end())
            device = inDevicePart.at("SDevice");
        else if (inDevicePart.find("Device") != inDevicePart.end())
            device = inDevicePart.at("Device");

        outFoundDevices.push_back(vend + " " + device);
    }
}

std::string aja_getgputype()
{
    std::string out;
    out = aja_cmd("lspci -vmm | grep VGA -A 4");

    // The following are some real world lspci results the code has been tested against and a few made up samples

    // CentOS 7 running under VirtualBox
    //out = "Class:\tVGA compatible controller\nVendor:\tInnoTek Systemberatung GmbH\nDevice:\tVirtualBox Graphics Adapter\n\nSlot:\t00:03.0";
    // Ubuntu 18.04 with NVIDIA Quadro K600
    //out = "Class:\tVGA compatible controller\nVendor:\tNVIDIA Corporation\nDevice:\tGK107GL [Quadro K600]\nSVendor:\tNVIDIA Corporation\nSDevice:\tGK107GL [Quadro K600]";
    // Ubuntu 18.04 with 2x NVIDIA Quadro K600
    //out = "Class:\tVGA compatible controller\nVendor:\tNVIDIA Corporation\nDevice:\tGK107GL [Quadro K600]\nSVendor:\tNVIDIA Corporation\nSDevice:\tGK107GL [Quadro K600]\n--\nClass:\tVGA compatible controller\nVendor:\tNVIDIA Corporation\nDevice:\tGK107GL [Quadro K600]\nSVendor:\tNVIDIA Corporation\nSDevice:\tGK107GL [Quadro K600]";
    // CentOS 7 running as a VM (not sure which hypervisor)
    //out = "Class:\tVGA compatible controller\nVendor:\tVendor 1234\nDevice:\t Device 1111\nSVendor:\tRed Hat, Inc.\nSDevice:\tDevice 1100";
    // If no VGA found or lspci fails
    //out = "";
    // VGA found but no vendor or device keys
    //out = "Class:\tVGA compatible controller\nPhySlot:\t1\nRev:\t02";
    // VGA found but a value contains colons
    //out = "Class:\tVGA compatible controller\nVendor:\tFake vendor with : colon in name, twice : end\nDevice:\tGeneric:VGA";

    std::ostringstream oss;
    std::vector<std::string> lines = aja::split(out, '\n');
    if (lines.empty() == false)
    {
        std::vector<std::string>::iterator it;
        std::vector<std::string> foundDevices;
        std::map<std::string, std::string> deviceParts;
        for(it=lines.begin();it!=lines.end();++it)
        {
            if (*it == "--" || *it == "")
            {
                if (deviceParts.size() >= 2)
                {
                    get_vendor_and_device(deviceParts, foundDevices);
                    deviceParts.clear();
                }
                continue;
            }

            std::vector<std::string> cols = aja::split(*it, ':');
            if (cols.size() > 1)
            {
                std::string key = aja::strip(cols.at(0));
                std::string val = cols.at(1);
                // handle any values that contain colons by concating back together
                for(size_t i=2;i<cols.size();++i)
                {
                    val = val + ":" + cols.at(i);
                }
                val = aja::strip(val);

                if (key == "Class")
                {
                    deviceParts.clear();
                    continue;
                }
                else if (key == "Vendor" || key == "Device" ||
                         key == "SVendor" || key == "SDevice")
                {
                    deviceParts[key] = val;
                }
            }
        }

        if (deviceParts.size() >= 2)
        {
            get_vendor_and_device(deviceParts, foundDevices);
            deviceParts.clear();
        }

        for(size_t i=0;i<foundDevices.size();++i)
        {
            if (i != 0)
            {
                oss << ", ";
            }
            oss << foundDevices.at(i);
        }
    }
    return oss.str();
}

AJASystemInfoImpl::AJASystemInfoImpl(int units)
{
    mMemoryUnits = units;
}

AJASystemInfoImpl::~AJASystemInfoImpl()
{

}

AJAStatus
AJASystemInfoImpl::Rescan(AJASystemInfoSections sections)
{
    AJAStatus ret = AJA_STATUS_FAIL;

    static char tmp_buf[4096];

    if (sections & AJA_SystemInfoSection_System)
    {
        mValueMap[int(AJA_SystemInfoTag_System_Model)] = aja_cmd("uname -m | tr -d '\n'");
        gethostname(tmp_buf, sizeof(tmp_buf));
        mValueMap[int(AJA_SystemInfoTag_System_Name)] = tmp_buf;
        mValueMap[int(AJA_SystemInfoTag_System_BootTime)] = aja_uptime();

        ret = AJA_STATUS_SUCCESS;
    }

    if (sections & AJA_SystemInfoSection_OS)
    {
        mValueMap[int(AJA_SystemInfoTag_OS_ProductName)] = aja_productname();
        mValueMap[int(AJA_SystemInfoTag_OS_Version)] = aja_osversion();
        mValueMap[int(AJA_SystemInfoTag_OS_VersionBuild)] = aja_cmd("uname -v | tr -d '\n'");
        mValueMap[int(AJA_SystemInfoTag_OS_KernelVersion)] = aja_cmd("uname -r | tr -d '\n'");

        ret = AJA_STATUS_SUCCESS;
    }

    if (sections & AJA_SystemInfoSection_CPU)
    {
        mValueMap[int(AJA_SystemInfoTag_CPU_Type)] = aja_procfs("cpuinfo", "model name");
        long int numProcs = sysconf(_SC_NPROCESSORS_ONLN);
        std::ostringstream num_cores;
        num_cores << numProcs;
        mValueMap[int(AJA_SystemInfoTag_CPU_NumCores)] = num_cores.str();

        ret = AJA_STATUS_SUCCESS;
    }

    if (sections & AJA_SystemInfoSection_Mem)
    {
        std::string memTotalStr = aja_procfs("meminfo", "MemTotal");
        int64_t memtotalbytes=0;
        if (memTotalStr.find(" kB") != std::string::npos)
        {
            // convert from kilobytes to bytes
            aja::replace(memTotalStr, " kB", "");
            std::istringstream(memTotalStr) >> memtotalbytes;
            memtotalbytes *= 1024;
        }
        else
        {
            // assume it is in bytes?
            std::istringstream(memTotalStr) >> memtotalbytes;
        }

        std::string memFreeStr = aja_procfs("meminfo", "MemFree");
        int64_t memfreebytes=0;
        if (memFreeStr.find(" kB") != std::string::npos)
        {
            // convert from kilobytes to bytes
            aja::replace(memFreeStr, " kB", "");
            std::istringstream(memFreeStr) >> memfreebytes;
            memfreebytes *= 1024;
        }
        else
        {
            // assume it is in bytes?
            std::istringstream(memFreeStr) >> memfreebytes;
        }

        std::string unitsLabel;
        double divisor = 1.0;
        switch(mMemoryUnits)
        {
            default:
            case AJA_SystemInfoMemoryUnit_Bytes:
                unitsLabel = "B";
                break;
            case AJA_SystemInfoMemoryUnit_Kilobytes:
                unitsLabel = "KB";
                divisor = 1024.0;
                break;
            case AJA_SystemInfoMemoryUnit_Megabytes:
                unitsLabel = "MB";
                divisor = 1048576.0;
                break;
            case AJA_SystemInfoMemoryUnit_Gigabytes:
                unitsLabel = "GB";
                divisor = 1073741824.0;
                break;
        }

        int64_t memusedbytes = memtotalbytes - memfreebytes;

        std::ostringstream t,u,f;
        t << int64_t(memtotalbytes / divisor) << " " << unitsLabel;
        u << int64_t(memusedbytes / divisor) << " " << unitsLabel;
        f << int64_t(memfreebytes / divisor) << " " << unitsLabel;

        mValueMap[int(AJA_SystemInfoTag_Mem_Total)] = t.str();
        mValueMap[int(AJA_SystemInfoTag_Mem_Used)] = u.str();
        mValueMap[int(AJA_SystemInfoTag_Mem_Free)] = f.str();

        ret = AJA_STATUS_SUCCESS;
    } // end if (sections & AJA_SystemInfoSection_Mem)

    if (sections & AJA_SystemInfoSection_GPU)
    {
        mValueMap[int(AJA_SystemInfoTag_GPU_Type)] = aja_getgputype();

        ret = AJA_STATUS_SUCCESS;
    }

    if (sections & AJA_SystemInfoSection_Path)
    {
        const char* homePath = getenv("HOME");
        if (homePath != NULL)
        {
            mValueMap[int(AJA_SystemInfoTag_Path_UserHome)] = homePath;
            mValueMap[int(AJA_SystemInfoTag_Path_PersistenceStoreUser)] = homePath;
            mValueMap[int(AJA_SystemInfoTag_Path_PersistenceStoreUser)].append("/.aja/config/");
        }

        mValueMap[int(AJA_SystemInfoTag_Path_PersistenceStoreSystem)] = "/opt/aja/config/";

        mValueMap[int(AJA_SystemInfoTag_Path_Applications)] = "/opt/aja/bin/";
        mValueMap[int(AJA_SystemInfoTag_Path_Utilities)] = "/opt/aja/bin/";
        mValueMap[int(AJA_SystemInfoTag_Path_Firmware)] = "/opt/aja/firmware/";

        ret = AJA_STATUS_SUCCESS;
    }

    return ret;
}
