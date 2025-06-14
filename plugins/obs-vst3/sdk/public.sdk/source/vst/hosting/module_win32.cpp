//-----------------------------------------------------------------------------
// Project     : VST SDK
//
// Category    : Helpers
// Filename    : public.sdk/source/vst/hosting/module_win32.cpp
// Created by  : Steinberg, 08/2016
// Description : hosting module classes (win32 implementation)
//
//-----------------------------------------------------------------------------
// This file is part of a Steinberg SDK. It is subject to the license terms
// in the LICENSE file found in the top-level directory of this distribution
// and at www.steinberg.net/sdklicenses. 
// No part of the SDK, including this file, may be copied, modified, propagated,
// or distributed except according to the terms contained in the LICENSE file.
//-----------------------------------------------------------------------------

#include "module.h"
#include "public.sdk/source/vst/utility/optional.h"
#include "public.sdk/source/vst/utility/stringconvert.h"

#include "pluginterfaces/base/funknownimpl.h"

#include <shlobj.h>
#include <windows.h>

#include <algorithm>
#include <iostream>

#if SMTG_CPP17

#if __has_include(<filesystem>)
#define USE_FILESYSTEM 1
#elif __has_include(<experimental/filesystem>)
#define USE_FILESYSTEM 0
#endif

#else // !SMTG_CPP17

#define USE_FILESYSTEM 0

#endif // SMTG_CPP17

#if USE_FILESYSTEM == 1

#include <filesystem>
namespace filesystem = std::filesystem;

#else // USE_FILESYSTEM == 0

// The <experimental/filesystem> header is deprecated. It is superseded by the C++17 <filesystem>
// header. You can define _SILENCE_EXPERIMENTAL_FILESYSTEM_DEPRECATION_WARNING to silence the
// warning, otherwise the build will fail in VS2019 16.3.0
#define _SILENCE_EXPERIMENTAL_FILESYSTEM_DEPRECATION_WARNING
#include <experimental/filesystem>
namespace filesystem = std::experimental::filesystem;

#endif // USE_FILESYSTEM

#pragma comment(lib, "Shell32")

//------------------------------------------------------------------------
extern "C" {
using InitModuleFunc = bool (PLUGIN_API*) ();
using ExitModuleFunc = bool (PLUGIN_API*) ();
}

//------------------------------------------------------------------------
namespace VST3 {
namespace Hosting {

constexpr unsigned long kIPPathNameMax = 1024;

//------------------------------------------------------------------------
namespace {

#define USE_OLE !USE_FILESYSTEM

// for testing only
#if 0 // DEVELOPMENT
#define LOG_ENABLE 1
#else
#define LOG_ENABLE 0
#endif

#if SMTG_PLATFORM_64

#if SMTG_OS_WINDOWS_ARM

#if SMTG_CPU_ARM_64EC
constexpr auto architectureString = "arm64ec-win";
constexpr auto architectureX64String = "x86_64-win";
#else // !SMTG_CPU_ARM_64EC
constexpr auto architectureString = "arm64-win";
#endif // SMTG_CPU_ARM_64EC

constexpr auto architectureArm64XString = "arm64x-win";

#else // !SMTG_OS_WINDOWS_ARM
constexpr auto architectureString = "x86_64-win";
#endif // SMTG_OS_WINDOWS_ARM

#else // !SMTG_PLATFORM_64

#if SMTG_OS_WINDOWS_ARM
constexpr auto architectureString = "arm-win";
#else // !SMTG_OS_WINDOWS_ARM
constexpr auto architectureString = "x86-win";
#endif // SMTG_OS_WINDOWS_ARM

#endif // SMTG_PLATFORM_64

#if USE_OLE
//------------------------------------------------------------------------
struct Ole
{
	static Ole& instance ()
	{
		static Ole gInstance;
		return gInstance;
	}

private:
	Ole () { OleInitialize (nullptr); }
	~Ole () { OleUninitialize (); }
};
#endif // USE_OLE

//------------------------------------------------------------------------
class Win32Module : public Module
{
public:
	template <typename T>
	T getFunctionPointer (const char* name)
	{
		return reinterpret_cast<T> (GetProcAddress (mModule, name));
	}

	~Win32Module () override
	{
		factory = PluginFactory (nullptr);

		if (mModule)
		{
			// ExitDll is optional
			if (auto dllExit = getFunctionPointer<ExitModuleFunc> ("ExitDll"))
				dllExit ();

			FreeLibrary ((HMODULE)mModule);
		}
	}

