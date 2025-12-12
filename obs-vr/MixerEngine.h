#pragma once

#include <QObject>
#include <QVector>
#include <QVector2D>
#include <QVector3D>
#include <QString>
#include "../shared/VRHeadsetProfile.h"

namespace NeuralStudio {

    class MixerEngine : public QObject
    {
        Q_OBJECT

          public:
        enum LayoutType {
            Single = 0,
            PictureInPicture = 1,
            SideBySide = 2,
            Grid2x2 = 3,
            Grid3x3 = 4
        };
        Q_ENUM(LayoutType)

        struct FeedTransform {
            QString feedId;
            QVector3D position;  // 3D position in scene (mm)
            QVector2D scale;     // 2D scale (width, height in meters)
            int zOrder;          // Rendering order (lower = further back)
            bool visible;

            FeedTransform() : position(0, 0, -5000), scale(10, 5.6), zOrder(0), visible(true) {}
        };

        explicit MixerEngine(QObject *parent = nullptr);

        // Layout configuration
        void setLayoutType(LayoutType type);
        LayoutType layoutType() const
        {
            return m_layoutType;
        }

        void setActiveFeedId(const QString &feedId);
        QString activeFeedId() const
        {
            return m_activeFeedId;
        }

        // Calculate transforms for current layout
        QVector<FeedTransform> calculateLayout(const QVector<QString> &feedIds, const VRHeadsetProfile &profile) const;

          signals:
        void layoutChanged();
        void activeFeedChanged(const QString &feedId);

          private:
        // Layout calculation functions
        QVector<FeedTransform> layoutSingle(const QVector<QString> &feeds) const;
        QVector<FeedTransform> layoutPIP(const QVector<QString> &feeds) const;
        QVector<FeedTransform> layoutSideBySide(const QVector<QString> &feeds) const;
        QVector<FeedTransform> layoutGrid2x2(const QVector<QString> &feeds) const;
        QVector<FeedTransform> layoutGrid3x3(const QVector<QString> &feeds) const;

        LayoutType m_layoutType;
        QString m_activeFeedId;
    };

}  // namespace NeuralStudio
