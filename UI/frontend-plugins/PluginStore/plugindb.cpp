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
    if (QSqlDatabase::isDriverAvailable(__DEF_SQL_DRIVER_NAME__))
    {
        if (m_database.isOpen())
        {
            CloseDB();
        }
        m_database.setDatabaseName(__DEF_SQL_DATABASE_NAME__);
        m_database.setUserName("root");
        m_database.setPassword("123456");

        bRet = OpenDB();
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
    return m_database.open();
}

void PluginDB::CloseDB()
{
    m_database.close();
}
