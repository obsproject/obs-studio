#pragma once

#include "../NodeItem.h"
#include <QJsonObject>

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
        if (m_stereoMode == mode)
            return;
        m_stereoMode = mode;

        QJsonObject data;
        data["stereoMode"] = (int) m_stereoMode;
        emit nodeUpdated(id().toString(), data);
    }
    StereoMode stereoMode() const
    {
        return m_stereoMode;
    }

      private:
    StereoMode m_stereoMode = StereoMode::Stereo_SBS;
};
