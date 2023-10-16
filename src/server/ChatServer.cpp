#include <functional>
#include "ChatServer.hpp"
#include "ChatService.hpp"
#include "json.hpp"

using namespace std;
using namespace placeholders;
using json = nlohmann::json;

ChatServer::ChatServer(EventLoop *evLoop,
                       InetAddress listenAddress,
                       const string &name)
    : server_(evLoop, listenAddress, name)
{
    server_.setConnectionCallback(bind(&ChatServer::onConnection, this, _1));
    server_.setMessageCallback(bind(&ChatServer::onMessage, this, _1, _2, _3));

    server_.setThreadNum(4);
}

void ChatServer::start()
{
    server_.start();
}

void ChatServer::onConnection(const TcpConnectionPtr &conn)
{
    if (!conn->connected())
    {
        ChatService::instance()->client_close_exception(conn);
        conn->disconnected();
    }
}

void ChatServer::onMessage(const TcpConnectionPtr &conn,
                           Buffer *buf,
                           Timestamp time)
{
    string tmp = buf->retrieveAllAsString(); // json数据需要解析
    cout << tmp << endl;
    json js = json::parse(tmp);
    cout << 111 << endl;
    auto msg_handler = ChatService::instance()->get_handler(js["msgid"].get<int>());

    msg_handler(conn, js, time);
}