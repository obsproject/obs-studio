#ifndef PLUGINDB_H
#define PLUGINDB_H
#include <QtSql/QSqlDatabase>

#define __DEF_SQL_DRIVER_NAME__     "QSQLITE"
#define __DEF_SQL_DATABASE_NAME__   "c:\\plugin_database.db"

class PluginDB
{
public:
    PluginDB();
    ~PluginDB();

    bool InitPluginDB();
    bool OpenDB();
    void CloseDB();

private:
    QSqlDatabase m_database;
};
#endif
