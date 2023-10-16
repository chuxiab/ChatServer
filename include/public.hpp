#ifndef PUBLIC_H
#define PUBLIC_H

/*
    server 和 service共用
*/

enum class MsgType
{
    LOGIN_MSG = 1,
    LOGIN_MSG_ACK,
    REG_MSG,
    REG_MSG_ACK,
    ONE_CHAT_MSG,
    ADD_FRIEND,
    GROUP_CREATE,
    GROUP_ADD,
    GROUP_CHAT,
    LOGINOUT_MSG
};

#endif