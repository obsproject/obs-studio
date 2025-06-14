//-----------------------------------------------------------------------------
// Project     : VST SDK
//
// Category    : Helpers
// Filename    : public.sdk/source/vst/hosting/hostclasses.h
// Created by  : Steinberg, 03/05/2008.
// Description : VST 3 hostclasses, example impl. for IHostApplication, IAttributeList and IMessage
//
//-----------------------------------------------------------------------------
// This file is part of a Steinberg SDK. It is subject to the license terms
// in the LICENSE file found in the top-level directory of this distribution
// and at www.steinberg.net/sdklicenses.
// No part of the SDK, including this file, may be copied, modified, propagated,
// or distributed except according to the terms contained in the LICENSE file.
//-----------------------------------------------------------------------------

#pragma once

#include "public.sdk/source/vst/hosting/pluginterfacesupport.h"
#include "pluginterfaces/vst/ivsthostapplication.h"
#include <map>
#include <memory>
#include <string>

namespace Steinberg {
namespace Vst {

//------------------------------------------------------------------------
/** Implementation's example of IHostApplication.
\ingroup hostingBase
*/
class HostApplication : public IHostApplication
{
public:
	HostApplication ();
	virtual ~HostApplication () noexcept {FUNKNOWN_DTOR}

	//--- IHostApplication ---------------
	tresult PLUGIN_API getName (String128 name) override;
	tresult PLUGIN_API createInstance (TUID cid, TUID _iid, void** obj) override;

	DECLARE_FUNKNOWN_METHODS

	PlugInterfaceSupport* getPlugInterfaceSupport () const { return mPlugInterfaceSupport; }

private:
	IPtr<PlugInterfaceSupport> mPlugInterfaceSupport;
};

//------------------------------------------------------------------------
/** Example, ready to use implementation of IAttributeList.
\ingroup hostingBase
*/
class HostAttributeList final : public IAttributeList
{
public:
	/** make a new attribute list instance */
	static IPtr<IAttributeList> make ();

	tresult PLUGIN_API setInt (AttrID aid, int64 value) override;
	tresult PLUGIN_API getInt (AttrID aid, int64& value) override;
	tresult PLUGIN_API setFloat (AttrID aid, double value) override;
	tresult PLUGIN_API getFloat (AttrID aid, double& value) override;
	tresult PLUGIN_API setString (AttrID aid, const TChar* string) override;
	tresult PLUGIN_API getString (AttrID aid, TChar* string, uint32 sizeInBytes) override;
	tresult PLUGIN_API setBinary (AttrID aid, const void* data, uint32 sizeInBytes) override;
	tresult PLUGIN_API getBinary (AttrID aid, const void*& data, uint32& sizeInBytes) override;

	virtual ~HostAttributeList () noexcept;
	DECLARE_FUNKNOWN_METHODS
private:
	HostAttributeList ();

	struct Attribute;
	std::map<std::string, Attribute> list;
};

//------------------------------------------------------------------------
/** Example implementation of IMessage.
\ingroup hostingBase
*/
class HostMessage final : public IMessage
{
public:
	HostMessage ();
	virtual ~HostMessage () noexcept;

	const char* PLUGIN_API getMessageID () override;
	void PLUGIN_API setMessageID (const char* messageID) override;
	IAttributeList* PLUGIN_API getAttributes () override;

	DECLARE_FUNKNOWN_METHODS
private:
	char* messageId {nullptr};
	IPtr<IAttributeList> attributeList;
};

//------------------------------------------------------------------------
} // namespace Vst
} // namespace Steinberg
