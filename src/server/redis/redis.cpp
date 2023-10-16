#include "redis.hpp"

Redis::Redis()
    : publish_context_(nullptr), subscribe_context_(nullptr)
{
}

Redis::~Redis()
{
    if (publish_context_ != nullptr)
    {
        redisFree(publish_context_);
    }

    if (subscribe_context_ != nullptr)
    {
        redisFree(subscribe_context_);
    }
}

bool Redis::connect()
{
    publish_context_ = redisConnect("127.0.0.1", 6379);
    if (publish_context_ == nullptr)
    {
        cerr << "redis connect error" << endl;
        return false;
    }

    subscribe_context_ = redisConnect("127.0.0.1", 6379);
    if (subscribe_context_ == nullptr)
    {
        cerr << "redis connect error" << endl;
        return false;
    }

    thread t([&]
             { observer_channel_message(); });

    t.detach();

    cout << "connect redis success" << endl;
    return true;
}

bool Redis::publish(int channel, string message)
{
    redisReply *reply = (redisReply *)redisCommand(publish_context_, "PUBLISH %d %s", channel, message.c_str());

    if (reply == nullptr)
    {
        cerr << "publish command error" << endl;
        return false;
    }

    freeReplyObject(reply);

    return true;
}

bool Redis::subscribe(int channel)
{
    if (REDIS_ERR == redisAppendCommand(subscribe_context_, "SUBSCRIBE %d", channel))
    {
        cerr << "subscribe channel error" << endl;
        return false;
    }

    int done = 0;
    while (!done)
    {
        if (REDIS_ERR == redisBufferWrite(subscribe_context_, &done))
        {
            cerr << "subscribe command error" << endl;
            return false;
        }
    }

    return true;
}

bool Redis::unsubscribe(int channel)
{
    if (REDIS_ERR == redisAppendCommand(subscribe_context_, "UNSUBSCRIBE %d", channel))
    {
        cerr << "unsubscribe channel error" << endl;
        return false;
    }

    int done = 0;
    while (!done)
    {
        if (REDIS_ERR == redisBufferWrite(subscribe_context_, &done))
        {
            cerr << "unsubscribe command error" << endl;
            return false;
        }
    }

    return true;
}

void Redis::observer_channel_message()
{
    redisReply *reply = nullptr;
    while (REDIS_OK == redisGetReply(subscribe_context_, (void **)&reply))
    {
        if (reply != nullptr && reply->element[2] != nullptr && reply->element[2]->str != nullptr)
        {
            notify_message_handler_(atoi(reply->element[1]->str), reply->element[2]->str);
        }
        freeReplyObject(reply);
    }

    cerr << "------------------ oberserve channel message quit -------------------" << endl;
}

void Redis::init_notify_handler(redis_handler handler)
{
    notify_message_handler_ = handler;
}
