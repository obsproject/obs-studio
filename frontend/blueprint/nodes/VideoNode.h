#pragma once

#include "../NodeItem.h"

class VideoNode : public NodeItem
{
      public:
    enum class StereoMode {
        Monoscopic,
        Stereo_SBS,
        Stereo_TB,
        Stereo_MVC,  // Multi-View Coding (often used in 3D files)
    };

    VideoNode(const QString &title = "Video") : NodeItem(title)
    {
        addOutputPort("Video Out", PortDataType::Video);
        addOutputPort("Audio Out", PortDataType::Audio);
    }

    void setStereoMode(StereoMode mode)
    {
        m_stereoMode = mode;
    }
    StereoMode stereoMode() const
    {
        return m_stereoMode;
    }

      private:
    StereoMode m_stereoMode = StereoMode::Stereo_SBS;
};
