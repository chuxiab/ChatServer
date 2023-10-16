#ifndef REDIS_H
#define REDIS_H

#include <iostream>
#include <hiredis/hiredis.h>
#include <thread>
#include <functional>

using namespace std;
using redis_handler = function<void(int, string)>;

class Redis
{
public:
    Redis();
    ~Redis();

    // 连接redis 服务器
    bool connect();

    // 向redis指定的通道channel发消息
    bool publish(int channel, string message);

    // 向redis 指定的通道订阅消息
    bool subscribe(int channel);

    // 取消订阅
    bool unsubscribe(int channel);

    // 独立线程接收订阅通道的信息
    void observer_channel_message();

    // 初识化业务上报通道的回调函数
    void init_notify_handler(redis_handler handler);

private:
    // hiredis 同步上下文对象， 负责publish对象
    redisContext *publish_context_;

    // 负责subscribe消息
    redisContext *subscribe_context_;

    // 回调操作，收到消息给service 上报
    redis_handler notify_message_handler_;
};

#endif