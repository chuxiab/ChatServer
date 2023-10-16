#ifndef OFFLINEMESSAGE_H
#define OFFLINEMESSAGE_H

#include <string>
#include <vector>

using namespace std;

class OfflineMessageModel
{
public:
    // 存储用户离线消息
    bool insert(int id, string msg);

    // 删除用户的离线消息
    bool remove(int id);

    // 查询用户的聊天消息
    vector<std::string> query(int id);
};

#endif