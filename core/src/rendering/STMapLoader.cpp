#include "STMapLoader.h"
#include <QImage>
#include <QFile>
#include <QDebug>

namespace NeuralStudio {
namespace Rendering {

STMapLoader::STMapLoader(QRhi *rhi, QObject *parent) : QObject(parent), m_rhi(rhi) {}

STMapLoader::~STMapLoader()
{
	unloadAll();
}

bool STMapLoader::loadSTMap(const QString &path, const QString &id)
{
	if (!m_rhi) {
		qCritical() << "Invalid QRhi instance";
		emit loadError(path, "Invalid QRhi instance");
		return false;
	}

	// Check if already loaded
	if (isLoaded(id)) {
		qWarning() << "STMap already loaded:" << id;
		return true;
	}

	// Check if file exists
	if (!QFile::exists(path)) {
		qCritical() << "STMap file not found:" << path;
		emit loadError(path, "File not found");
		return false;
	}

	// Load image using Qt
	QImage image(path);
	if (image.isNull()) {
		qCritical() << "Failed to load STMap image:" << path;
		emit loadError(path, "Failed to load image");
		return false;
	}

	// Convert to RGBA8 format
	QImage rgba = image.convertToFormat(QImage::Format_RGBA8888);
	if (rgba.isNull()) {
		qCritical() << "Failed to convert STMap to RGBA:" << path;
		emit loadError(path, "Format conversion failed");
		return false;
	}

	// Create Vulkan texture
	std::unique_ptr<QRhiTexture> texture(m_rhi->newTexture(QRhiTexture::RGBA8, QSize(rgba.width(), rgba.height()),
							       1, QRhiTexture::UsedAsTransferSource));

	if (!texture->create()) {
		qCritical() << "Failed to create Vulkan texture for STMap:" << path;
		emit loadError(path, "Vulkan texture creation failed");
		return false;
	}

	// Upload image data to texture
	QRhiResourceUpdateBatch *batch = m_rhi->nextResourceUpdateBatch();
	QRhiTextureUploadDescription uploadDesc(
		{{0, 0, QRhiTextureSubresourceUploadDescription(rgba.constBits(), rgba.sizeInBytes())}});
	batch->uploadTexture(texture.get(), uploadDesc);

	// Submit upload (note: caller should submit this batch to a command buffer)
	// For now, we'll assume the batch will be submitted by the renderer

	// Store STMap data
	STMapData data;
	data.id = id;
	data.path = path;
	data.texture = std::move(texture);
	data.width = rgba.width();
	data.height = rgba.height();

	m_stmaps[id] = std::move(data);

	qInfo() << "Loaded STMap:" << id;
	qInfo() << "  Path:" << path;
	qInfo() << "  Resolution:" << rgba.width() << "x" << rgba.height();

	emit stmapLoaded(id);
	return true;
}

QRhiTexture *STMapLoader::getSTMap(const QString &id)
{
	auto it = m_stmaps.find(id);
	if (it != m_stmaps.end()) {
		return it->second.texture.get();
	}
	return nullptr;
}

void STMapLoader::unloadSTMap(const QString &id)
{
	auto it = m_stmaps.find(id);
	if (it != m_stmaps.end()) {
		qInfo() << "Unloaded STMap:" << id;
		m_stmaps.erase(it);
		emit stmapUnloaded(id);
	}
}

void STMapLoader::unloadAll()
{
	for (const auto &pair : m_stmaps) {
		emit stmapUnloaded(pair.first);
	}
	m_stmaps.clear();
	qInfo() << "Unloaded all STMaps";
}

bool STMapLoader::isLoaded(const QString &id) const
{
	return m_stmaps.find(id) != m_stmaps.end();
}

} // namespace Rendering
} // namespace NeuralStudio
