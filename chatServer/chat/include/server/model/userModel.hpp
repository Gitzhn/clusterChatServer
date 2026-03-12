#ifndef USERMODEL_H
#define USERMODEL_H

#include "user.hpp"


// user表的数据操作类
class UserModel{
public:
    // User表的增加方法
    bool insert(User& user);

    // 根据用户号码查询用户信息
    User query(int id);

    // 更新用户的状态信息（无条件）
    bool updateState(User user);

    // 仅当当前状态为offline时，将状态更新为online，用于并发登录控制
    bool updateStateOnlineIfOffline(int id);

    // 重置用户的状态信息
    void resetState();
};


#endif