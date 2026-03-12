#include "db.h"
#include <muduo/base/Logging.h>


static string server = "127.0.0.1";
static string user = "root";
static string password = "279375";
static string dbname = "chatdb";


MySQL::MySQL(){
    // 开辟一块存储连接数据的空间
    _conn = mysql_init(nullptr);
}

MySQL::~MySQL(){
    if(_conn != nullptr){
        mysql_close(_conn);
    }
}

bool MySQL::connect(){
    MYSQL* p = mysql_real_connect(_conn,server.c_str(),user.c_str(),password.c_str(),dbname.c_str(),3306,nullptr,0);
    
    if(p!=nullptr){
        // c和c++代码默认编码字符是ASCII，从mysql拉取的中文会乱码，因此要指定编码字符
        mysql_query(_conn,"set name gbk");
        LOG_INFO<<"connect mysql success!";
    }else{
        LOG_INFO<<"connect mysql failed!";
    }

    return p;
}

// 更新操作（delete,update）
bool MySQL::update(string sql){
    if(mysql_query(_conn,sql.c_str())){
        LOG_INFO<<__FILE__<<":"<<__LINE__<<":"<<sql<<"更新失败！";
        return false;
    }
    return true;
}

// 查询操作（select）
MYSQL_RES* MySQL::query(string sql){
    if(mysql_query(_conn,sql.c_str())){
        LOG_INFO<<__FILE__<<":"<<__LINE__<<":"<<sql<<"查询失败！";
        return nullptr;
    }
    return mysql_use_result(_conn);
}

// 获取连接
MYSQL* MySQL::getConnection(){
    return _conn;
}