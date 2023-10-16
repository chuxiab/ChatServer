#ifndef GROUPMODEL_H
#define GROUPMODEL_H

#include <string>
#include <vector>
#include "Group.hpp"

using namespace std;

class GroupModel
{
public:
    // 创建群组
    bool create_group(Group &group);

    // 加入群组
    bool add_group(int user_id, int group_id, string desc);

    // 查询用户所在群组信息
    vector<Group> query_group(int user_id);

    // 根据指定的groupid给除自己外群发消息, 获取其它人的userid
    vector<int> query_group_users(int user_id, int group_id);
};

#endif