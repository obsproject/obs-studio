//-----------------------------------------------------------------------------
// Project     : VST SDK
//
// Category    : Helpers
// Filename    : public.sdk/source/vst/hosting/module_mac.mm
// Created by  : Steinberg, 08/2016
// Description : hosting module classes (macOS implementation)
//
//-----------------------------------------------------------------------------
// This file is part of a Steinberg SDK. It is subject to the license terms
// in the LICENSE file found in the top-level directory of this distribution
// and at www.steinberg.net/sdklicenses.
// No part of the SDK, including this file, may be copied, modified, propagated,
// or distributed except according to the terms contained in the LICENSE file.
//-----------------------------------------------------------------------------
#import "module.h"

#import <Cocoa/Cocoa.h>
#import <CoreFoundation/CoreFoundation.h>

#if !__has_feature(objc_arc)
#error this file needs to be compiled with automatic reference counting enabled
#endif

//------------------------------------------------------------------------
extern "C" {
typedef bool (*BundleEntryFunc) (CFBundleRef);
typedef bool (*BundleExitFunc) ();
}

//------------------------------------------------------------------------
namespace VST3 {
namespace Hosting {

//------------------------------------------------------------------------
namespace {

//------------------------------------------------------------------------
template <typename T>
class CFPtr
{
public:
	inline CFPtr (const T& obj = nullptr) : obj (obj) {}
	inline CFPtr (CFPtr&& other) { *this = other; }
	inline ~CFPtr ()
	{
		if (obj)
			CFRelease (obj);
	}

	inline CFPtr& operator= (CFPtr&& other)
	{
		obj = other.obj;
		other.obj = nullptr;
		return *this;
	}
	inline CFPtr& operator= (const T& o)
	{
		if (obj)
			CFRelease (obj);
		obj = o;
		return *this;
	}
	inline operator T () const { return obj; } // act as T
private:
	CFPtr (const CFPtr& other) = delete;
	CFPtr& operator= (const CFPtr& other) = delete;

	T obj = nullptr;
};

//------------------------------------------------------------------------
class MacModule : public Module
{
public:
	template <typename T>
	T getFunctionPointer (const char* name)
	{
		assert (bundle);
		CFPtr<CFStringRef> functionName (
		    CFStringCreateWithCString (kCFAllocatorDefault, name, kCFStringEncodingASCII));
		return reinterpret_cast<T> (CFBundleGetFunctionPointerForName (bundle, functionName));
	}

	bool loadInternal (const std::string& path, std::string& errorDescription)
	{
		CFPtr<CFURLRef> url (CFURLCreateFromFileSystemRepresentation (
		    kCFAllocatorDefault, reinterpret_cast<const UInt8*> (path.data ()), path.length (),
		    true));
		if (!url)
			return false;
		bundle = CFBundleCreate (kCFAllocatorDefault, url);
		CFErrorRef error = nullptr;
		if (!bundle || !CFBundleLoadExecutableAndReturnError (bundle, &error))
		{
			if (error)
			{
				CFPtr<CFStringRef> errorString (CFErrorCopyDescription (error));
				if (errorString)
				{
					auto stringLength = CFStringGetLength (errorString);
					auto maxSize =
					    CFStringGetMaximumSizeForEncoding (stringLength, kCFStringEncodingUTF8);
					auto buffer = std::make_unique<char[]> (maxSize);
					if (CFStringGetCString (errorString, buffer.get (), maxSize,
					                        kCFStringEncodingUTF8))
						errorDescription = buffer.get ();
					CFRelease (error);
				}
			}
			else
			{
				errorDescription = "Could not create Bundle for path: " + path;
			}
			return false;
		}
		// bundleEntry is mandatory
		auto bundleEntry = getFunctionPointer<BundleEntryFunc> ("bundleEntry");
		if (!bundleEntry)
		{
			errorDescription = "Bundle does not export the required 'bundleEntry' function";
			return false;
		}
		// bundleExit is mandatory
		auto bundleExit = getFunctionPointer<BundleExitFunc> ("bundleExit");
		if (!bundleExit)
		{
			errorDescription = "Bundle does not export the required 'bundleExit' function";
			return false;
		}
		auto factoryProc = getFunctionPointer<GetFactoryProc> ("GetPluginFactory");
		if (!factoryProc)
		{
			errorDescription = "Bundle does not export the required 'GetPluginFactory' function";
			return false;
		}
		if (!bundleEntry (bundle))
		{
			errorDescription = "Calling 'bundleEntry' failed";
			return false;
		}
		auto f = owned (factoryProc ());
		if (!f)
		{
			errorDescription = "Calling 'GetPluginFactory' returned nullptr";
			return false;
		}
		factory = PluginFactory (f);
		return true;
	}

