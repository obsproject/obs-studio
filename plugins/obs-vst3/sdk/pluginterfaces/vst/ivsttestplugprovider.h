//-----------------------------------------------------------------------------
// Flags       : clang-format SMTGSequencer
// Project     : VST SDK
//
// Category    : Validator
// Filename    : public.sdk/source/vst/testsuite/iplugprovider.h
// Created by  : Steinberg, 04/2005
// Description : VST Test Suite
//
//-----------------------------------------------------------------------------
// This file is part of a Steinberg SDK. It is subject to the license terms
// in the LICENSE file found in the top-level directory of this distribution
// and at www.steinberg.net/sdklicenses.
// No part of the SDK, including this file, may be copied, modified, propagated,
// or distributed except according to the terms contained in the LICENSE file.
//-----------------------------------------------------------------------------

#pragma once

#include "pluginterfaces/base/istringresult.h"
#include "pluginterfaces/vst/ivstcomponent.h"
#include "pluginterfaces/vst/ivsteditcontroller.h"

//------------------------------------------------------------------------
namespace Steinberg {
namespace Vst {

//------------------------------------------------------------------------
/** Test Helper.
 * \ingroup TestClass
 *
 *	This class provides access to the component and the controller of a plug-in when running a unit
 *	test (see ITest).
 * 	You get this interface as the context argument in the ITestFactory::createTests method.
 */
class ITestPlugProvider : public FUnknown
{
public:
//------------------------------------------------------------------------
	/** get the component of the plug-in.
	 *
	 * The reference count of the component is increased in this function and you need to call
	 * releasePlugIn when done with the component.
	 */
	virtual IComponent* PLUGIN_API getComponent () = 0;
	/** get the controller of the plug-in.
	 *
	 * The reference count of the controller is increased in this function and you need to call
	 * releasePlugIn when done with the controller.
	 */
	virtual IEditController* PLUGIN_API getController () = 0;
	/** release the component and/or controller */
	virtual tresult PLUGIN_API releasePlugIn (IComponent* component /*in*/,
	                                          IEditController* controller /*in*/) = 0;
	/** get the sub categories of the plug-in */
	virtual tresult PLUGIN_API getSubCategories (IStringResult& result /*out*/) const = 0;
	/** get the component UID of the plug-in */
	virtual tresult PLUGIN_API getComponentUID (FUID& uid /*out*/) const = 0;

//------------------------------------------------------------------------
	static const FUID iid;
};

DECLARE_CLASS_IID (ITestPlugProvider, 0x86BE70EE, 0x4E99430F, 0x978F1E6E, 0xD68FB5BA)

//------------------------------------------------------------------------
/** Test Helper extension.
 * \ingroup TestClass
 */
class ITestPlugProvider2 : public ITestPlugProvider
{
public:
	/** get the plugin factory.
	 *
	 * The reference count of the returned factory object is not increased when calling this
	 * function.
	 */
	virtual IPluginFactory* PLUGIN_API getPluginFactory () = 0;

//------------------------------------------------------------------------
	static const FUID iid;
};

DECLARE_CLASS_IID (ITestPlugProvider2, 0xC7C75364, 0x7B8343AC, 0xA4495B0A, 0x3E5A46C7)

//------------------------------------------------------------------------
} // Vst
} // Steinberg
