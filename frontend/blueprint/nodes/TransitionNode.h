#pragma once

#include "../NodeItem.h"

class TransitionNode : public NodeItem
{
      public:
    TransitionNode() : NodeItem("Transition")
    {
        addInputPort("Source A", PortDataType::Video);
        addInputPort("Source B", PortDataType::Video);
        addOutputPort("Result", PortDataType::Video);

        // Default properties
        // In real impl, these would be Q_PROPERTY exposed to QML
    }
};
