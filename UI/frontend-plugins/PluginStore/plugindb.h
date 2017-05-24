#ifndef PLUGINDB_H
#define PLUGINDB_H
#include <QSqlDatabase>
#include <QSqlError>
#include <QSqlQuery>
#include <QFile>
#include <QDebug>

#define __DEF_SQL_DRIVER_TYPE__     "QSQLITE"
#define __DEF_SQL_DATABASE_NAME__   "plugin_database.db"

class PluginDB
{
public:
    struct PluginInfo
    {
        qint64      plugin_id;
        QString     local_path;
        qint64      down_size;
        qint64      file_size;
        QString     json_data;
    };

public:
    PluginDB();
    ~PluginDB();

    bool InitPluginDB();
    bool OpenDB();
    void CloseDB();
    bool CreatePluginTable();
    bool IsExistTable(QString qstrTabName);
    bool InsertPluginData(PluginInfo obj);
    bool DeletePluginData(qint64 qiPluginId);
    bool UpdatePluginData(qint64 qiPluginId, PluginInfo obj);
    QList<PluginInfo> QueryPluginData(qint64 qiPluginId = -1);
private:
    QSqlDatabase m_database;
};
#endif
