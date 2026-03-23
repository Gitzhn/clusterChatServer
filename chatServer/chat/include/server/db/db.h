#ifndef DB_H
#define DB_H

#include <mysql/mysql.h>
#include <string>

using namespace std;

// 数据库：chatdb

class MySQL{
public:
    MySQL();

    ~MySQL();

    bool connect();

    // 更新操作（delete,update）
    bool update(string sql);

    // 查询操作（select）
    MYSQL_RES* query(string sql);

    // 获取连接
    MYSQL* getConnection();
private:
    MYSQL* _conn;
};


#endif