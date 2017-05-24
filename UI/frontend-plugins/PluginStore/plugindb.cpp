#include "pluginDB.h"

PluginDB::PluginDB()
{

}


PluginDB::~PluginDB()
{
}

bool PluginDB::InitPluginDB()
{
    bool bRet = false;
    QString qstrFilePath;
    if (QSqlDatabase::isDriverAvailable(__DEF_SQL_DRIVER_TYPE__))
    {
        qstrFilePath = __DEF_SQL_DATABASE_NAME__;
        m_database = QSqlDatabase::addDatabase(__DEF_SQL_DRIVER_TYPE__);

        if (!QFile::exists(qstrFilePath))
        {
            QFile dbFile(qstrFilePath);
            bRet = dbFile.open(QIODevice::WriteOnly) ? true : false;
            dbFile.close();
        }
        else
            bRet = true;

        if (bRet)
        {
            m_database.setDatabaseName(qstrFilePath);
            bRet = OpenDB();
        }

        if (bRet)
        {
            if (!IsExistTable("PluginInfo"))
            {
                bRet = CreatePluginTable();
            }
            else
                bRet = true;
        }
    }    
    return bRet;
}


bool PluginDB::OpenDB()
{
    bool bRet = false;
    if (m_database.isOpen())
    {
        CloseDB();
    }

    bRet = m_database.open();
    if (!bRet)
    {
        qDebug() << m_database.lastError();
    }

    return bRet;
}

void PluginDB::CloseDB()
{
    m_database.close();
}

bool PluginDB::CreatePluginTable()
{
    bool bRet = false;
    QString qstrSQL("CREATE TABLE PluginInfo(plug_id INTEGER,local_path TEXT,down_size INTEGER,file_size INTEGER,json_data TEXT);");
    if (!qstrSQL.isEmpty())
    {
        QSqlQuery query(m_database);
        bRet = query.exec(qstrSQL);
        if (!bRet)
        {
            qDebug() << query.lastError();
        }
    }
    return bRet;
}

bool PluginDB::IsExistTable(QString qstrTabName)
{
    bool bRet = false;
    QSqlQuery query(m_database);
    QString qstrSQL = QString("SELECT name FROM sqlite_master where type = 'table' and  name = '%1'").arg(qstrTabName);
    query.exec(qstrSQL);
    if (query.next())
    {
        QString qResult = query.value("name").toString();
        if (qResult.compare(qstrTabName));
        {
            bRet = true;
        }
    }
    return bRet;
}

bool PluginDB::InsertPluginData(PluginInfo obj)
{
    bool bRet = false;
    QSqlQuery query(m_database);
    QString qstrSQL = QString("INSERT INTO PluginInfo (plug_id,local_path,down_size,file_size,json_data)"\
                              " VALUES(:plug_id,:local_path,:down_size,:file_size,:json_data);");

    query.prepare(qstrSQL);
    query.bindValue(":plug_id", obj.plugin_id);
    query.bindValue(":local_path", obj.local_path);
    query.bindValue(":down_size", obj.down_size);
    query.bindValue(":file_size", obj.file_size);
    query.bindValue(":json_data", obj.json_data);
    bRet = query.exec();
    if (!bRet)
    {
        qDebug() << query.lastError();
    }

    return bRet;
}

bool PluginDB::DeletePluginData(qint64 qiPluginId)
{
    bool bRet = false;
    QSqlQuery query(m_database);
    QString qstrSQL = QString("DELETE FROM PluginInfo where PluginInfo.plug_id ='%1'").arg(qiPluginId);
    bRet = query.exec();
    if (!bRet)
    {
        qDebug() << query.lastError();
    }
    return bRet;
}

bool PluginDB::UpdatePluginData(qint64 qiPluginId, PluginInfo obj)
{
    bool bRet = false;
    QSqlQuery query(m_database);
    QString qstrSQL = QString("UPDATE PluginInfo SET down_size = :down_size,file_size = :file_size,local_path = :local_path,"\
                                                    "json_data = :json_data WHERE plug_id = :plug_id");
    query.prepare(qstrSQL);
    query.bindValue(":plug_id", qiPluginId);
    query.bindValue(":local_path", obj.local_path);
    query.bindValue(":down_size", obj.down_size);
    query.bindValue(":file_size", obj.file_size);
    query.bindValue(":json_data", obj.json_data);
    bRet = query.exec();
    if (!bRet)
    {
        qDebug() << query.lastError();
    }
    return bRet;
}

QList<PluginDB::PluginInfo> PluginDB::QueryPluginData(qint64 qiPluginId)
{
    QList<PluginInfo> list;
    QSqlQuery query(m_database);
    QString qstrSQL = QString("SELECT * FROM PluginInfo");
    if (qiPluginId != -1)
    {
        qstrSQL.append(" WHERE plug_id = '%1'").arg(qiPluginId);
    }
    query.exec(qstrSQL);
    while (query.next())
    {
        PluginInfo item;
        item.plugin_id = query.value("plug_id").toInt();
        item.local_path = query.value("local_path").toString();
        item.down_size = query.value("down_size").toInt();
        item.file_size = query.value("file_size").toInt();
        item.json_data = query.value("json_data").toString();

        list.append(item);
    }

    return list;
}
