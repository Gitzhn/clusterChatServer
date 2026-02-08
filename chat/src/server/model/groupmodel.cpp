#include "groupmodel.hpp"
#include "db.h"

// 创建群组
bool GroupModel::createGroup(Group& group){
    // 组装sql语句
    char sql[1024] = {0};
    // 通过userid找到User的详细信息
    sprintf(sql,"insert into AllGroup (groupname,groupdesc) values ('%s', '%s')",group.getName().c_str(),group.getDesc().c_str());

    MySQL mysql;
    if(mysql.connect()){
        if(mysql.update(sql)){
            group.setId(mysql_insert_id(mysql.getConnection()));
            return true;
        }
    }
    return false;
}

// 加入群组（使用显式列名；已存在则忽略，避免重复加入报错）
void GroupModel::addGroup(int userid,int groupid,string role){
    // 组装sql语句
    char sql[1024] = {0};
    // 表结构为 (groupid, userid, grouprole)，INSERT IGNORE 避免重复加入同一群组时报错
    sprintf(sql,"insert ignore into GroupUser (groupid, userid, grouprole) values (%d, %d, '%s')",groupid,userid,role.c_str());

    MySQL mysql;
    if(mysql.connect()){
        mysql.update(sql);
    }
}

// 查询用户所在群组信息
vector<Group> GroupModel::queryGroups(int userid){
    // 组装sql语句
    char sql[1024] = {0};
    // 通过userid找到User的详细信息
    sprintf(sql,"select a.id,a.groupname,a.groupdesc from AllGroup as a inner join GroupUser as b on a.id = b.groupid where b.userid = %d",userid);

    vector<Group> groupVec;
    MySQL mysql;
    if(mysql.connect()){
        MYSQL_RES* res=mysql.query(sql);
        if(res!=nullptr){
            MYSQL_ROW row;
            // 查出userid所有得到群组信息
            while((row=mysql_fetch_row(res))!=nullptr){
                Group group;
                group.setId(atoi(row[0]));
                group.setName(row[1]);
                group.setDesc(row[2]);
                groupVec.emplace_back(group);
            }
            mysql_free_result(res);
        }
    }

    // 查询群组的用户信息
    for(Group& group : groupVec){
        sprintf(sql,"select a.id,a.name,a.state,b.grouprole from User as a inner join GroupUser as b on b.userid = a.id where b.groupid = %d",group.getId());

        MYSQL_RES* res=mysql.query(sql);
        if(nullptr!=res){
            MYSQL_ROW row;
            while((row=mysql_fetch_row(res))!=nullptr){
                GroupUser user;
                user.setId(atoi(row[0]));
                user.setName(row[1]);
                user.setState(row[2]);
                user.setRole(row[3]);
                group.getUsers().emplace_back(user);
            }
            mysql_free_result(res);
        }
    }
    return groupVec;
}

// 根据指定的groupid查询群组用户id列表，并且不给自己发，主要用于群发消息
vector<int> GroupModel::queryGroupUsers(int userid,int groupid){
    char sql[1024]={0};
    sprintf(sql,"select userid from GroupUser where groupid = %d and userid != %d",groupid,userid);

    vector<int> idVec;
    MySQL mysql;
    if(mysql.connect()){
        MYSQL_RES* res=mysql.query(sql);
        if(nullptr!=res){
            MYSQL_ROW row;
            while((row=mysql_fetch_row(res))!=nullptr){
                idVec.emplace_back(atoi(row[0]));
            }
            mysql_free_result(res);
        }
    }
    return idVec;
}