#pragma once

#include "../NodeItem.h"

class EffectNode : public NodeItem
{
      public:
    EffectNode(const QString &title = "Effect") : NodeItem(title)
    {
        addInputPort("Video In", PortDataType::Video);
        addOutputPort("Video Out", PortDataType::Video);
    }
};
