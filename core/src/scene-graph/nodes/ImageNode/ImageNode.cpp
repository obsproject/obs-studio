#include "ImageNode.h"
#include <iostream>

namespace NeuralStudio {
namespace SceneGraph {

ImageNode::ImageNode(const std::string &id) : BaseNodeBackend(id, "ImageNode")
{
	// Define Outputs - image is a static visual source
	addOutput("visual_out", "Visual Output", DataType::Texture2D());
}

ImageNode::~ImageNode()
{
	// Cleanup texture resources
}

void ImageNode::execute(const ExecutionContext &context)
{
	if (m_dirty && !m_imagePath.empty()) {
		// Load image from disk
		std::cout << "ImageNode: Loading " << m_imagePath << std::endl;

		// Future: Load image using stb_image, QImage, or similar
		// m_textureId = TextureLoader::load(m_imagePath, m_filterMode);
		// Extract metadata: m_width, m_height, m_hasAlpha

		m_dirty = false;
	}

	// Output loaded texture
	if (m_textureId > 0) {
		// setOutputData("visual_out", m_textureId);
	}
}

void ImageNode::setImagePath(const std::string &path)
{
	if (m_imagePath != path) {
		m_imagePath = path;
		m_dirty = true;
	}
}

std::string ImageNode::getImagePath() const
{
	return m_imagePath;
}

void ImageNode::setFilterMode(const std::string &filterMode)
{
	if (m_filterMode != filterMode) {
		m_filterMode = filterMode;
		m_dirty = true; // Reload with new filter settings
	}
}

std::string ImageNode::getFilterMode() const
{
	return m_filterMode;
}

} // namespace SceneGraph
} // namespace NeuralStudio
