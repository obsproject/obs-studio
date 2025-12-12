#pragma once

#include "NodeItem.h"

// Concrete Node classes could be defined here or inherited.
// For now, NodeItem is generic enough, but we can subtype if needed for custom painting/IO.

class CameraNode : public NodeItem
{
      public:
    CameraNode(const QString &title) : NodeItem(title, 0, 0)
    {
        // Customize ports for Camera (Output only)
    }
    // Override paint or logic
};
// ... we will use the core NodeItem for the skeleton.
