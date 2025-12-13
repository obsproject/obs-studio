#include "VideoInputNode.h"
#include <QPainter>

namespace NeuralStudio {

VideoInputNode::VideoInputNode(const QString &title, QGraphicsItem *parent)
	: NodeItem(title, parent),
	  m_sourceType(VideoSourceType::Camera),
	  m_resolution(1920, 1080),
	  m_frameRate(30),
	  m_stereoMode(StereoMode::Mono),
	  m_position3D(0, 0, -5000),
	  m_scale3D(10, 5.6, 1),
	  m_active(false)
{
	// Add output port for video texture
	addOutputPort("Video Out", PortDataType::Video);
	addOutputPort("Metadata", PortDataType::Data);
}

void VideoInputNode::setSourceType(VideoSourceType type)
{
	if (m_sourceType != type) {
		m_sourceType = type;
		emit sourceChanged();
		emitSpatialUpdate();
	}
}

void VideoInputNode::setSourcePath(const QString &path)
{
	if (m_sourcePath != path) {
		m_sourcePath = path;
		emit sourceChanged();
	}
}

void VideoInputNode::setResolution(int width, int height)
{
	QSize newRes(width, height);
	if (m_resolution != newRes) {
		m_resolution = newRes;
		emit videoPropertiesChanged();
	}
}

void VideoInputNode::setFrameRate(int fps)
{
	if (m_frameRate != fps) {
		m_frameRate = fps;
		emit videoPropertiesChanged();
	}
}

void VideoInputNode::setStereoMode(StereoMode mode)
{
	if (m_stereoMode != mode) {
		m_stereoMode = mode;
		emit videoPropertiesChanged();
		emitSpatialUpdate();
	}
}

void VideoInputNode::setPosition3D(const QVector3D &pos)
{
	if (m_position3D != pos) {
		m_position3D = pos;
		emit position3DChanged();
		emitSpatialUpdate();
	}
}

void VideoInputNode::setScale3D(const QVector3D &scale)
{
	if (m_scale3D != scale) {
		m_scale3D = scale;
		emit position3DChanged();
		emitSpatialUpdate();
	}
}

void VideoInputNode::setActive(bool active)
{
	if (m_active != active) {
		m_active = active;
		emit activeStateChanged(active);
		update(); // Redraw node
	}
}

} // namespace NeuralStudio
