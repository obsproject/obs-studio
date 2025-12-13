#include "NodeGraphController.h"
#include "nodes/BaseNodeController.h"
#include <QDebug>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QFile>
#include <QVector3D> // Required for QVector3D
#include <map>       // Required for m_nodePositions
#include <string>    // Required for std::to_string
#include <cstdlib>   // Required for std::rand
#include <optional>
#include "../../../../core/lib/vr/scene/SceneManager.h" // Access to SceneManager internal types
#include "../../../../core/src/scene-graph/nodes/ThreeDModelNode/ThreeDModelNode.h"
#include "../../../../core/src/scene-graph/nodes/CameraNode/CameraNode.h"
#include "../../../../core/src/scene-graph/nodes/VideoNode/VideoNode.h"
#include "../../../../core/src/scene-graph/nodes/ImageNode/ImageNode.h"
#include "../../../../core/src/scene-graph/nodes/AudioNode/AudioNode.h"
#include "../../../../core/src/scene-graph/nodes/FontNode/FontNode.h"
#include "../../../../core/src/scene-graph/nodes/ShaderNode/ShaderNode.h"
#include "../../../../core/src/scene-graph/nodes/TextureNode/TextureNode.h"
#include "../../../../core/src/scene-graph/nodes/EffectNode/EffectNode.h"
#include "../../../../core/src/scene-graph/nodes/ScriptNode/ScriptNode.h"
#include "../../../../core/src/scene-graph/nodes/MLNode/MLNode.h"
#include "../../../../core/src/scene-graph/nodes/LLMNode/LLMNode.h"
namespace NeuralStudio {
namespace UI {

NodeGraphController::NodeGraphController(QObject *parent) : QObject(parent)
{
	// Initialize SceneGraphManager
	m_sceneGraphManager = std::make_shared<SceneGraph::SceneGraphManager>();
	m_sceneGraphManager->initialize(nullptr); // Pass renderer later or if available
}

NodeGraphController::~NodeGraphController()
{
	if (m_sceneGraphManager) {
		m_sceneGraphManager->shutdown();
	}
}

QString NodeGraphController::createNode(const QString &nodeType, float x, float y, float z)
{
	// Generate ID
	auto graph = m_sceneGraphManager->getNodeGraph();
	if (!graph)
		return "";

	std::string id = "node_" + std::to_string(graph->getNodeCount() + 1);
	while (graph->getNode(id)) {
		id = "node_" + std::to_string(std::rand());
	}

	// Delegate to Manager
	if (m_sceneGraphManager->createNode(nodeType.toStdString(), id)) {
		updateNodePosition(QString::fromStdString(id), x, y, z);
		emit nodeCreated(QString::fromStdString(id), nodeType, x, y, z);
		emit isCompiledChanged();
		return QString::fromStdString(id);
	}

	qWarning() << "Failed to create node type:" << nodeType;
	return "";
}

void NodeGraphController::deleteNode(const QString &nodeId)
{
	if (m_sceneGraphManager->deleteNode(nodeId.toStdString())) {
		m_nodePositions.erase(nodeId);
		emit nodeDeleted(nodeId);
		emit isCompiledChanged();
	}
}

bool NodeGraphController::connectPins(const QString &sourceNodeId, const QString &sourcePinId,
				      const QString &targetNodeId, const QString &targetPinId)
{
	bool success = m_sceneGraphManager->connectNodes(sourceNodeId.toStdString(), sourcePinId.toStdString(),
							 targetNodeId.toStdString(), targetPinId.toStdString());
	if (success) {
		emit connectionCreated(sourceNodeId, sourcePinId, targetNodeId, targetPinId);
		emit isCompiledChanged();
	}
	return success;
}

void NodeGraphController::disconnectPins(const QString &targetNodeId, const QString &targetPinId)
{
	auto graph = m_sceneGraphManager->getNodeGraph();
	if (graph) {
		graph->disconnectPins(targetNodeId.toStdString(), targetPinId.toStdString());
		emit connectionDeleted(targetNodeId, targetPinId);
		emit isCompiledChanged();
	}
}

void NodeGraphController::compileAndRun()
{
	auto graph = m_sceneGraphManager->getNodeGraph();
	if (!graph)
		return;

	bool success = graph->compile();
	emit isCompiledChanged();

	if (success) {
		// Execute one step via Manager
		m_sceneGraphManager->update(0.016); // Simulate 60fps frame
		emit executionFinished(true, "Graph executed successfully");
	} else {
		emit executionFinished(false, "Compilation failed (Cycle or Type mismatch)");
	}
}

void NodeGraphController::setNodeProperty(const QString &nodeId, const QString &propertyName, const QVariant &value)
{
	auto graph = m_sceneGraphManager->getNodeGraph();
	if (!graph) {
		qWarning() << "NodeGraphController::setNodeProperty: No graph available";
		return;
	}

	auto node = graph->getNode(nodeId.toStdString());
	if (!node) {
		qWarning() << "NodeGraphController::setNodeProperty: Node not found:" << nodeId;
		return;
	}

	// Currently, backend nodes expose setters for specific properties
	// We need to cast to specific node types to call their setters
	// This is a simplified approach; a more robust solution would use reflection/metadata

	std::string nodeType = node->getNodeType();
	std::string propName = propertyName.toStdString();

	// Handle ThreeDModelNode
	if (nodeType == "ThreeDModelNode") {
		auto threeDNode = std::dynamic_pointer_cast<SceneGraph::ThreeDModelNode>(node);
		if (threeDNode) {
			if (propName == "modelPath") {
				threeDNode->setModelPath(value.toString().toStdString());
				qDebug() << "Set modelPath on node" << nodeId << "to" << value.toString();
				return;
			}
		}
	}

	// Handle CameraNode
	if (nodeType == "CameraNode") {
		auto cameraNode = std::dynamic_pointer_cast<SceneGraph::CameraNode>(node);
		if (cameraNode) {
			if (propName == "deviceId") {
				cameraNode->setDeviceId(value.toString().toStdString());
				qDebug() << "Set deviceId on node" << nodeId << "to" << value.toString();
				return;
			}
		}
	}

	// Handle VideoNode
	if (nodeType == "VideoNode") {
		auto videoNode = std::dynamic_pointer_cast<SceneGraph::VideoNode>(node);
		if (videoNode) {
			if (propName == "videoPath") {
				videoNode->setVideoPath(value.toString().toStdString());
				qDebug() << "Set videoPath on node" << nodeId << "to" << value.toString();
				return;
			} else if (propName == "loop") {
				videoNode->setLoop(value.toBool());
				qDebug() << "Set loop on node" << nodeId << "to" << value.toBool();
				return;
			}
		}
	}

	// Handle ImageNode
	if (nodeType == "ImageNode") {
		auto imageNode = std::dynamic_pointer_cast<SceneGraph::ImageNode>(node);
		if (imageNode) {
			if (propName == "imagePath") {
				imageNode->setImagePath(value.toString().toStdString());
				qDebug() << "Set imagePath on node" << nodeId << "to" << value.toString();
				return;
			} else if (propName == "filterMode") {
				imageNode->setFilterMode(value.toString().toStdString());
				qDebug() << "Set filterMode on node" << nodeId << "to" << value.toString();
				return;
			}
		}
	}

	// Handle AudioNode
	if (nodeType == "AudioNode") {
		auto audioNode = std::dynamic_pointer_cast<SceneGraph::AudioNode>(node);
		if (audioNode) {
			if (propName == "audioPath") {
				audioNode->setAudioPath(value.toString().toStdString());
				qDebug() << "Set audioPath on node" << nodeId << "to" << value.toString();
				return;
			} else if (propName == "loop") {
				audioNode->setLoop(value.toBool());
				qDebug() << "Set loop on node" << nodeId << "to" << value.toBool();
				return;
			} else if (propName == "volume") {
				audioNode->setVolume(value.toFloat());
				qDebug() << "Set volume on node" << nodeId << "to" << value.toFloat();
				return;
			}
		}
	}

	// Handle FontNode
	if (nodeType == "FontNode") {
		auto fontNode = std::dynamic_pointer_cast<SceneGraph::FontNode>(node);
		if (fontNode && propName == "fontPath") {
			fontNode->setFontPath(value.toString().toStdString());
			qDebug() << "Set fontPath on node" << nodeId;
			return;
		}
	}

	// Handle ShaderNode
	if (nodeType == "ShaderNode") {
		auto shaderNode = std::dynamic_pointer_cast<SceneGraph::ShaderNode>(node);
		if (shaderNode && propName == "shaderPath") {
			shaderNode->setShaderPath(value.toString().toStdString());
			qDebug() << "Set shaderPath on node" << nodeId;
			return;
		}
	}

	// Handle TextureNode
	if (nodeType == "TextureNode") {
		auto textureNode = std::dynamic_pointer_cast<SceneGraph::TextureNode>(node);
		if (textureNode && propName == "texturePath") {
			textureNode->setTexturePath(value.toString().toStdString());
			qDebug() << "Set texturePath on node" << nodeId;
			return;
		}
	}

	// Handle EffectNode
	if (nodeType == "EffectNode") {
		auto effectNode = std::dynamic_pointer_cast<SceneGraph::EffectNode>(node);
		if (effectNode && propName == "effectPath") {
			effectNode->setEffectPath(value.toString().toStdString());
			qDebug() << "Set effectPath on node" << nodeId;
			return;
		}
	}

	// Handle ScriptNode
	if (nodeType == "ScriptNode") {
		auto scriptNode = std::dynamic_pointer_cast<SceneGraph::ScriptNode>(node);
		if (scriptNode) {
			if (propName == "scriptPath") {
				scriptNode->setScriptPath(value.toString().toStdString());
				qDebug() << "Set scriptPath on node" << nodeId;
				return;
			} else if (propName == "scriptLanguage") {
				scriptNode->setScriptLanguage(value.toString().toStdString());
				qDebug() << "Set scriptLanguage on node" << nodeId;
				return;
			}
		}
	}

	// Handle MLNode (generic ML/AI models - ONNX, TensorFlow, PyTorch, etc.)
	if (nodeType == "MLNode") {
		auto mlNode = std::dynamic_pointer_cast<SceneGraph::MLNode>(node);
		if (mlNode && propName == "modelPath") {
			mlNode->setModelPath(value.toString().toStdString());
			qDebug() << "Set modelPath on node" << nodeId;
			return;
		}
	}

	// Handle LLMNode (generic LLM - Gemini, GPT, Claude, etc.)
	if (nodeType == "LLMNode") {
		auto llmNode = std::dynamic_pointer_cast<SceneGraph::LLMNode>(node);
		if (llmNode) {
			if (propName == "prompt") {
				llmNode->setPrompt(value.toString().toStdString());
				qDebug() << "Set prompt on node" << nodeId;
				return;
			} else if (propName == "model") {
				llmNode->setModel(value.toString().toStdString());
				qDebug() << "Set model on node" << nodeId;
				return;
			}
		}
	}

	// TODO: Add other node types as they're implemented
	// For a more scalable solution, consider:
	// 1. Adding a virtual setProperty(name, value) method to IExecutableNode
	// 2. Using a property metadata system
	// 3. Implementing a generic attribute map in BaseNodeBackend

	qWarning() << "NodeGraphController::setNodeProperty: Property" << propertyName << "not supported for node type"
		   << QString::fromStdString(nodeType);
}

bool NodeGraphController::isCompiled() const
{
	return m_backendGraph->isCompiled();
}

QVector3D NodeGraphController::getNodePosition(const QString &nodeId) const
{
	auto it = m_nodePositions.find(nodeId);
	if (it != m_nodePositions.end()) {
		return it->second;
	}
	return QVector3D(0, 0, 0);
}

void NodeGraphController::updateNodePosition(const QString &nodeId, float x, float y, float z)
{
	m_nodePositions[nodeId] = QVector3D(x, y, z);
}

void NodeGraphController::clearPositions()
{
	m_nodePositions.clear();
}

bool NodeGraphController::saveGraph(const QString &filePath)
{
	if (!m_backendGraph)
		return false;

	QJsonObject root;
	QJsonArray nodesArray;
	QJsonArray connectionsArray;

	// Serialize Nodes
	auto nodeIds = m_backendGraph->getNodeIds();
	for (const auto &id : nodeIds) {
		auto node = m_backendGraph->getNode(id);
		if (!node)
			continue;

		QJsonObject nodeObj;
		nodeObj["id"] = QString::fromStdString(node->getNodeId());
		nodeObj["type"] = QString::fromStdString(node->getNodeType());

		// Position (including Z for VR)
		QVector3D pos = getNodePosition(QString::fromStdString(id));
		nodeObj["x"] = pos.x();
		nodeObj["y"] = pos.y();
		nodeObj["z"] = pos.z();

		// TODO: Serialize node properties via IExecutableNode if generic access exists
		// For now, relies on reconstructing default + manual reconfiguration if properties aren't in generic map

		nodesArray.append(nodeObj);
	}

	// Serialize Connections
	auto connections = m_backendGraph->getConnections();
	for (const auto &conn : connections) {
		QJsonObject connObj;
		connObj["sourceNode"] = QString::fromStdString(conn.sourceNodeId);
		connObj["sourcePin"] = QString::fromStdString(conn.sourcePinId);
		connObj["targetNode"] = QString::fromStdString(conn.targetNodeId);
		connObj["targetPin"] = QString::fromStdString(conn.targetPinId);
		connectionsArray.append(connObj);
	}

	root["nodes"] = nodesArray;
	root["connections"] = connectionsArray;

	QFile file(filePath);
	if (!file.open(QIODevice::WriteOnly)) {
		qWarning() << "Failed to open file for saving:" << filePath;
		return false;
	}

	QJsonDocument doc(root);
	file.write(doc.toJson());
	return true;
}

bool NodeGraphController::loadGraph(const QString &filePath)
{
	if (!m_backendGraph)
		return false;

	QFile file(filePath);
	if (!file.open(QIODevice::ReadOnly)) {
		qWarning() << "Failed to open file for loading:" << filePath;
		return false;
	}

	QJsonDocument doc = QJsonDocument::fromJson(file.readAll());
	if (doc.isNull())
		return false;

	QJsonObject root = doc.object();

	// Clear existing graph
	auto existingIds = m_backendGraph->getNodeIds();
	for (const auto &id : existingIds) {
		m_backendGraph->removeNode(id);
	}
	clearPositions();
	emit nodeDeleted("ALL"); // Simplified signal to clear UI (BlueprintCanvas needs to handle "ALL")

	// Load Nodes
	QJsonArray nodesArray = root["nodes"].toArray();
	for (const auto &val : nodesArray) {
		QJsonObject nodeObj = val.toObject();
		QString id = nodeObj["id"].toString();
		QString type = nodeObj["type"].toString();
		float x = nodeObj["x"].toDouble();
		float y = nodeObj["y"].toDouble();
		float z = nodeObj["z"].toDouble();

		// Create node using factory
		auto node = SceneGraph::NodeFactory::create(type.toStdString(), id.toStdString());
		if (node) {
			m_backendGraph->addNode(node);
			updateNodePosition(id, x, y, z);
			emit nodeCreated(id, type, x, y, z);
		} else {
			qWarning() << "Failed to create node" << type << "during load.";
		}
	}

	// Load Connections
	QJsonArray connectionsArray = root["connections"].toArray();
	for (const auto &val : connectionsArray) {
		QJsonObject connObj = val.toObject();
		QString srcNode = connObj["sourceNode"].toString();
		QString srcPin = connObj["sourcePin"].toString();
		QString dstNode = connObj["targetNode"].toString();
		QString dstPin = connObj["targetPin"].toString();

		if (m_backendGraph->connectPins(srcNode.toStdString(), srcPin.toStdString(), dstNode.toStdString(),
						dstPin.toStdString())) {
			emit connectionCreated(srcNode, srcPin, dstNode, dstPin);
		}
	}

	return true;
}

} // namespace UI
} // namespace NeuralStudio
