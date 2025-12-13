#pragma once

#include <QWidget>
#include <QVBoxLayout>
#include <QLabel>
#include <QDoubleSpinBox>
#include <QComboBox>
#include <QGroupBox>

class NodeItem;

class PropertiesPanel : public QWidget
{
    Q_OBJECT
      public:
    explicit PropertiesPanel(QWidget *parent = nullptr);
    void setNode(NodeItem *node);

      private slots:
    void onSpatialPositionChanged();
    void onSpatialRotationChanged();
    void onSpatialScaleChanged();
    // Specialized slots
    void onStereoModeChanged(int index);
    void onLensTypeChanged(int index);

      private:
    NodeItem *m_currentNode = nullptr;
    QVBoxLayout *m_layout;

    // Spatial Widgets
    QDoubleSpinBox *m_posX, *m_posY, *m_posZ;
    QDoubleSpinBox *m_rotX, *m_rotY, *m_rotZ;  // Euler angles for simplicity
    QDoubleSpinBox *m_scaleX, *m_scaleY, *m_scaleZ;

    // Wrappers for clearing layout
    void clearLayout();
    void buildSpatialUI();
    void buildCameraUI();  // If CameraNode
    void buildVideoUI();   // If VideoNode

    // Helpers
    QGroupBox *createTransformGroup();
};
