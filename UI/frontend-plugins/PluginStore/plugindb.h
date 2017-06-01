#ifndef PLUGINDB_H
#define PLUGINDB_H
#include <QSqlDatabase>
#include <QSqlError>
#include <QSqlQuery>
#include <QFile>
#include <QDebug>

#define __DEF_SQL_DRIVER_TYPE__     "QSQLITE"
#define __DEF_SQL_DATABASE_NAME__   "plugin_database.db"

#define __DEF_CFG_DOWN_PATH_NAME__  "download_path"

class PluginDB
{
public:
    enum PluginState{
        DOWNSTART,
        DOWNING,
        DOWNPAUSED,
        CANINSTALL,
        INSTALLED
    };

    struct PluginInfo
    {
        qint64      plugin_id;
        QString     file_name;
        QString     local_path;
        qint64      down_size;
        qint64      file_size;
        QString     json_data;
        PluginState state;
    };

public:
    PluginDB();
    ~PluginDB();

    bool InitPluginDB();
    bool OpenDB();
    void CloseDB();
    bool CreatePluginTable();
    bool CreateConfigTable();
    bool SetConfigValue(QString qstrName,QString qstrValue);
    QString GetConfigValue(QString qstrName);
    bool IsExistTable(QString qstrTabName);
    bool InsertPluginData(PluginInfo obj);
    bool DeletePluginData(qint64 qiPluginId);
    bool UpdatePluginData(qint64 qiPluginId, PluginInfo obj);
    int  GetPluginDataCount(qint64 qiPluginId = -1);
    QList<PluginInfo> QueryPluginData(qint64 qiPluginId = -1);
private:
    QSqlDatabase m_database;
};
#endif
