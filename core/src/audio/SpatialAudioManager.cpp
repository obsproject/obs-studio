#include "SpatialAudioManager.h"
#include <QDebug>

namespace NeuralStudio {

SpatialAudioManager::SpatialAudioManager() : m_audioEngine(nullptr), m_virtualRoom(nullptr), m_initialized(false) {}

SpatialAudioManager::~SpatialAudioManager()
{
	shutdown();
}

void SpatialAudioManager::initialize()
{
	if (m_initialized) {
		qWarning() << "SpatialAudioManager already initialized";
		return;
	}

	m_audioEngine = new QAudioEngine();
	m_audioEngine->start();

	m_initialized = true;
	qDebug() << "SpatialAudioManager initialized";
}

void SpatialAudioManager::shutdown()
{
	if (!m_initialized) {
		return;
	}

	// Clean up spatial sounds
	qDeleteAll(m_spatialSounds);
	m_spatialSounds.clear();

	// Clean up ambient sounds
	qDeleteAll(m_ambientSounds);
	m_ambientSounds.clear();

	// Clean up room
	if (m_virtualRoom) {
		delete m_virtualRoom;
		m_virtualRoom = nullptr;
	}

	// Stop and delete engine
	if (m_audioEngine) {
		m_audioEngine->stop();
		delete m_audioEngine;
		m_audioEngine = nullptr;
	}

	m_initialized = false;
}

void SpatialAudioManager::addSpatialSound(const QString &nodeId, const QString &audioSource)
{
	if (!m_initialized || m_spatialSounds.contains(nodeId)) {
		return;
	}

	QSpatialSound *sound = new QSpatialSound(m_audioEngine);
	sound->setSource(QUrl::fromLocalFile(audioSource));
	sound->setSize(1.0f); // Default size
	sound->setVolume(1.0f);

	m_spatialSounds[nodeId] = sound;
	qDebug() << "Added spatial sound:" << nodeId;
}

void SpatialAudioManager::removeSpatialSound(const QString &nodeId)
{
	if (m_spatialSounds.contains(nodeId)) {
		delete m_spatialSounds.take(nodeId);
	}
}

void SpatialAudioManager::updateSoundPosition(const QString &nodeId, const QVector3D &position)
{
	if (m_spatialSounds.contains(nodeId)) {
		m_spatialSounds[nodeId]->setPosition(position);
	}
}

void SpatialAudioManager::updateSoundVolume(const QString &nodeId, float volume)
{
	if (m_spatialSounds.contains(nodeId)) {
		m_spatialSounds[nodeId]->setVolume(volume);
	}
}

void SpatialAudioManager::addAmbientSound(const QString &id, const QString &audioSource)
{
	if (!m_initialized || m_ambientSounds.contains(id)) {
		return;
	}

	QAmbientSound *ambient = new QAmbientSound(m_audioEngine);
	ambient->setSource(QUrl::fromLocalFile(audioSource));
	ambient->setVolume(1.0f);

	m_ambientSounds[id] = ambient;
}

void SpatialAudioManager::removeAmbientSound(const QString &id)
{
	if (m_ambientSounds.contains(id)) {
		delete m_ambientSounds.take(id);
	}
}

void SpatialAudioManager::updateListenerPosition(const QVector3D &position)
{
	if (m_audioEngine) {
		m_audioEngine->setListenerPosition(position);
	}
}

void SpatialAudioManager::updateListenerOrientation(const QVector3D &forward, const QVector3D &up)
{
	if (m_audioEngine) {
		m_audioEngine->setListenerRotation(QQuaternion::fromDirection(forward, up));
	}
}

void SpatialAudioManager::setVirtualRoom(const QVector3D &roomDimensions, float reflectionGain)
{
	if (!m_initialized) {
		return;
	}

	if (!m_virtualRoom) {
		m_virtualRoom = new QAudioRoom(m_audioEngine);
	}

	m_virtualRoom->setDimensions(roomDimensions);
	m_virtualRoom->setReflectionGain(reflectionGain);
	m_virtualRoom->setReverbGain(reflectionGain * 0.5f); // Reverb is half of reflection

	qDebug() << "Virtual room set:" << roomDimensions << "reflection:" << reflectionGain;
}

void SpatialAudioManager::disableRoom()
{
	if (m_virtualRoom) {
		delete m_virtualRoom;
		m_virtualRoom = nullptr;
	}
}

void SpatialAudioManager::setMasterVolume(float volume)
{
	if (m_audioEngine) {
		m_audioEngine->setMasterVolume(volume);
	}
}

float SpatialAudioManager::getMasterVolume() const
{
	return m_audioEngine ? m_audioEngine->masterVolume() : 0.0f;
}

QByteArray SpatialAudioManager::getMixedAudioBuffer()
{
	// TODO: Extract processed audio from QAudioEngine
	// This requires accessing the internal buffer or output device
	// For now, return empty (will need Qt internal investigation)
	return QByteArray();
}

} // namespace NeuralStudio