	//--- -----------------------------------------------------------------------
	HINSTANCE loadAsPackage (const std::string& inPath, std::string& errorDescription,
	                         const char* archString = architectureString)
	{
		namespace StringConvert = Steinberg::Vst::StringConvert;

		filesystem::path p (inPath);

		auto filename = p.filename ();
		p /= "Contents";
		p /= archString;
		p /= filename;
		const std::wstring wString = p.generic_wstring ();
		HINSTANCE instance = LoadLibraryW (reinterpret_cast<LPCWSTR> (wString.data ()));
#if SMTG_CPU_ARM_64EC
		if (instance == nullptr)
			instance = loadAsPackage (inPath, errorDescription, architectureArm64XString);
		if (instance == nullptr)
			instance = loadAsPackage (inPath, errorDescription, architectureX64String);
#endif // SMTG_CPU_ARM_64EC
		if (instance == nullptr)
			getLastError (p.string (), errorDescription);
		return instance;
	}

	//--- -----------------------------------------------------------------------
	HINSTANCE loadAsDll (const std::string& inPath, std::string& errorDescription)
	{
		namespace StringConvert = Steinberg::Vst::StringConvert;

		auto wideStr = StringConvert::convert (inPath);
		HINSTANCE instance = LoadLibraryW (reinterpret_cast<LPCWSTR> (wideStr.data ()));
		if (instance == nullptr)
		{
			getLastError (inPath, errorDescription);
		}
		else
		{
			hasBundleStructure = false;
		}
		return instance;
	}

	//--- -----------------------------------------------------------------------
	bool load (const std::string& inPath, std::string& errorDescription) override
	{
		// filesystem::u8path is deprecated in C++20
#if SMTG_CPP20
		const filesystem::path tmp (inPath);
#else
		const filesystem::path tmp = filesystem::u8path (inPath);
#endif // SMTG_CPP20
		std::error_code ec;
		if (filesystem::is_directory (tmp, ec))
		{
			// try as package (bundle)
			mModule = loadAsPackage (inPath, errorDescription);
		}
		else
		{
			// try old definition without package
			mModule = loadAsDll (inPath, errorDescription);
		}
		if (mModule == nullptr)
			return false;

		auto factoryProc = getFunctionPointer<GetFactoryProc> ("GetPluginFactory");
		if (!factoryProc)
		{
			errorDescription = "The dll does not export the required 'GetPluginFactory' function";
			return false;
		}
		// InitDll is optional
		auto dllEntry = getFunctionPointer<InitModuleFunc> ("InitDll");
		if (dllEntry && !dllEntry ())
		{
			errorDescription = "Calling 'InitDll' failed";
			return false;
		}
		auto f = Steinberg::U::cast<Steinberg::IPluginFactory> (owned (factoryProc ()));
		if (!f)
		{
			errorDescription = "Calling 'GetPluginFactory' returned nullptr";
			return false;
		}
		factory = PluginFactory (f);
		return true;
	}

