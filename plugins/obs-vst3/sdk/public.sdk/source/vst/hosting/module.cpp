//-----------------------------------------------------------------------------
// Project     : VST SDK
//
// Category    : Helpers
// Filename    : public.sdk/source/vst/hosting/module.cpp
// Created by  : Steinberg, 08/2016
// Description : hosting module classes
//
//-----------------------------------------------------------------------------
// This file is part of a Steinberg SDK. It is subject to the license terms
// in the LICENSE file found in the top-level directory of this distribution
// and at www.steinberg.net/sdklicenses.
// No part of the SDK, including this file, may be copied, modified, propagated,
// or distributed except according to the terms contained in the LICENSE file.
//-----------------------------------------------------------------------------

#include "module.h"
#include "public.sdk/source/vst/utility/stringconvert.h"
#include "public.sdk/source/vst/utility/optional.h"
#include "pluginterfaces/base/funknownimpl.h"
#include <sstream>
#include <utility>

//------------------------------------------------------------------------
namespace VST3 {
namespace Hosting {

//------------------------------------------------------------------------
FactoryInfo::FactoryInfo (PFactoryInfo&& other) noexcept
{
	*this = std::move (other);
}

//------------------------------------------------------------------------
FactoryInfo& FactoryInfo::operator= (FactoryInfo&& other) noexcept
{
	info = std::move (other.info);
	other.info = {};
	return *this;
}

//------------------------------------------------------------------------
FactoryInfo& FactoryInfo::operator= (PFactoryInfo&& other) noexcept
{
	info = std::move (other);
	other = {};
	return *this;
}

//------------------------------------------------------------------------
std::string FactoryInfo::vendor () const noexcept
{
	namespace StringConvert = Steinberg::Vst::StringConvert;
	return StringConvert::convert (info.vendor, PFactoryInfo::kNameSize);
}

//------------------------------------------------------------------------
std::string FactoryInfo::url () const noexcept
{
	namespace StringConvert = Steinberg::Vst::StringConvert;
	return StringConvert::convert (info.url, PFactoryInfo::kURLSize);
}

//------------------------------------------------------------------------
std::string FactoryInfo::email () const noexcept
{
	namespace StringConvert = Steinberg::Vst::StringConvert;
	return StringConvert::convert (info.email, PFactoryInfo::kEmailSize);
}

//------------------------------------------------------------------------
Steinberg::int32 FactoryInfo::flags () const noexcept
{
	return info.flags;
}

//------------------------------------------------------------------------
bool FactoryInfo::classesDiscardable () const noexcept
{
	return (info.flags & PFactoryInfo::kClassesDiscardable) != 0;
}

//------------------------------------------------------------------------
bool FactoryInfo::licenseCheck () const noexcept
{
	return (info.flags & PFactoryInfo::kLicenseCheck) != 0;
}

//------------------------------------------------------------------------
bool FactoryInfo::componentNonDiscardable () const noexcept
{
	return (info.flags & PFactoryInfo::kComponentNonDiscardable) != 0;
}

//------------------------------------------------------------------------
//------------------------------------------------------------------------
//------------------------------------------------------------------------
PluginFactory::PluginFactory (const PluginFactoryPtr& factory) noexcept : factory (factory)
{
}

//------------------------------------------------------------------------
void PluginFactory::setHostContext (Steinberg::FUnknown* context) const noexcept
{
	if (auto f = Steinberg::FUnknownPtr<Steinberg::IPluginFactory3> (factory))
		f->setHostContext (context);
}

//------------------------------------------------------------------------
FactoryInfo PluginFactory::info () const noexcept
{
	Steinberg::PFactoryInfo i;
	factory->getFactoryInfo (&i);
	return FactoryInfo (std::move (i));
}

//------------------------------------------------------------------------
uint32_t PluginFactory::classCount () const noexcept
{
	auto count = factory->countClasses ();
	assert (count >= 0);
	return static_cast<uint32_t> (count);
}

//------------------------------------------------------------------------
PluginFactory::ClassInfos PluginFactory::classInfos () const noexcept
{
	auto count = classCount ();
	Optional<FactoryInfo> factoryInfo;
	ClassInfos classes;
	classes.reserve (count);
	auto f3 = Steinberg::U::cast<Steinberg::IPluginFactory3> (factory);
	auto f2 = Steinberg::U::cast<Steinberg::IPluginFactory2> (factory);
	Steinberg::PClassInfo ci;
	Steinberg::PClassInfo2 ci2;
	Steinberg::PClassInfoW ci3;
	for (uint32_t i = 0; i < count; ++i)
	{
		if (f3 && f3->getClassInfoUnicode (i, &ci3) == Steinberg::kResultTrue)
			classes.emplace_back (ci3);
		else if (f2 && f2->getClassInfo2 (i, &ci2) == Steinberg::kResultTrue)
			classes.emplace_back (ci2);
		else if (factory->getClassInfo (i, &ci) == Steinberg::kResultTrue)
			classes.emplace_back (ci);
		auto& classInfo = classes.back ();
		if (classInfo.vendor ().empty ())
		{
			if (!factoryInfo)
				factoryInfo = Optional<FactoryInfo> (info ());
			classInfo.get ().vendor = factoryInfo->vendor ();
		}
	}
	return classes;
}

//------------------------------------------------------------------------
//------------------------------------------------------------------------
//------------------------------------------------------------------------
const UID& ClassInfo::ID () const noexcept
{
	return data.classID;
}

//------------------------------------------------------------------------
int32_t ClassInfo::cardinality () const noexcept
{
	return data.cardinality;
}

//------------------------------------------------------------------------
const std::string& ClassInfo::category () const noexcept
{
	return data.category;
}

//------------------------------------------------------------------------
const std::string& ClassInfo::name () const noexcept
{
	return data.name;
}

//------------------------------------------------------------------------
const std::string& ClassInfo::vendor () const noexcept
{
	return data.vendor;
}

//------------------------------------------------------------------------
const std::string& ClassInfo::version () const noexcept
{
	return data.version;
}

//------------------------------------------------------------------------
const std::string& ClassInfo::sdkVersion () const noexcept
{
	return data.sdkVersion;
}

//------------------------------------------------------------------------
const ClassInfo::SubCategories& ClassInfo::subCategories () const noexcept
{
	return data.subCategories;
}

//------------------------------------------------------------------------
Steinberg::uint32 ClassInfo::classFlags () const noexcept
{
	return data.classFlags;
}

//------------------------------------------------------------------------
ClassInfo::ClassInfo (const PClassInfo& info) noexcept
{
	namespace StringConvert = Steinberg::Vst::StringConvert;
	data.classID = info.cid;
	data.cardinality = info.cardinality;
	data.category = StringConvert::convert (info.category, PClassInfo::kCategorySize);
	data.name = StringConvert::convert (info.name, PClassInfo::kNameSize);
}

//------------------------------------------------------------------------
ClassInfo::ClassInfo (const PClassInfo2& info) noexcept
{
	namespace StringConvert = Steinberg::Vst::StringConvert;
	data.classID = info.cid;
	data.cardinality = info.cardinality;
	data.category = StringConvert::convert (info.category, PClassInfo::kCategorySize);
	data.name = StringConvert::convert (info.name, PClassInfo::kNameSize);
	data.vendor = StringConvert::convert (info.vendor, PClassInfo2::kVendorSize);
	data.version = StringConvert::convert (info.version, PClassInfo2::kVersionSize);
	data.sdkVersion = StringConvert::convert (info.sdkVersion, PClassInfo2::kVersionSize);
	parseSubCategories (
	    StringConvert::convert (info.subCategories, PClassInfo2::kSubCategoriesSize));
	data.classFlags = info.classFlags;
}

//------------------------------------------------------------------------
ClassInfo::ClassInfo (const PClassInfoW& info) noexcept
{
	namespace StringConvert = Steinberg::Vst::StringConvert;
	data.classID = info.cid;
	data.cardinality = info.cardinality;
	data.category = StringConvert::convert (info.category, PClassInfo::kCategorySize);
	data.name = StringConvert::convert (info.name, PClassInfo::kNameSize);
	data.vendor = StringConvert::convert (info.vendor, PClassInfo2::kVendorSize);
	data.version = StringConvert::convert (info.version, PClassInfo2::kVersionSize);
	data.sdkVersion = StringConvert::convert (info.sdkVersion, PClassInfo2::kVersionSize);
	parseSubCategories (
	    StringConvert::convert (info.subCategories, PClassInfo2::kSubCategoriesSize));
	data.classFlags = info.classFlags;
}

//------------------------------------------------------------------------
void ClassInfo::parseSubCategories (const std::string& str) noexcept
{
	std::stringstream stream (str);
	std::string item;
	while (std::getline (stream, item, '|'))
		data.subCategories.emplace_back (std::move (item));
}

//------------------------------------------------------------------------
std::string ClassInfo::subCategoriesString () const noexcept
{
	std::string result;
	if (data.subCategories.empty ())
		return result;
	result = data.subCategories[0];
	for (auto index = 1u; index < data.subCategories.size (); ++index)
		result += "|" + data.subCategories[index];
	return result;
}

//------------------------------------------------------------------------
namespace {

//------------------------------------------------------------------------
std::pair<size_t, size_t> rangeOfScaleFactor (const std::string& name)
{
	auto result = std::make_pair (std::string::npos, std::string::npos);
	size_t xIndex = name.find_last_of ('x');
	if (xIndex == std::string::npos)
		return result;

	size_t indicatorIndex = name.find_last_of ('_');
	if (indicatorIndex == std::string::npos)
		return result;
	if (xIndex < indicatorIndex)
		return result;
	result.first = indicatorIndex + 1;
	result.second = xIndex;
	return result;
}

//------------------------------------------------------------------------
} // anonymous

//------------------------------------------------------------------------
Optional<double> Module::Snapshot::decodeScaleFactor (const std::string& name)
{
	auto range = rangeOfScaleFactor (name);
	if (range.first == std::string::npos || range.second == std::string::npos)
		return {};
	std::string tmp (name.data () + range.first, range.second - range.first);
	std::istringstream sstream (tmp);
	sstream.imbue (std::locale::classic ());
	sstream.precision (static_cast<std::streamsize> (3));
	double result;
	sstream >> result;
	return Optional<double> (result);
}

//------------------------------------------------------------------------
Optional<UID> Module::Snapshot::decodeUID (const std::string& filename)
{
	if (filename.size () < 45)
		return {};
	if (filename.find ("_snapshot") != 32)
		return {};
	auto uidStr = filename.substr (0, 32);
	return UID::fromString (uidStr);
}

//------------------------------------------------------------------------
} // Hosting
} // VST3