	bool load (const std::string& path, std::string& errorDescription) override
	{
		if (!path.empty () && path[0] != '/')
		{
			auto buffer = std::make_unique<char[]> (PATH_MAX);
			auto workDir = getcwd (buffer.get (), PATH_MAX);
			if (workDir)
			{
				std::string wd (workDir);
				wd += "/";
				if (loadInternal (wd + path, errorDescription))
				{
					name = path;
					return true;
				}
				return false;
			}
		}
		return loadInternal (path, errorDescription);
	}

	~MacModule () override
	{
		factory = PluginFactory (nullptr);

		if (bundle)
		{
			if (auto bundleExit = getFunctionPointer<BundleExitFunc> ("bundleExit"))
				bundleExit ();
		}
	}

	CFPtr<CFBundleRef> bundle;
};

//------------------------------------------------------------------------
void findModulesInDirectory (NSURL* dirUrl, Module::PathList& result)
{
	dirUrl = [dirUrl URLByResolvingSymlinksInPath];
	if (!dirUrl)
		return;
	NSDirectoryEnumerator* enumerator = [[NSFileManager defaultManager]
	               enumeratorAtURL: dirUrl
	    includingPropertiesForKeys:nil
	                       options:NSDirectoryEnumerationSkipsPackageDescendants
	                  errorHandler:nil];
	for (NSURL* url in enumerator)
	{
		if ([[[url lastPathComponent] pathExtension] isEqualToString:@"vst3"])
		{
			CFPtr<CFArrayRef> archs (
			    CFBundleCopyExecutableArchitecturesForURL (static_cast<CFURLRef> (url)));
			if (archs)
				result.emplace_back ([url.path UTF8String]);
		}
		else
		{
			id resValue;
			if (![url getResourceValue:&resValue forKey:NSURLIsSymbolicLinkKey error:nil])
				continue;
			if (!static_cast<NSNumber*> (resValue).boolValue)
				continue;
			auto resolvedUrl = [url URLByResolvingSymlinksInPath];
			if (![resolvedUrl getResourceValue:&resValue forKey:NSURLIsDirectoryKey error:nil])
				continue;
			if (!static_cast<NSNumber*> (resValue).boolValue)
				continue;
			findModulesInDirectory (resolvedUrl, result);
		}
	}
}

//------------------------------------------------------------------------
void getModules (NSSearchPathDomainMask domain, Module::PathList& result)
{
	NSURL* libraryUrl = [[NSFileManager defaultManager] URLForDirectory:NSLibraryDirectory
	                                                           inDomain:domain
	                                                  appropriateForURL:nil
	                                                             create:NO
	                                                              error:nil];
	if (libraryUrl == nil)
		return;
	NSURL* audioUrl = [libraryUrl URLByAppendingPathComponent:@"Audio"];
	if (audioUrl == nil)
		return;
	NSURL* plugInsUrl = [audioUrl URLByAppendingPathComponent:@"Plug-Ins"];
	if (plugInsUrl == nil)
		return;
	NSURL* vst3Url =
	    [[plugInsUrl URLByAppendingPathComponent:@"VST3"] URLByResolvingSymlinksInPath];
	if (vst3Url == nil)
		return;
	findModulesInDirectory (vst3Url, result);
}

//------------------------------------------------------------------------
void getApplicationModules (Module::PathList& result)
{
	auto bundle = CFBundleGetMainBundle ();
	if (!bundle)
		return;
	auto bundleUrl = static_cast<NSURL*> (CFBridgingRelease (CFBundleCopyBundleURL (bundle)));
	if (!bundleUrl)
		return;
	auto resUrl = [bundleUrl URLByAppendingPathComponent:@"Contents"];
	if (!resUrl)
		return;
	auto vst3Url = [resUrl URLByAppendingPathComponent:@"VST3"];
	if (!vst3Url)
		return;
	findModulesInDirectory (vst3Url, result);
}

//------------------------------------------------------------------------
void getModuleSnapshots (const std::string& path, Module::SnapshotList& result)
{
	auto* nsString = [NSString stringWithUTF8String:path.data ()];
	if (!nsString)
		return;
	auto bundleUrl = [NSURL fileURLWithPath:nsString];
	if (!bundleUrl)
		return;
	auto urls = [NSBundle URLsForResourcesWithExtension:@"png"
	                                       subdirectory:@"Snapshots"
	                                    inBundleWithURL:bundleUrl];
	if (!urls || [urls count] == 0)
		return;

	for (NSURL* url in urls)
	{
		std::string fullpath ([[url path] UTF8String]);
		std::string filename ([[[url path] lastPathComponent] UTF8String]);
		auto uid = Module::Snapshot::decodeUID (filename);
		if (!uid)
			continue;

		auto scaleFactor = 1.;
		if (auto decodedScaleFactor = Module::Snapshot::decodeScaleFactor (filename))
			scaleFactor = *decodedScaleFactor;

		Module::Snapshot::ImageDesc desc;
		desc.scaleFactor = scaleFactor;
		desc.path = std::move (fullpath);
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
}

//------------------------------------------------------------------------
} // anonymous

//------------------------------------------------------------------------
Module::Ptr Module::create (const std::string& path, std::string& errorDescription)
{
	auto module = std::make_shared<MacModule> ();
	if (module->load (path, errorDescription))
	{
		module->path = path;
		auto it = std::find_if (path.rbegin (), path.rend (),
		                        [] (const std::string::value_type& c) { return c == '/'; });
		if (it != path.rend ())
			module->name = {it.base (), path.end ()};
		return std::move (module);
	}
	return nullptr;
}

//------------------------------------------------------------------------
Module::PathList Module::getModulePaths ()
{
	PathList list;
	getModules (NSUserDomainMask, list);
	getModules (NSLocalDomainMask, list);
	// TODO getModules (NSNetworkDomainMask, list);
	getApplicationModules (list);
	return list;
}

//------------------------------------------------------------------------
Module::SnapshotList Module::getSnapshots (const std::string& modulePath)
{
	SnapshotList list;
	getModuleSnapshots (modulePath, list);
	return list;
}

//------------------------------------------------------------------------
Optional<std::string> Module::getModuleInfoPath (const std::string& modulePath)
{
	auto* nsString = [NSString stringWithUTF8String:modulePath.data ()];
	if (!nsString)
		return {};
	auto bundleUrl = [NSURL fileURLWithPath:nsString];
	if (!bundleUrl)
		return {};
	auto moduleInfoUrl = [NSBundle URLForResource:@"moduleinfo"
	                                withExtension:@"json"
	                                 subdirectory:nullptr
	                              inBundleWithURL:bundleUrl];
	if (!moduleInfoUrl)
		return {};
	NSError* error = nil;
	if ([moduleInfoUrl checkResourceIsReachableAndReturnError:&error])
		return {std::string (moduleInfoUrl.fileSystemRepresentation)};
	return {};
}

//------------------------------------------------------------------------
bool Module::validateBundleStructure (const std::string& path, std::string& errorDescription)
{
	auto* nsString = [NSString stringWithUTF8String:path.data ()];
	if (!nsString)
		return false;
	return [NSBundle bundleWithPath:nsString] != nil;
}

//------------------------------------------------------------------------
} // Hosting
} // VST3
