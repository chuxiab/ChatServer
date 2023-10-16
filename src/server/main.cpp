#include "ChatServer.hpp"
#include "ChatService.hpp"
#include <signal.h>

using namespace muduo;
using namespace muduo::net;

void funcbreak(int arg)
{
    ChatService::instance()->reset();
    exit(0);
}

int main(int argc, char **argv)
{
    signal(SIGINT, funcbreak);

    EventLoop evLoop;
    short port = atoi(argv[1]);
    InetAddress listenAddr(port);
    std::string str("bai");
    ChatServer server(&evLoop, listenAddr, str);
    server.start();
    evLoop.loop();

    return 0;
}