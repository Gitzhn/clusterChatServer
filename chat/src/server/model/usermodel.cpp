#include "usermodel.hpp"
#include "db.h"
#include <iostream>

using namespace std;

// User表的增加方法
bool UserModel::insert(User& user){
    // 组装sql语句
    char sql[1024] = {0};
    sprintf(sql,"insert into User(name,password,state) values ('%s','%s','%s')",
            user.getName().c_str(),user.getPwd().c_str(),user.getState().c_str());

    MySQL mysql;

    // 连接数据库
    if(mysql.connect()){
        // 更新数据
        if(mysql.update(sql)){
            // 获取插入成功的用户的主键id
            user.setId(mysql_insert_id(mysql.getConnection()));
            return true;
        }
    }

    return false;
}

// 根据用户号码查询用户信息
User UserModel::query(int id){
    // 组装sql语句
    char sql[1024] = {0};
    sprintf(sql,"select * from User where id = %d",id);

    MySQL mysql;

    // 连接数据库
    if(mysql.connect()){
        MYSQL_RES* res=mysql.query(sql);
        if(nullptr!=res){
            MYSQL_ROW row=mysql_fetch_row(res);
            if(nullptr!=row){
                User user;
                user.setId(atoi(row[0]));
                user.setName(row[1]);
                user.setPwd(row[2]);
                user.setState(row[3]);
                // 释放指针资源
                mysql_free_result(res);
                return user;
            }
        }
    }
    return User(); // id默认为-1
}


// 更新用户的状态信息
bool UserModel::updateState(User user){
    // 组装sql语句
    char sql[1024] = {0};
    sprintf(sql,"update User set state = '%s' where id = %d",user.getState().c_str(),user.getId());

    MySQL mysql;

    // 连接数据库
    if(mysql.connect()){
        // 更新数据
        if(mysql.update(sql)){
            return true;
        }
    }

    return false;
}

// 重置用户的状态信息
void UserModel::resetState(){
    // 组装sql语句
    char sql[1024] = "update User set state = 'offline' where state = 'online'";

    MySQL mysql;
    // 连接数据库
    if(mysql.connect()){
        // 更新数据
        mysql.update(sql);
    }
}