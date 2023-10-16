#ifndef MYSQL_H
#define MYSQL_H

#include <string>
#include <mysql/mysql.h>
#include <muduo/base/Logging.h>

#define SERVER "127.0.0.1"
#define USER "bai"
#define PASSWORD "123456"
#define DBNAME "chat"

using namespace std;

class MySQL
{
public:
    MySQL()
    {
        conn_ = mysql_init(nullptr);
    }

    ~MySQL()
    {
        if (conn_ != nullptr)
        {
            mysql_close(conn_);
        }
    }

    bool connect()
    {
        MYSQL *p = mysql_real_connect(conn_, SERVER, USER, PASSWORD, DBNAME, 3306, nullptr, 0);

        if (p != nullptr)
        {

            // 代码支持中文， 默认是ASCII 码
            mysql_query(conn_, "set names gbk");
        }
        else
        {
            LOG_INFO << "connect mysql fail!";
            return false;
        }
        return true;
    }

    bool update(string sql)
    {
        if (mysql_query(conn_, sql.c_str()))
        {
            LOG_INFO << __FILE__ << " : " << __LINE__ << sql << " update Error!";
            return false;
        }
        return true;
    }

    MYSQL_RES *query(string sql)
    {
        if (mysql_query(conn_, sql.c_str()))
        {
            LOG_INFO << __FILE__ << " : " << __LINE__ << sql << " select Error!";
            return nullptr;
        }
        return mysql_use_result(conn_);
    }

    MYSQL *get_Connection()
    {
        return this->conn_;
    }

private:
    MYSQL *conn_;
};

#endif