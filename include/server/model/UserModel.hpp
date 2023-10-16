#ifndef USER_MODEL_H
#define USER_MODEL_H

#include "User.hpp"

class UserModel
{
public:
    // 增加用户
    bool insert(User &user);

    // 根据id查找用户
    User query(int id);

    // 更新状态
    User update_state(User user);

    // 重置用户状态信息
    void reset_state();
};

#endif