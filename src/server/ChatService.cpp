#include "ChatService.hpp"

using namespace std;
using namespace placeholders;

ChatService *ChatService::instance()
{
    static ChatService service;
    return &service;
}

ChatService::ChatService()
{
    msg_handler_map_.insert({MsgType::LOGIN_MSG, bind(&ChatService::login, this, _1, _2, _3)});
    msg_handler_map_.insert({MsgType::REG_MSG, bind(&ChatService::regist, this, _1, _2, _3)});
    msg_handler_map_.insert({MsgType::ONE_CHAT_MSG, bind(&ChatService::one_chat, this, _1, _2, _3)});
    msg_handler_map_.insert({MsgType::ADD_FRIEND, bind(&ChatService::add_friend, this, _1, _2, _3)});
    msg_handler_map_.insert({MsgType::LOGINOUT_MSG, bind(&ChatService::login_out, this, _1, _2, _3)});
    msg_handler_map_.insert({MsgType::GROUP_CREATE, bind(&ChatService::create_group, this, _1, _2, _3)});
    msg_handler_map_.insert({MsgType::GROUP_ADD, bind(&ChatService::add_group, this, _1, _2, _3)});

    if (redis_.connect())
    {
        redis_.init_notify_handler(bind(&ChatService::redis_subscribe_message_handler, this, _1, _2));
    }
}

void ChatService::login(const TcpConnectionPtr &conn, json &js, Timestamp time)
{
    int id = js["id"].get<int>();
    string password = js["password"];

    User user = userModel_.query(id);

    json response;
    response["id"] = id;
    response["name"] = user.get_name();
    if (user.get_id() == id && user.get_password() == password)
    {
        if (user.get_state() == "online")
        {
            response["msgid"] = MsgType::LOGIN_MSG_ACK;
            response["errno"] = 2;

            response["errmsg"] = "already online";
        }
        else
        {
            {
                lock_guard<mutex> lock(conn_mutex_);
                user_conn_map_[user.get_id()] = conn;
            }

            redis_.subscribe(id);
            user.set_state("online");
            userModel_.update_state(user);

            response["msgid"] = MsgType::LOGIN_MSG_ACK;
            response["errno"] = 0;
            response["msg"] = "login success";

            vector<string> vec = offlineMessageModel_.query(id);
            if (!vec.empty())
            {
                response["offlineMessage"] = vec;
                offlineMessageModel_.remove(id);
            }
            // 查询该用户的好友信息并返回
            vector<User> friend_vec = friendModel_.query(id);

            vector<string> friend_info;
            for (auto &t : friend_vec)
            {
                json tmp;
                tmp["id"] = t.get_id();
                tmp["name"] = t.get_name();
                tmp["state"] = t.get_state();
                friend_info.push_back(tmp.dump());
            }
            response["friends"] = friend_info;

            // 查询用户的群组信息

            vector<Group> group_vec = groupModel_.query_group(id);
            if (!group_vec.empty())
            {
                vector<string> gvec_;
                for (auto &t : group_vec)
                {
                    json tmp;
                    tmp["groupid"] = t.get_id();
                    tmp["name"] = t.get_name();
                    tmp["desc"] = t.get_desc();

                    vector<string> user_vec;
                    for (auto &temp : t.get_vec())
                    {
                        json js_tmp;
                        js_tmp["id"] = temp.get_id();
                        js_tmp["name"] = temp.get_name();
                        js_tmp["state"] = temp.get_state();
                        js_tmp["role"] = temp.get_role();
                        user_vec.push_back(js_tmp.dump());
                    }
                    tmp["users"] = user_vec;
                    cout << tmp.dump() << endl;
                    gvec_.push_back(tmp.dump());
                }
                response["groups"] = gvec_;
            }
        }
    }
    else
    {
        response["msgid"] = MsgType::LOGIN_MSG_ACK;
        response["errno"] = 1;
        response["errmsg"] = "id or password error";
    }
    cout << response.dump() << endl;
    conn->send(response.dump());
}

void ChatService::regist(const TcpConnectionPtr &conn, json &js, Timestamp time)
{
    string name = js["name"];
    string password = js["password"];

    User user;
    user.set_name(name);
    user.set_password(password);

    bool flag = userModel_.insert(user);

    json response;
    response["msgid"] = MsgType::REG_MSG_ACK;

    if (flag)
    {
        response["errno"] = 0;
        response["id"] = user.get_id();
    }
    else
    {
        response["errno"] = 1;
    }

    conn->send(response.dump());
}

