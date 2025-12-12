#pragma once

#include "../NodeItem.h"
#include <QJsonObject>

class CameraNode : public NodeItem
{
      public:
    enum class StereoMode {
        Monoscopic,
        Stereo_SBS,  // Side-by-Side
        Stereo_TB,   // Top-Bottom
    };

    enum class LensType {
        Standard,
        VR180_Equirectangular,
        VR180_Fisheye,
        VR360_Equirectangular,
    };

    CameraNode(const QString &title = "Camera") : NodeItem(title)
    {
        addOutputPort("Video Out", PortDataType::Video);
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

    void setLensType(LensType type)
    {
        if (m_lensType == type)
            return;
        m_lensType = type;

        QJsonObject data;
        data["lensType"] = (int) m_lensType;
        emit nodeUpdated(id().toString(), data);
    }
    LensType lensType() const
    {
        return m_lensType;
    }

      private:
    StereoMode m_stereoMode = StereoMode::Stereo_SBS;  // Default to VR
    LensType m_lensType = LensType::VR180_Fisheye;     // Default to VR180
};
