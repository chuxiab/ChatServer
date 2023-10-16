#ifndef FRIENDMODEL_H
#define FRIENDMODEL_H

#include <vector>
#include "User.hpp"
#include "MySQL.hpp"

class FriendModel
{
public:
    bool insert(int user_id, int friend_id);

    std::vector<User> query(int user_id);
};

#endif