#include "PropertiesPanel.h"
#include "NodeItem.h"
#include "nodes/CameraNode.h"
#include "nodes/VideoNode.h"
#include <QFormLayout>

PropertiesPanel::PropertiesPanel(QWidget *parent) : QWidget(parent)
{
	m_layout = new QVBoxLayout(this);
	m_layout->setAlignment(Qt::AlignTop);
	setLayout(m_layout);

	// Initial label
	m_layout->addWidget(new QLabel("No Node Selected"));
}

void PropertiesPanel::setNode(NodeItem *node)
{
	if (m_currentNode == node)
		return;

	m_currentNode = node;
	clearLayout();

	if (!m_currentNode) {
		m_layout->addWidget(new QLabel("No Node Selected"));
		return;
	}

	// Title
	QLabel *title = new QLabel(m_currentNode->title());
	QFont f = title->font();
	f.setBold(true);
	f.setPointSize(12);
	title->setFont(f);
	m_layout->addWidget(title);

	// Build UI sections
	buildSpatialUI();

	// Check for specific types
	if (auto *cam = dynamic_cast<CameraNode *>(m_currentNode)) {
		buildCameraUI();
	} else if (auto *vid = dynamic_cast<VideoNode *>(m_currentNode)) {
		buildVideoUI();
	}

	m_layout->addStretch();
}

void PropertiesPanel::clearLayout()
{
	QLayoutItem *item;
	while ((item = m_layout->takeAt(0)) != nullptr) {
		delete item->widget();
		delete item;
	}
}

QGroupBox *PropertiesPanel::createTransformGroup()
{
	QGroupBox *group = new QGroupBox("Spatial Transform 3D");
	QFormLayout *form = new QFormLayout(group);

	// Helper to create 3 spinboxes row
	auto createRow = [this](const QString &label, QDoubleSpinBox *&x, QDoubleSpinBox *&y, QDoubleSpinBox *&z) {
		QWidget *rowWidget = new QWidget();
		QHBoxLayout *rowLayout = new QHBoxLayout(rowWidget);
		rowLayout->setContentsMargins(0, 0, 0, 0);

		x = new QDoubleSpinBox();
		x->setRange(-10000, 10000);
		y = new QDoubleSpinBox();
		y->setRange(-10000, 10000);
		z = new QDoubleSpinBox();
		z->setRange(-10000, 10000);

		rowLayout->addWidget(new QLabel("X"));
		rowLayout->addWidget(x);
		rowLayout->addWidget(new QLabel("Y"));
		rowLayout->addWidget(y);
		rowLayout->addWidget(new QLabel("Z"));
		rowLayout->addWidget(z);

		return rowWidget;
	};

	// Position
	QWidget *posRow = createRow("Position", m_posX, m_posY, m_posZ);
	form->addRow("Position:", posRow);

	// Initialize values
	QVector3D pos = m_currentNode->spatialPosition();
	m_posX->setValue(pos.x());
	m_posY->setValue(pos.y());
	m_posZ->setValue(pos.z());

	connect(m_posX, &QDoubleSpinBox::valueChanged, this, &PropertiesPanel::onSpatialPositionChanged);
	connect(m_posY, &QDoubleSpinBox::valueChanged, this, &PropertiesPanel::onSpatialPositionChanged);
	connect(m_posZ, &QDoubleSpinBox::valueChanged, this, &PropertiesPanel::onSpatialPositionChanged);

	// Rotation (simplified Euler for now, usually needs Quat conversion)
	// For simplicity in this demo, we'll just show X/Y/Z euler
	QWidget *rotRow = createRow("Rotation", m_rotX, m_rotY, m_rotZ);
	form->addRow("Rotation:", rotRow);
	// TODO: convert quat to euler for display

	// Scale
	QWidget *scaleRow = createRow("Scale", m_scaleX, m_scaleY, m_scaleZ);
	form->addRow("Scale:", scaleRow);
	QVector3D scale = m_currentNode->spatialScale();
	m_scaleX->setValue(scale.x());
	m_scaleY->setValue(scale.y());
	m_scaleZ->setValue(scale.z());

	connect(m_scaleX, &QDoubleSpinBox::valueChanged, this, &PropertiesPanel::onSpatialScaleChanged);
	connect(m_scaleY, &QDoubleSpinBox::valueChanged, this, &PropertiesPanel::onSpatialScaleChanged);
	connect(m_scaleZ, &QDoubleSpinBox::valueChanged, this, &PropertiesPanel::onSpatialScaleChanged);

	return group;
}

