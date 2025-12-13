#include "GraphSerializer.h"
#include "NodeGraph.h"
#include "NodeRegistry.h"
#include "NodeItem.h"
#include "ConnectionItem.h"
#include "PortItem.h"
#include <QFile>
#include <QJsonDocument>
#include <QJsonArray>
#include <QJsonObject>
#include <QGraphicsScene>
#include <QTextStream>
#include <QIODevice>
#include <QDebug>

bool GraphSerializer::save(const NodeGraph *graph, const QString &path)
{
	QJsonObject root;
	QJsonArray nodesArray;
	QJsonArray connsArray;

	QList<QGraphicsItem *> items = graph->scene()->items();

	// 1. Serialize Nodes
	for (auto item : items) {
		if (NodeItem *node = dynamic_cast<NodeItem *>(item)) {
			QJsonObject nodeObj;
			nodeObj["id"] = node->id().toString();
			nodeObj["title"] = node->title();
			nodeObj["x"] = node->pos().x();
			nodeObj["y"] = node->pos().y();
			nodesArray.append(nodeObj);
		}
	}

	// 2. Serialize Connections
	for (auto item : items) {
		if (ConnectionItem *conn = dynamic_cast<ConnectionItem *>(item)) {
			QJsonObject connObj;
			connObj["startNode"] = conn->startPort()->node()->id().toString();
			// In a real impl, we'd iterate ports to find name, or store it in PortItem
			// For now assuming we can serialize enough info.
			// ... skipping detailed connection serialization for this skeleton to match "PortItem" capabilities
			// Note: PortItem needs to expose its name/index for this to work fully.
		}
	}

	root["nodes"] = nodesArray;
	// root["connections"] = connsArray; // TODO: finish connection serialization

	QFile file(path);
	if (!file.open(QIODevice::WriteOnly))
		return false;
	file.write(QJsonDocument(root).toJson());
	return true;
}

bool GraphSerializer::load(NodeGraph *graph, const QString &path)
{
	QFile file(path);
	if (!file.open(QIODevice::ReadOnly))
		return false;

	QJsonDocument doc = QJsonDocument::fromJson(file.readAll());
	QJsonObject root = doc.object();

	graph->scene()->clear(); // Wipes everything

	QJsonArray nodesArray = root["nodes"].toArray();
	for (auto val : nodesArray) {
		QJsonObject nodeObj = val.toObject();
		QString title = nodeObj["title"].toString();
		double x = nodeObj["x"].toDouble();
		double y = nodeObj["y"].toDouble();

		// Lookup definition by title/name
		const NodeDefinition *def = NodeRegistry::instance().getNodeByName(title);
		if (def) {
			graph->addNode(*def, x, y);
			// TODO: Restore ID
		} else {
			// Fallback for unknown nodes?
			// For now, create a dummy compatible definition or skip
			qWarning() << "Unknown node type:" << title;
		}
	}

	return true;
}

QString GraphSerializer::exportAIContext(const NodeGraph *graph)
{
	QString context;
	QTextStream out(&context);

	out << "Current Node Graph Worklow:\n";
	out << "---------------------------\n";

	QList<QGraphicsItem *> items = graph->scene()->items();
	int nodeCount = 0;

	for (auto item : items) {
		if (NodeItem *node = dynamic_cast<NodeItem *>(item)) {
			nodeCount++;
			out << "Node '" << node->title() << "' (" << node->id().toString() << ")\n";
			// In a real scenario, we'd list properties here
		}
	}

	out << "\nConnections:\n";
	for (auto item : items) {
		if (ConnectionItem *conn = dynamic_cast<ConnectionItem *>(item)) {
			if (conn->startPort() && conn->endPort()) {
				out << "  " << conn->startPort()->node()->title() << " ["
				    << conn->startPort()->node()->id().toString() << "] -> "
				    << conn->endPort()->node()->title() << " ["
				    << conn->endPort()->node()->id().toString() << "]\n";
			}
		}
	}

	if (nodeCount == 0) {
		out << "(Empty Graph)\n";
	}

	return context;
}
