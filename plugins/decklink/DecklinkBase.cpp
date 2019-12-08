#include "DecklinkBase.h"

DecklinkBase::DecklinkBase(DeckLinkDeviceDiscovery *discovery_)
	: discovery(discovery_)
{
}

DeckLinkDevice *DecklinkBase::GetDevice() const
{
	return instance ? instance->GetDevice() : nullptr;
}

bool DecklinkBase::Activate(DeckLinkDevice *, long long)
{
	return false;
}

void DecklinkBase::Deactivate() {}
