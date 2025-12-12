#pragma once

#include "../NodeItem.h"

class FilterNode : public NodeItem
{
      public:
    FilterNode() : NodeItem("Filter")
    {
        addInputPort("In", PortDataType::Video);  // Could be polymorphic (Audio/Video)
        addOutputPort("Out", PortDataType::Video);

        // Property: Filter Type (Blur, Color, etc.)
    }
};