void PropertiesPanel::buildSpatialUI()
{
	m_layout->addWidget(createTransformGroup());
}

void PropertiesPanel::buildCameraUI()
{
	CameraNode *cam = static_cast<CameraNode *>(m_currentNode);
	QGroupBox *group = new QGroupBox("VR Camera Settings");
	QFormLayout *form = new QFormLayout(group);

	// Stereo Mode
	QComboBox *stereoCombo = new QComboBox();
	stereoCombo->addItem("Monoscopic", (int)CameraNode::StereoMode::Monoscopic);
	stereoCombo->addItem("Stereo Side-by-Side", (int)CameraNode::StereoMode::Stereo_SBS);
	stereoCombo->addItem("Stereo Top-Bottom", (int)CameraNode::StereoMode::Stereo_TB);
	stereoCombo->setCurrentIndex((int)cam->stereoMode());
	connect(stereoCombo, QOverload<int>::of(&QComboBox::currentIndexChanged), [this](int index) {
		if (auto c = dynamic_cast<CameraNode *>(m_currentNode))
			c->setStereoMode((CameraNode::StereoMode)index);
	});
	form->addRow("Stereo Mode:", stereoCombo);

	// Lens Type
	QComboBox *lensCombo = new QComboBox();
	lensCombo->addItem("Standard (Rectilinear)", (int)CameraNode::LensType::Standard);
	lensCombo->addItem("VR180 (Fisheye)", (int)CameraNode::LensType::VR180_Fisheye);
	lensCombo->addItem("VR180 (Equirectangular)", (int)CameraNode::LensType::VR180_Equirectangular);
	lensCombo->addItem("VR360", (int)CameraNode::LensType::VR360_Equirectangular);
	lensCombo->setCurrentIndex((int)cam->lensType());
	connect(lensCombo, QOverload<int>::of(&QComboBox::currentIndexChanged), [this](int index) {
		if (auto c = dynamic_cast<CameraNode *>(m_currentNode))
			c->setLensType((CameraNode::LensType)index);
	});
	form->addRow("Lens Type:", lensCombo);

	m_layout->addWidget(group);
}

void PropertiesPanel::buildVideoUI()
{
	VideoNode *vid = static_cast<VideoNode *>(m_currentNode);
	QGroupBox *group = new QGroupBox("VR Video Settings");
	QFormLayout *form = new QFormLayout(group);

	// Stereo Mode
	QComboBox *stereoCombo = new QComboBox();
	stereoCombo->addItem("Monoscopic", (int)VideoNode::StereoMode::Monoscopic);
	stereoCombo->addItem("Stereo Side-by-Side", (int)VideoNode::StereoMode::Stereo_SBS);
	stereoCombo->addItem("Stereo Top-Bottom", (int)VideoNode::StereoMode::Stereo_TB);
	stereoCombo->addItem("Stereo MVC", (int)VideoNode::StereoMode::Stereo_MVC);
	stereoCombo->setCurrentIndex((int)vid->stereoMode());
	connect(stereoCombo, QOverload<int>::of(&QComboBox::currentIndexChanged), [this](int index) {
		if (auto v = dynamic_cast<VideoNode *>(m_currentNode))
			v->setStereoMode((VideoNode::StereoMode)index);
	});
	form->addRow("Stereo Mode:", stereoCombo);

	m_layout->addWidget(group);
}

void PropertiesPanel::onSpatialPositionChanged()
{
	if (!m_currentNode)
		return;
	m_currentNode->setSpatialPosition(QVector3D(m_posX->value(), m_posY->value(), m_posZ->value()));
}

void PropertiesPanel::onSpatialScaleChanged()
{
	if (!m_currentNode)
		return;
	m_currentNode->setSpatialScale(QVector3D(m_scaleX->value(), m_scaleY->value(), m_scaleZ->value()));
}

void PropertiesPanel::onSpatialRotationChanged()
{
	// Not implemented for brevity
}

// Stubs for specialized slots if not using lambdas
void PropertiesPanel::onStereoModeChanged(int index) {}
void PropertiesPanel::onLensTypeChanged(int index) {}
