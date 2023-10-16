#ifndef CHATSERVICE_H
#define CHATSERVICE_H

#include <mutex>
#include <functional>
#include <unordered_map>
#include <muduo/net/TcpConnection.h>
#include <muduo/base/Logging.h>
#include "OfflineMessageModel.hpp"
#include "public.hpp"
#include "UserModel.hpp"
#include "json.hpp"
#include "FriendModel.hpp"
#include "GroupModel.hpp"
#include "redis.hpp"

using namespace muduo;
using namespace muduo::net;
using json = nlohmann::json;
using MsgHandler = std::function<void(const TcpConnectionPtr &, json &, Timestamp)>;

class ChatService
{
public:
    // 获取单例对象
    static ChatService *instance();

    void login(const TcpConnectionPtr &, json &, Timestamp);

    void regist(const TcpConnectionPtr &, json &, Timestamp);

    void one_chat(const TcpConnectionPtr &, json &, Timestamp);

    void add_friend(const TcpConnectionPtr &, json &, Timestamp);

    void create_group(const TcpConnectionPtr &, json &, Timestamp);

    void add_group(const TcpConnectionPtr &, json &, Timestamp);

    void group_chat(const TcpConnectionPtr &, json &, Timestamp);

    void login_out(const TcpConnectionPtr &, json &, Timestamp);

    void redis_subscribe_message_handler(int channel, string message);

    string operate_group_message(int num);

    void reset();

    void client_close_exception(const TcpConnectionPtr &conn);

    MsgHandler get_handler(int type)
    {
        auto it = msg_handler_map_.find(static_cast<MsgType>(type));

        if (it == msg_handler_map_.end())
        {
            // 返回日志处理函数
            return [=](const TcpConnectionPtr &conn, json &js, Timestamp time)
            {
                LOG_ERROR << "msg_id: " << type << "can not find handler";
            };
        }
        return msg_handler_map_[static_cast<MsgType>(type)];
    }

private:
    ChatService();

private:
    // 存储事件触发的函数
    std::unordered_map<MsgType, MsgHandler> msg_handler_map_;
    std::unordered_map<int, TcpConnectionPtr> user_conn_map_;
    std::mutex conn_mutex_;

    Redis redis_;

    UserModel userModel_;
    OfflineMessageModel offlineMessageModel_;
    FriendModel friendModel_;
    GroupModel groupModel_;
};

#endif