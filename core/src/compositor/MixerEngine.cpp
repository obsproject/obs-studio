#include "MixerEngine.h"
#include <QDebug>
#include <QtMath>

namespace NeuralStudio {

MixerEngine::MixerEngine(QObject *parent) : QObject(parent), m_layoutType(LayoutType::Single) {}

void MixerEngine::setLayoutType(LayoutType type)
{
	if (m_layoutType != type) {
		m_layoutType = type;
		emit layoutChanged();
	}
}

void MixerEngine::setActiveFeedId(const QString &feedId)
{
	if (m_activeFeedId != feedId) {
		m_activeFeedId = feedId;
		emit activeFeedChanged(feedId);
	}
}

QVector<MixerEngine::FeedTransform> MixerEngine::calculateLayout(const QVector<QString> &feedIds,
								 const VRHeadsetProfile &profile) const
{
	if (feedIds.isEmpty()) {
		return QVector<FeedTransform>();
	}

	switch (m_layoutType) {
	case Single:
		return layoutSingle(feedIds);
	case PictureInPicture:
		return layoutPIP(feedIds);
	case SideBySide:
		return layoutSideBySide(feedIds);
	case Grid2x2:
		return layoutGrid2x2(feedIds);
	case Grid3x3:
		return layoutGrid3x3(feedIds);
	default:
		return layoutSingle(feedIds);
	}
}

QVector<MixerEngine::FeedTransform> MixerEngine::layoutSingle(const QVector<QString> &feedIds) const
{
	QVector<FeedTransform> transforms;

	// Single feed at standard video plane position
	FeedTransform main;
	main.feedId = m_activeFeedId.isEmpty() ? feedIds[0] : m_activeFeedId;
	main.position = QVector3D(0, 0, -5000); // 5m from viewer
	main.scale = QVector2D(10, 5.6);        // 10m wide, 16:9 aspect
	main.zOrder = 0;
	main.visible = true;
	transforms.append(main);

	return transforms;
}

QVector<MixerEngine::FeedTransform> MixerEngine::layoutPIP(const QVector<QString> &feedIds) const
{
	QVector<FeedTransform> transforms;

	if (feedIds.isEmpty())
		return transforms;

	// Main feed (full size)
	FeedTransform main;
	main.feedId = m_activeFeedId.isEmpty() ? feedIds[0] : m_activeFeedId;
	main.position = QVector3D(0, 0, -5000);
	main.scale = QVector2D(10, 5.6);
	main.zOrder = 0;
	main.visible = true;
	transforms.append(main);

	// PIP feeds (smaller, in front)
	int pipCount = 0;
	for (const QString &feedId : feedIds) {
		if (feedId == main.feedId)
			continue;

		FeedTransform pip;
		pip.feedId = feedId;
		// Position PIPs in top-right area, stacked vertically
		pip.position = QVector3D(3.5f,                     // Right side
					 2.5f - (pipCount * 1.5f), // Top, stacked down
					 -2000                     // In front of main
		);
		pip.scale = QVector2D(2, 1.125); // Small 16:9
		pip.zOrder = 10 + pipCount;
		pip.visible = true;
		transforms.append(pip);

		pipCount++;
		if (pipCount >= 3)
			break; // Max 3 PIP feeds
	}

	return transforms;
}

QVector<MixerEngine::FeedTransform> MixerEngine::layoutSideBySide(const QVector<QString> &feedIds) const
{
	QVector<FeedTransform> transforms;

	int feedCount = qMin(feedIds.size(), 4); // Max 4 feeds side-by-side
	if (feedCount == 0)
		return transforms;

	float totalWidth = 10.0f;
	float feedWidth = totalWidth / feedCount;
	float feedHeight = feedWidth / (16.0f / 9.0f);
	float startX = -(totalWidth / 2.0f) + (feedWidth / 2.0f);

	for (int i = 0; i < feedCount; ++i) {
		FeedTransform transform;
		transform.feedId = feedIds[i];
		transform.position = QVector3D(startX + (i * feedWidth), 0, -5000);
		transform.scale = QVector2D(feedWidth * 0.95f, feedHeight * 0.95f); // 5% gap
		transform.zOrder = i;
		transform.visible = true;
		transforms.append(transform);
	}

	return transforms;
}

QVector<MixerEngine::FeedTransform> MixerEngine::layoutGrid2x2(const QVector<QString> &feedIds) const
{
	QVector<FeedTransform> transforms;

	int feedCount = qMin(feedIds.size(), 4);
	if (feedCount == 0)
		return transforms;

	float cellWidth = 5.0f;  // 5m per cell
	float cellHeight = 2.8f; // 16:9 aspect

	// Grid positions (2x2)
	const QVector2D gridPositions[] = {
		QVector2D(-2.5f, 1.4f),  // Top-left
		QVector2D(2.5f, 1.4f),   // Top-right
		QVector2D(-2.5f, -1.4f), // Bottom-left
		QVector2D(2.5f, -1.4f)   // Bottom-right
	};

	for (int i = 0; i < feedCount; ++i) {
		FeedTransform transform;
		transform.feedId = feedIds[i];
		transform.position = QVector3D(gridPositions[i].x(), gridPositions[i].y(), -5000);
		transform.scale = QVector2D(cellWidth * 0.9f, cellHeight * 0.9f);
		transform.zOrder = i;
		transform.visible = true;
		transforms.append(transform);
	}

	return transforms;
}

QVector<MixerEngine::FeedTransform> MixerEngine::layoutGrid3x3(const QVector<QString> &feedIds) const
{
	QVector<FeedTransform> transforms;

	int feedCount = qMin(feedIds.size(), 9);
	if (feedCount == 0)
		return transforms;

	float cellWidth = 3.33f;  // 10m / 3
	float cellHeight = 1.87f; // 16:9 aspect

	for (int i = 0; i < feedCount; ++i) {
		int row = i / 3;
		int col = i % 3;

		FeedTransform transform;
		transform.feedId = feedIds[i];
		transform.position = QVector3D(-5.0f + (col * cellWidth) + (cellWidth / 2.0f),
					       2.8f - (row * cellHeight) - (cellHeight / 2.0f), -5000);
		transform.scale = QVector2D(cellWidth * 0.9f, cellHeight * 0.9f);
		transform.zOrder = i;
		transform.visible = true;
		transforms.append(transform);
	}

	return transforms;
}

} // namespace NeuralStudio
