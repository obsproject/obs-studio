#pragma once
#include <QObject>
#include <QString>
#include <QJsonObject>
#include <QJsonArray>
#include <QJsonDocument>

class NodeGraph;

class GraphSerializer
{
      public:
    static bool save(const NodeGraph *graph, const QString &path);
    static bool load(NodeGraph *graph, const QString &path);
    static QString exportAIContext(const NodeGraph *graph);
};
