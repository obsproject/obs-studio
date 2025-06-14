//-----------------------------------------------------------------------------
// Project     : VST SDK
//
// Category    : Helpers
// Filename    : public.sdk/source/vst/hosting/plugprovider.cpp
// Created by  : Steinberg, 08/2016
// Description : VST 3 Plug-in Provider class
//
//-----------------------------------------------------------------------------
// This file is part of a Steinberg SDK. It is subject to the license terms
// in the LICENSE file found in the top-level directory of this distribution
// and at www.steinberg.net/sdklicenses.
// No part of the SDK, including this file, may be copied, modified, propagated,
// or distributed except according to the terms contained in the LICENSE file.
//-----------------------------------------------------------------------------

#include "plugprovider.h"

#include "connectionproxy.h"
#include "pluginterfaces/vst/ivstcomponent.h"
#include "pluginterfaces/vst/ivsteditcontroller.h"
#include "pluginterfaces/vst/ivstmessage.h"

#include <cstdio>
#include <iostream>

static std::ostream* errorStream = &std::cout;

//------------------------------------------------------------------------
namespace Steinberg {
namespace Vst {

//------------------------------------------------------------------------
// PlugProvider
//------------------------------------------------------------------------
PlugProvider::PlugProvider (const PluginFactory& factory, ClassInfo classInfo, bool plugIsGlobal)
: factory (factory)
, component (nullptr)
, controller (nullptr)
, classInfo (classInfo)
, plugIsGlobal (plugIsGlobal)
{
}

//------------------------------------------------------------------------
PlugProvider::~PlugProvider ()
{
	terminatePlugin ();
}

//------------------------------------------------------------------------
template <typename Proc>
void PlugProvider::printError (Proc p) const
{
	if (errorStream)
	{
		p (*errorStream);
	}
}

//------------------------------------------------------------------------
bool PlugProvider::initialize ()
{
	if (plugIsGlobal)
	{
		return setupPlugin (PluginContextFactory::instance ().getPluginContext ());
	}
	return true;
}

//------------------------------------------------------------------------
IComponent* PLUGIN_API PlugProvider::getComponent ()
{
	if (!component)
		setupPlugin (PluginContextFactory::instance ().getPluginContext ());

	if (component)
		component->addRef ();

	return component;
}

//------------------------------------------------------------------------
IEditController* PLUGIN_API PlugProvider::getController ()
{
	if (controller)
		controller->addRef ();

	// 'iController == 0' is allowed! In this case the plug has no controller
	return controller;
}

//------------------------------------------------------------------------
IPluginFactory* PLUGIN_API PlugProvider::getPluginFactory ()
{
	if (auto f = factory.get ())
		return f.get ();
	return nullptr;
}

//------------------------------------------------------------------------
tresult PLUGIN_API PlugProvider::getComponentUID (FUID& uid) const
{
	uid = FUID::fromTUID (classInfo.ID ().data ());
	return kResultOk;
}

//------------------------------------------------------------------------
tresult PLUGIN_API PlugProvider::releasePlugIn (IComponent* iComponent,
                                                IEditController* iController)
{
	if (iComponent)
		iComponent->release ();

	if (iController)
		iController->release ();

	if (!plugIsGlobal)
	{
		terminatePlugin ();
	}

	return kResultOk;
}

//------------------------------------------------------------------------
bool PlugProvider::setupPlugin (FUnknown* hostContext)
{
	bool res = false;
	bool isSingleComponent = false;

	//---create Plug-in here!--------------
	// create its component part
	component = factory.createInstance<IComponent> (classInfo.ID ());
	if (component)
	{
		// initialize the component with our context
		if (auto plugBase = U::cast<IPluginBase> (component))
		{
			res = (plugBase->initialize (hostContext) == kResultOk);
			if (res == false)
			{
				printError ([&] (std::ostream& stream) {
					stream << "Failed to initialize component of " << classInfo.name () << "!\n";
				});
				return false;
			}
		}
		else
		{
			printError ([&] (std::ostream& stream) {
				stream << "Failed to get IPluginBase from component of " << classInfo.name ()
				       << "!\n";
			});
			return false;
		}

		// try to create the controller part from the component
		// (for Plug-ins which did not succeed to separate component from controller)
		if (component->queryInterface (IEditController::iid, (void**)&controller) == kResultTrue)
		{
			isSingleComponent = true;
		}
		else
		{
			TUID controllerCID;

			// ask for the associated controller class ID
			if (component->getControllerClassId (controllerCID) == kResultTrue)
			{
				// create its controller part created from the factory
				controller = factory.createInstance<IEditController> (VST3::UID (controllerCID));
				if (controller)
				{
					// initialize the component with our context
					if (auto plugCtrlBase = U::cast<IPluginBase> (controller))
					{
						res = (plugCtrlBase->initialize (hostContext) == kResultOk);
						if (res == false)
						{
							printError ([&] (std::ostream& stream) {
								stream << "Failed to initialize controller of " << classInfo.name ()
								       << "!\n";
							});
						}
					}
					else
					{
						printError ([&] (std::ostream& stream) {
							stream << "Failed to get IPluginBase from controller of "
							       << classInfo.name () << "!\n";
						});
						return false;
					}
				}
			}
			else
			{
				printError ([&] (std::ostream& stream) {
					stream << "Component does not provide a required controller class ID ["
					       << classInfo.name () << "]!\n";
				});
			}
		}
		if (!res)
		{
			component.reset ();
			controller.reset ();
		}
	}
	else if (errorStream)
	{
		printError ([&] (std::ostream& stream) {
			stream << "Failed to create component instance of " << classInfo.name () << "!\n";
		});
	}

	if (res && !isSingleComponent)
		return connectComponents ();

	return res;
}

//------------------------------------------------------------------------
bool PlugProvider::connectComponents ()
{
	if (!component || !controller)
		return false;

	auto compICP = U::cast<IConnectionPoint> (component);
	auto contrICP = U::cast<IConnectionPoint> (controller);
	if (!compICP || !contrICP)
		return false;

	componentCP = owned (new ConnectionProxy (compICP));
	controllerCP = owned (new ConnectionProxy (contrICP));

	tresult tres = componentCP->connect (contrICP);
	if (tres != kResultTrue)
	{
		printError ([&] (std::ostream& stream) {
			stream << "Failed to connect the component with the controller with result code '"
			       << tres << "'!\n";
		});
		return false;
	}
	tres = controllerCP->connect (compICP);
	if (tres != kResultTrue)
	{
		printError ([&] (std::ostream& stream) {
			stream << "Failed to connect the controller with the component with result code '"
			       << tres << "'!\n";
		});
		return false;
	}
	return true;
}

//------------------------------------------------------------------------
bool PlugProvider::disconnectComponents ()
{
	if (!componentCP || !controllerCP)
		return false;

	bool res = componentCP->disconnect ();
	res &= controllerCP->disconnect ();

	componentCP.reset ();
	controllerCP.reset ();

	return res;
}

//------------------------------------------------------------------------
void PlugProvider::terminatePlugin ()
{
	disconnectComponents ();

	bool controllerIsComponent = false;
	if (component)
	{
		controllerIsComponent = FUnknownPtr<IEditController> (component).getInterface () != nullptr;

		if (auto plugBase = U::cast<IPluginBase> (component))
			plugBase->terminate ();
		else
		{
			printError ([&](std::ostream& stream) {
				stream << "Failed to get IPluginBase from component of " << classInfo.name ()
					<< "!\n";
				});
		}
	}

	if (controller && controllerIsComponent == false)
	{
		if (auto plugCtrlBase = U::cast<IPluginBase> (controller))
			plugCtrlBase->terminate ();
		else
		{
			printError ([&](std::ostream& stream) {
				stream << "Failed to get IPluginBase from controller of " << classInfo.name ()
					<< "!\n";
				});
		}
	}

	component.reset ();
	controller.reset ();
}

//------------------------------------------------------------------------
void PlugProvider::setErrorStream (std::ostream* stream)
{
	errorStream = stream;
}

//------------------------------------------------------------------------
} // Vst
} // Steinberg