void ChatService::one_chat(const TcpConnectionPtr &conn_, json &js, Timestamp time)
{
    int recv_id = js["to"].get<int>();

    {
        lock_guard<mutex> lock(conn_mutex_);
        auto it = user_conn_map_.find(recv_id);

        if (it != user_conn_map_.end())
        {
            it->second->send(js.dump());
            return;
        }
    }

    User user = userModel_.query(recv_id);
    if (user.get_state() == "online")
    {
        redis_.publish(recv_id, js.dump());
    }
    else
    {
        offlineMessageModel_.insert(recv_id, js.dump());
    }
}

void ChatService::client_close_exception(const TcpConnectionPtr &conn_)
{
    User user;
    {
        lock_guard<mutex> lock(conn_mutex_);
        for (auto it = user_conn_map_.begin(); it != user_conn_map_.end(); ++it)
        {
            if (it->second == conn_)
            {
                user.set_id(it->first);
                user_conn_map_.erase(it);
                break;
            }
        }
    }

    if (user.get_id() != -1)
    {
        user.set_state("offline");
        userModel_.update_state(user);
    }
}

void ChatService::add_friend(const TcpConnectionPtr &conn, json &js, Timestamp time)
{
    int id = js["id"].get<int>();
    int friend_id = js["friendid"].get<int>();

    friendModel_.insert(id, friend_id);
}

string ChatService::operate_group_message(int num)
{
    json js;
    js["errno"] = num;
    if (num == 0)
    {
        js["msg"] = "create group success";
    }
    else if (num == 3)
    {
        js["errormsg"] = "add group error";
    }
    else if (num == 4)
    {
        js["errormsg"] = "create group error";
    }
    return js.dump();
}

void ChatService::create_group(const TcpConnectionPtr &conn, json &js, Timestamp time)
{
    int user_id = js["id"].get<int>();
    string group_name = js["groupname"];
    string desc = js["desc"];

    Group group(-1, group_name, desc);
    string reval;
    if (groupModel_.create_group(group))
    {
        if (groupModel_.add_group(user_id, group.get_id(), "creator"))
        {
            reval = operate_group_message(0);
        }
        {
            reval = operate_group_message(3);
        }
    }
    else
    {
        reval = operate_group_message(4);
    }
    conn->send(reval);
}

void ChatService::add_group(const TcpConnectionPtr &conn, json &js, Timestamp time)
{
    int user_id = js["id"].get<int>();
    int group_id = js["groupid"].get<int>();
    string desc = js["desc"];

    if (groupModel_.add_group(user_id, group_id, "normal"))
    {
        conn->send(operate_group_message(0));
    }
    conn->send(operate_group_message(3));
}

void ChatService::group_chat(const TcpConnectionPtr &conn, json &js, Timestamp time)
{
    int user_id = js["id"].get<int>();
    int group_id = js["groupid"].get<int>();

    vector<int> userid_vec = groupModel_.query_group_users(user_id, group_id);

    lock_guard<mutex> lock(conn_mutex_);
    for (int id : userid_vec)
    {
        auto it = user_conn_map_.find(id);
        if (it != user_conn_map_.end())
        {
            it->second->send(js.dump());
        }
        else
        {
            // 离线消息或在另一个服务器上
            User user = userModel_.query(id);
            if (user.get_state() == "online")
            {
                redis_.publish(id, js.dump());
            }
            else
            {
                offlineMessageModel_.insert(id, js.dump());
            }
        }
    }
}

void ChatService::login_out(const TcpConnectionPtr &conn, json &js, Timestamp time)
{
    int id = js["id"].get<int>();
    {
        lock_guard<mutex> lock(conn_mutex_);
        auto it = user_conn_map_.find(id);
        if (it != user_conn_map_.end())
        {
            user_conn_map_.erase(it);
        }
    }

    // 取消redis 订阅
    redis_.unsubscribe(id);
    // 更新用户状态信息
    User user(id, "", "", "offline");
    userModel_.update_state(user);
}

void ChatService::reset()
{
    userModel_.reset_state();
}

void ChatService::redis_subscribe_message_handler(int channel, string message)
{
    lock_guard<mutex> locker(conn_mutex_);
    auto it = user_conn_map_.find(channel);

    if (it != user_conn_map_.end())
    {
        it->second->send(message);
        return;
    }
    offlineMessageModel_.insert(channel, message);
}