	HINSTANCE mModule {nullptr};

private:
	//--- -----------------------------------------------------------------------
	void getLastError (const std::string& inPath, std::string& errorDescription)
	{
		auto lastError = GetLastError ();
		LPVOID lpMessageBuffer {nullptr};
		if (FormatMessageA (FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM, nullptr,
		                    lastError, MAKELANGID (LANG_NEUTRAL, SUBLANG_DEFAULT),
		                    (LPSTR)&lpMessageBuffer, 0, nullptr) > 0)
		{
			errorDescription = "LoadLibraryW failed for path " + inPath + ": " +
			                   std::string ((char*)lpMessageBuffer);
			LocalFree (lpMessageBuffer);
		}
		else
		{
			errorDescription = "LoadLibraryW failed with error number: " +
			                   std::to_string (lastError) + " for path " + inPath;
		}
	}
};

//------------------------------------------------------------------------
bool openVST3Package (const filesystem::path& p, const char* archString,
                      filesystem::path* result = nullptr)
{
	auto path = p;
	path /= "Contents";
	path /= archString;
	path /= p.filename ();
	const std::wstring wString = path.generic_wstring ();
	auto hFile = CreateFileW (reinterpret_cast<LPCWSTR> (wString.data ()), GENERIC_READ,
	                          FILE_SHARE_READ, nullptr, OPEN_EXISTING, 0, nullptr);
	if (hFile != INVALID_HANDLE_VALUE)
	{
		CloseHandle (hFile);
		if (result)
			*result = path;
		return true;
	}
	return false;
}

//------------------------------------------------------------------------
bool checkVST3Package (const filesystem::path& p, filesystem::path* result = nullptr,
                       const char* archString = architectureString)
{
	if (openVST3Package (p, archString, result))
		return true;

#if SMTG_CPU_ARM_64EC
	if (openVST3Package (p, architectureArm64XString, result))
		return true;
	if (openVST3Package (p, architectureX64String, result))
		return true;
#endif // SMTG_CPU_ARM_64EC
	return false;
}

//------------------------------------------------------------------------
bool isFolderSymbolicLink (const filesystem::path& p)
{
#if USE_FILESYSTEM
	std::error_code ec;
	if (filesystem::is_symlink (p, ec))
		return true;
#else
	const std::wstring wString = p.generic_wstring ();
	auto attrib = GetFileAttributesW (reinterpret_cast<LPCWSTR> (wString.data ()));
	if (attrib & FILE_ATTRIBUTE_REPARSE_POINT)
	{
		auto hFile = CreateFileW (reinterpret_cast<LPCWSTR> (wString.data ()), GENERIC_READ,
		                          FILE_SHARE_READ, nullptr, OPEN_EXISTING, 0, nullptr);
		if (hFile == INVALID_HANDLE_VALUE)
			return true;
		CloseHandle (hFile);
	}
#endif // USE_FILESYSTEM
	return false;
}

//------------------------------------------------------------------------
Optional<std::string> getKnownFolder (REFKNOWNFOLDERID folderID)
{
	namespace StringConvert = Steinberg::Vst::StringConvert;

	PWSTR wideStr {};
	if (FAILED (SHGetKnownFolderPath (folderID, 0, nullptr, &wideStr)))
		return {};
	return StringConvert::convert (Steinberg::wscast (wideStr));
}

//------------------------------------------------------------------------
VST3::Optional<filesystem::path> resolveShellLink (const filesystem::path& p)
{
#if USE_FILESYSTEM
	std::error_code ec;
	auto target = filesystem::read_symlink (p, ec);
	if (ec)
		return {};
	else
		return { target.lexically_normal () };
#elif USE_OLE
	Ole::instance ();

	IShellLink* shellLink = nullptr;
	if (!SUCCEEDED (CoCreateInstance (CLSID_ShellLink, nullptr, CLSCTX_INPROC_SERVER,
	                                  IID_IShellLink, reinterpret_cast<LPVOID*> (&shellLink))))
		return {};

	IPersistFile* persistFile = nullptr;
	if (!SUCCEEDED (
	        shellLink->QueryInterface (IID_IPersistFile, reinterpret_cast<void**> (&persistFile))))
		return {};

	if (!SUCCEEDED (persistFile->Load (p.wstring ().data (), STGM_READ)))
		return {};

	if (!SUCCEEDED (shellLink->Resolve (nullptr, MAKELONG (SLR_NO_UI, 500))))
		return {};

	WCHAR resolvedPath[kIPPathNameMax];
	if (!SUCCEEDED (shellLink->GetPath (resolvedPath, kIPPathNameMax, nullptr, SLGP_SHORTPATH)))
		return {};

	std::wstring longPath;
	longPath.resize (kIPPathNameMax);
	auto numChars =
	    GetLongPathNameW (resolvedPath, const_cast<wchar_t*> (longPath.data ()), kIPPathNameMax);
	if (!numChars)
		return {};
	longPath.resize (numChars);

	persistFile->Release ();
	shellLink->Release ();

	return {filesystem::path (longPath)};
#else
	return {};
#endif // USE_FILESYSTEM
}

//------------------------------------------------------------------------
void addToPathList (Module::PathList& pathList, const std::string& toAdd)
{
#if LOG_ENABLE
	std::cout << "=> add: " << toAdd << "\n";
#endif

	pathList.push_back (toAdd);
}

//------------------------------------------------------------------------
void findFilesWithExt (const filesystem::path& path, const std::string& ext,
                       Module::PathList& pathList, bool recursive = true)
{
	for (auto& p : filesystem::directory_iterator (path))
	{
#if USE_FILESYSTEM
		filesystem::path finalPath (p);
		if (isFolderSymbolicLink (p))
		{
			if (auto res = resolveShellLink (p))
			{
				finalPath = *res;
				std::error_code ec;
				if (!filesystem::exists (finalPath, ec))
					continue;
			}
			else
				continue;
		}
		const auto& cpExt = finalPath.extension ();
		if (cpExt == ext)
		{
			filesystem::path result;
			if (checkVST3Package (finalPath, &result))
			{
#if SMTG_CPP20
				std::u8string u8str = result.generic_u8string ();
				std::string str;
				str.assign (std::begin (u8str), std::end (u8str));
				addToPathList (pathList, str);
#else
				addToPathList (pathList, result.generic_u8string ());
#endif // SMTG_CPP20
				continue;
			}
		}
		std::error_code ec;
		if (filesystem::is_directory (finalPath, ec))
		{
			if (recursive)
				findFilesWithExt (finalPath, ext, pathList, recursive);
		}
		else if (cpExt == ext)
		{
#if SMTG_CPP20
			std::u8string u8str = finalPath.generic_u8string ();
			std::string str;
			str.assign (std::begin (u8str), std::end (u8str));
			addToPathList (pathList, str);
#else
			addToPathList (pathList, finalPath.generic_u8string ());
#endif // SMTG_CPP20
		}
#else // !USE_FILESYSTEM
		const auto& cp = p.path ();
		const auto& cpExt = cp.extension ();
		if (cpExt == ext)
		{
			if ((p.status ().type () == filesystem::file_type::directory) ||
			    isFolderSymbolicLink (p))
			{
				filesystem::path result;
				if (checkVST3Package (p, &result))
				{
					addToPathList (pathList, result.generic_u8string ());
					continue;
				}
				findFilesWithExt (cp, ext, pathList, recursive);
			}
			else
				addToPathList (pathList, cp.generic_u8string ());
		}
		else if (recursive)
		{
			if (p.status ().type () == filesystem::file_type::directory)
			{
				findFilesWithExt (cp, ext, pathList, recursive);
			}
			else if (cpExt == ".lnk")
			{
				if (auto resolvedLink = resolveShellLink (cp))
				{
					if (resolvedLink->extension () == ext)
					{
						if (filesystem::is_directory (*resolvedLink) ||
						    isFolderSymbolicLink (*resolvedLink))
						{
							filesystem::path result;
							if (checkVST3Package (*resolvedLink, &result))
							{
								addToPathList (pathList, result.generic_u8string ());
								continue;
							}
							findFilesWithExt (*resolvedLink, ext, pathList, recursive);
						}
						else
							addToPathList (pathList, resolvedLink->generic_u8string ());
					}
					else if (filesystem::is_directory (*resolvedLink))
					{
						const auto& str = resolvedLink->generic_u8string ();
						if (cp.generic_u8string ().compare (0, str.size (), str.data (),
						                                    str.size ()) != 0)
							findFilesWithExt (*resolvedLink, ext, pathList, recursive);
					}
				}
			}
		}
#endif // USE_FILESYSTEM
	}
}

//------------------------------------------------------------------------
void findModules (const filesystem::path& path, Module::PathList& pathList)
{
	std::error_code ec;
	if (filesystem::exists (path, ec))
		findFilesWithExt (path, ".vst3", pathList);
}

//------------------------------------------------------------------------
Optional<filesystem::path> getContentsDirectoryFromModuleExecutablePath (
    const std::string& modulePath)
{
	// filesystem::u8path is deprecated in C++20
#if SMTG_CPP20
	filesystem::path path (modulePath);
#else
	filesystem::path path = filesystem::u8path (modulePath);
#endif // SMTG_CPP20

	path = path.parent_path ();
	if (path.filename () != architectureString)
		return {};
	path = path.parent_path ();
	if (path.filename () != "Contents")
		return {};

	return Optional<filesystem::path> {std::move (path)};
}

//------------------------------------------------------------------------
} // anonymous

//------------------------------------------------------------------------
Module::Ptr Module::create (const std::string& path, std::string& errorDescription)
{
	auto _module = std::make_shared<Win32Module> ();
	if (_module->load (path, errorDescription))
	{
		_module->path = path;
		auto it = std::find_if (path.rbegin (), path.rend (),
		                        [] (const std::string::value_type& c) { return c == '/'; });
		if (it != path.rend ())
			_module->name = {it.base (), path.end ()};
		return _module;
	}
	return nullptr;
}

//------------------------------------------------------------------------
Module::PathList Module::getModulePaths ()
{
	namespace StringConvert = Steinberg::Vst::StringConvert;

	// find plug-ins located in common/VST3
	PathList list;
	if (auto knownFolder = getKnownFolder (FOLDERID_UserProgramFilesCommon))
	{
		// filesystem::u8path is deprecated in C++20
#if SMTG_CPP20
		filesystem::path path (*knownFolder);
#else
		filesystem::path path = filesystem::u8path (*knownFolder);
#endif // SMTG_CPP20
		path.append ("VST3");
#if LOG_ENABLE
		std::cout << "Check folder: " << path << "\n";
#endif
		findModules (path, list);
	}

	if (auto knownFolder = getKnownFolder (FOLDERID_ProgramFilesCommon))
	{
		// filesystem::u8path is deprecated in C++20
#if SMTG_CPP20
		filesystem::path path (*knownFolder);
#else
		filesystem::path path = filesystem::u8path (*knownFolder);
#endif // SMTG_CPP20
		path.append ("VST3");
#if LOG_ENABLE
		std::cout << "Check folder: " << path << "\n";
#endif
		findModules (path, list);
	}

	// find plug-ins located in VST3 (application folder)
	WCHAR modulePath[kIPPathNameMax];
	GetModuleFileNameW (nullptr, modulePath, kIPPathNameMax);
	auto appPath = StringConvert::convert (Steinberg::wscast (modulePath));

	// filesystem::u8path is deprecated in C++20
#if SMTG_CPP20
	filesystem::path path (appPath);
#else
	filesystem::path path = filesystem::u8path (appPath);
#endif // SMTG_CPP20
	path = path.parent_path ();
	path = path.append ("VST3");
#if LOG_ENABLE
	std::cout << "Check folder: " << path << "\n";
#endif
	findModules (path, list);

	return list;
}

//------------------------------------------------------------------------
Optional<std::string> Module::getModuleInfoPath (const std::string& modulePath)
{
	auto path = getContentsDirectoryFromModuleExecutablePath (modulePath);
	if (!path)
	{
		filesystem::path p;
		if (!checkVST3Package ({modulePath}, &p))
			return {};
		p = p.parent_path ();
		p = p.parent_path ();
		path = Optional<filesystem::path> {p};
	}

	*path /= "Resources";
	*path /= "moduleinfo.json";

	std::error_code ec;
	if (filesystem::exists (*path, ec))
	{
		return {path->generic_string ()};
	}
	return {};
}

//------------------------------------------------------------------------
bool Module::validateBundleStructure (const std::string& modulePath, std::string& errorDescription)
{
	try
	{
		auto path = getContentsDirectoryFromModuleExecutablePath (modulePath);
		if (!path)
		{
			filesystem::path p;
			if (!checkVST3Package ({modulePath}, &p))
			{
				errorDescription = "Not a bundle: '" + modulePath + "'.";
				return false;
			}
			p = p.parent_path ();
			p = p.parent_path ();
			path = Optional<filesystem::path> {p};
		}
		if (path->filename () != "Contents")
		{
			errorDescription = "Unexpected directory name, should be 'Contents' but is '" +
			                   path->filename ().string () + "'.";
			return false;
		}
		auto bundlePath = path->parent_path ();
		*path /= architectureString;
		*path /= bundlePath.filename ();
		std::error_code ec;
		if (filesystem::exists (*path, ec) == false)
		{
			errorDescription = "Shared library name is not equal to bundle folder name. Must be '" +
			                   bundlePath.filename ().string () + "'.";
			return false;
		}
		return true;
	}
	catch (const std::exception& exc)
	{
		errorDescription = exc.what ();
		return false;
	}
}

//------------------------------------------------------------------------
Module::SnapshotList Module::getSnapshots (const std::string& modulePath)
{
	SnapshotList result;
	auto path = getContentsDirectoryFromModuleExecutablePath (modulePath);
	if (!path)
	{
		filesystem::path p;
		if (!checkVST3Package ({modulePath}, &p))
			return result;
		p = p.parent_path ();
		p = p.parent_path ();
		path = Optional<filesystem::path> (p);
	}

	*path /= "Resources";
	*path /= "Snapshots";

	std::error_code ec;
	if (filesystem::exists (*path, ec) == false)
		return result;

	PathList pngList;
	findFilesWithExt (*path, ".png", pngList, false);
	for (auto& png : pngList)
	{
		// filesystem::u8path is deprecated in C++20
#if SMTG_CPP20
		const filesystem::path p (png);
#else
		const filesystem::path p = filesystem::u8path (png);
#endif // SMTG_CPP20
		auto filename = p.filename ().generic_string ();
		auto uid = Snapshot::decodeUID (filename);
		if (!uid)
			continue;
		auto scaleFactor = 1.;
		if (auto decodedScaleFactor = Snapshot::decodeScaleFactor (filename))
			scaleFactor = *decodedScaleFactor;

		Module::Snapshot::ImageDesc desc;
		desc.scaleFactor = scaleFactor;
		desc.path = std::move (png);
		bool found = false;
		for (auto& entry : result)
		{
			if (entry.uid != *uid)
				continue;
			found = true;
			entry.images.emplace_back (std::move (desc));
			break;
		}
		if (found)
			continue;
		Module::Snapshot snapshot;
		snapshot.uid = *uid;
		snapshot.images.emplace_back (std::move (desc));
		result.emplace_back (std::move (snapshot));
	}
	return result;
}

//------------------------------------------------------------------------
} // Hosting
} // VST3
