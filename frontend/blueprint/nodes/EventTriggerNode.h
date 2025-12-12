#pragma once

#include "../NodeItem.h"

class EventTriggerNode : public NodeItem
{
      public:
    EventTriggerNode() : NodeItem("Event Trigger")
    {
        addOutputPort("Signal", PortDataType::Script);  // or Event type
        // Property: Event Name (e.g. "OnStreamStart")
    }
};
