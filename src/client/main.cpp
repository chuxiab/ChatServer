#include <iostream>
#include <cstring>
#include <vector>
#include <thread>
#include <chrono>
#include <unordered_map>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>

#include "User.hpp"
#include "Group.hpp"
#include "public.hpp"
#include "json.hpp"

#define BUFFER_SIZE 1024
#define TMP_SIZE 128

using namespace std;
using json = nlohmann::json;

// 记录当前用户
User g_current_user;
// 记录当前用户的好友列表
vector<User> g_current_friends_list;
// 记录当前用户的群组列表
vector<Group> g_current_group_list;

bool g_menu_running = false;

// 显示当前登录成功用户的基本信息
void showCurrentUserData();
// 用户接收信息的线程
void readTaskHandler(int cfd);
// 获取系统时间
string getCurrentTime();
// 主聊天页面程序
void mainMenu(int cfd);

void help(int fd = 0, string str = "");
void chat(int cfd, string str);
void addFriend(int cfd, string str);
void createGroup(int cfd, string str);
void addGroup(int cfd, string str);
void groupChat(int cfd, string str);
void loginout(int cfd, string str);

unordered_map<string, string> command_map =
    {
        {"help", "显示所有支持的命令，格式 help"},
        {"chat", "一对一聊天，格式chat:friendid:message"},
        {"addfriend", "添加好友，格式addfriend:friendid"},
        {"creategroup", "创建群组，格式creategroup:groupname:groupdesc"},
        {"addgroup", "加入群组，格式addgroup:groupid"},
        {"groupchat", "群聊，格式groupchat:groupid:message"},
        {"loginout", "登出， 格式loginout"}};

unordered_map<string, function<void(int, string)>> command_handler_map =
    {
        {"help", help},
        {"chat", chat},
        {"addfriend", addFriend},
        {"creategroup", createGroup},
        {"addgroup", addGroup},
        {"groupchat", groupChat},
        {"loginout", loginout}};

int main(int argc, char **argv)
{
    if (argc < 3)
    {
        cout << "./file IP PORT" << endl;
        exit(1);
    }

    // 创建套接字
    int cfd = socket(AF_INET, SOCK_STREAM, 0);
    if (-1 == cfd)
    {
        perror("socket");
        exit(1);
    }

    char *ip = argv[1];
    short port = atoi(argv[2]);

    sockaddr_in addr;
    memset(&addr, 0, sizeof addr);

    // 连接套接字
    addr.sin_family = AF_INET;
    addr.sin_port = htons(port);
    addr.sin_addr.s_addr = inet_addr(ip);

    if (-1 == connect(cfd, (sockaddr *)&addr, sizeof(addr)))
    {
        perror("connect");
        exit(1);
    }

    for (;;)
    {
        cout << "************* Welcome **************" << endl;
        cout << "             1. login " << endl;
        cout << "             2. register " << endl;
        cout << "             3. quit" << endl;

        int choice = 0;
        cin >> choice;

        cin.get();
        switch (choice)
        {
        case 1:
        {
            int id;
            cout << "please input id : ";
            cin >> id;
            cin.get();

            char password[TMP_SIZE] = "";
            cout << "please input password : ";
            cin.getline(password, TMP_SIZE);

            json js;
            js["msgid"] = MsgType::LOGIN_MSG;
            js["id"] = id;
            js["password"] = password;
            string request = js.dump();

            int ret = send(cfd, request.c_str(), strlen(request.c_str()) + 1, 0);
            if (-1 == ret)
            {
                perror("send req msg error");
            }
            else
            {
                char buf[BUFFER_SIZE] = "";
                ret = recv(cfd, buf, BUFFER_SIZE, 0);
                if (-1 == ret)
                {
                    perror("recv response login msg error");
                }
                else
                {
                    json response_js = json::parse(buf);

                    if (response_js["errno"].get<int>() != 0)
                    {
                        cerr << response_js["errno"] << endl;
                    }
                    else
                    {
                        // 登录成功记录当前用户信息、好友信息、群组信息、离线消息
                        g_current_user.set_id(response_js["id"].get<int>());
                        g_current_user.set_name(response_js["name"]);

                        if (response_js.contains("friends"))
                        {
                            vector<string> vec = response_js["friends"];
                            for (auto &t : vec)
                            {
                                json friend_js = json::parse(t);
                                User user;
                                user.set_id(friend_js["id"].get<int>());
                                user.set_name(friend_js["name"]);
                                user.set_state(friend_js["state"]);

                                g_current_friends_list.push_back(user);
                            }
                        }
                        cout << "555555555" << endl;

                        if (response_js.contains("groups"))
                        {
                            vector<string> vec = response_js["groups"];
                            for (auto &t : vec)
                            {
                                json group_js = json::parse(t);
                                Group group;
                                group.set_id(group_js["id"].get<int>());
                                group.set_name(group_js["name"]);
                                group.set_desc(group_js["desc"]);

                                vector<string> user_vec = group_js["users"];
                                for (auto &tmp : vec)
                                {
                                    json user_ = json::parse(tmp);
                                    GroupUser group_user;
                                    group_user.set_id(user_["id"].get<int>());
                                    group_user.set_name(user_["name"]);
                                    group_user.set_state(user_["state"]);
                                    group_user.set_role(user_["role"]);
                                    group.get_vec().push_back(group_user);
                                }
                                g_current_group_list.push_back(group);
                            }
                        }

                        showCurrentUserData();
                        if (response_js.contains("offlinemsg"))
                        {
                            vector<string> vec = response_js["offlinemsg"];
                            for (auto &t : vec)
                            {
                                json off_js = json::parse(t);
                                cout << js["time"].get<string>() << " [" << js["id"].get<int>() << "] "
                                     << "name " << js["name"] << " "
                                     << "msg: " << js["msg"] << endl;
                            }
                        }

                        thread read_task(readTaskHandler, cfd);
                        read_task.detach();

                        g_menu_running = true;
                        mainMenu(cfd);
                    }
                }
            }
        }
        break;

        case 2:
        {
            char name[TMP_SIZE] = "";
            cout << "please input the name : " << endl;
            cin.getline(name, TMP_SIZE);

            char password[TMP_SIZE] = "";
            cout << "please input the password : " << endl;
            cin.getline(password, TMP_SIZE);

            User user;
            user.set_name(name);
            json js;
            js["msgid"] = MsgType::REG_MSG;
            js["name"] = name;
            js["password"] = password;

            string request = js.dump();

            int ret = send(cfd, request.c_str(), strlen(request.c_str()) + 1, 0);
            if (-1 == ret)
            {
                perror("send regist msg error");
            }
            else
            {
                char buf[BUFFER_SIZE] = "";
                ret = recv(cfd, buf, BUFFER_SIZE, 0);
                if (-1 == ret)
                {
                    perror("recv regist msg error");
                }
                else
                {
                    json response_js = json::parse(buf);

                    if (response_js["errno"].get<int>() == 0)
                    {
                        user.set_id(response_js["id"]);
                        cout << "ID : " << user.get_id() << " name : " << user.get_name() << " regist success!" << endl;
                    }
                    else
                    {
                        perror("regist error");
                    }
                }
            }
        }
        break;
        case 3:
        {
            close(cfd);
            cout << "Quit" << endl;
            exit(0);
        }
        break;

        default:
            cerr << "invalid input" << endl;
            break;
        }
    }

    return 0;
}

void showCurrentUserData()
{
    cout << "---------------------  login user ---------------------" << endl;
    cout << "current user id : " << g_current_user.get_id() << " name : " << g_current_user.get_name() << endl;

    cout << "---------------------  friend list ---------------------" << endl;

    if (!g_current_friends_list.empty())
    {
        for (auto &t : g_current_friends_list)
        {
            cout << t.get_id() << ' ' << t.get_name() << ' ' << t.get_state() << endl;
        }
    }

    cout << "---------------------  group list ---------------------" << endl;
    for (auto &group : g_current_group_list)
    {
        cout << group.get_id() << ' ' << group.get_name() << ' ' << group.get_desc() << endl;
        cout << "---------------------  group user ---------------------";
        for (auto &t : group.get_vec())
        {
            cout << t.get_id() << ' ' << t.get_name() << ' ' << t.get_role() << ' ' << t.get_state() << endl;
        }
        cout << "-------------------------------------------------------" << endl;
    }
}

void readTaskHandler(int cfd)
{
    while (1)
    {
        char buf[BUFFER_SIZE] = "";
        int ret = recv(cfd, buf, BUFFER_SIZE, 0);
        if (ret == -1 || ret == 0)
        {
            continue;
        }

        json js = json::parse(buf);
        if (js["msgid"].get<int>() == (int)MsgType::ONE_CHAT_MSG)
        {
            cout << js["time"] << "[ " << js["id"] << " ] " << js["name"] << ' '
                 << " : " << js["msg"] << endl;
        }
        else if (js["msgid"].get<int>() == (int)MsgType::GROUP_CHAT)
        {
            cout << "group msg : "
                 << "[ " << js["groupid"] << " ] ";
            cout << js["time"] << "[ " << js["id"] << " ] " << js["name"] << ' '
                 << " : " << js["msg"] << endl;
        }
    }
}

string getCurrentTime()
{
    auto tt = chrono::system_clock::to_time_t(std::chrono::system_clock::now());
    tm *ntm = localtime(&tt);
    char date[TMP_SIZE] = "";
    sprintf(date, "%d-%02d-%02d %02d-%02d-%02d", ntm->tm_year + 1900, ntm->tm_mon + 1,
            ntm->tm_mday, ntm->tm_hour, ntm->tm_min, ntm->tm_sec);

    return string(date);
}

void mainMenu(int cfd)
{
    help(0, "");
    char buf[BUFFER_SIZE] = "";
    while (g_menu_running)
    {
        cin.getline(buf, BUFFER_SIZE);
        string command_buf(buf);

        string command;
        int index = command_buf.find(":");
        if (index == -1)
        {
            command = command_buf;
        }
        else
        {
            command = command_buf.substr(0, index);
        }

        auto it = command_handler_map.find(command);

        if (it == command_handler_map.end())
        {
            cerr << "invalid command input" << endl;
            continue;
        }

        it->second(cfd, command_buf.substr(index + 1));
    }
}

void help(int fd, string str)
{
    cout << "------------------  command list  -----------------------" << endl;
    for (auto &it : command_map)
        cout << it.first << "  " << it.second << endl;

    cout << endl;
}

void chat(int cfd, string str)
{
    int index = str.find(":");
    if (-1 == index)
    {
        cerr << "invalid command input" << endl;
        return;
    }

    int friend_id = atoi(str.substr(0, index).c_str());
    string message = str.substr(index + 1);

    json js;
    js["msgid"] = (int)MsgType::ONE_CHAT_MSG;
    js["id"] = g_current_user.get_id();
    js["name"] = g_current_user.get_name();
    js["to"] = friend_id;
    js["msg"] = message;
    js["time"] = getCurrentTime();

    string request = js.dump();

    int ret = send(cfd, request.c_str(), strlen(request.c_str()) + 1, 0);

    if (-1 == ret)
    {
        cerr << "send one chat msg error" << endl;
    }
}

void addFriend(int cfd, string str)
{
    int friend_id = atoi(str.c_str());

    json js;
    js["msgid"] = MsgType::ADD_FRIEND;
    js["id"] = g_current_user.get_id();
    js["friendid"] = friend_id;

    string request = js.dump();

    int ret = send(cfd, request.c_str(), strlen(request.c_str()) + 1, 0);
    if (-1 == ret)
    {
        cerr << "send addfriend msg error" << endl;
    }
}

void createGroup(int cfd, string str)
{
    int index = str.find(":");
    if (-1 == index)
    {
        cerr << "invalid command input" << endl;
        return;
    }

    string group_name = str.substr(0, index);
    string group_desc = str.substr(index + 1);

    json js;
    js["msgid"] = (int)MsgType::GROUP_CREATE;
    js["id"] = g_current_user.get_id();
    js["groupname"] = group_name;
    js["desc"] = group_desc;

    string request = js.dump();

    int ret = send(cfd, request.c_str(), strlen(request.c_str()) + 1, 0);
    if (-1 == ret)
    {
        cerr << "send create group msg error" << endl;
    }
}

void addGroup(int cfd, string str)
{
    int group_id = atoi(str.c_str());
    string desc = "hello I am " + g_current_user.get_name();

    json js;
    js["msgid"] = (int)MsgType::GROUP_ADD;
    js["id"] = g_current_user.get_id();
    js["groupid"] = group_id;
    js["desc"] = desc;

    string request = js.dump();

    int ret = send(cfd, request.c_str(), strlen(request.c_str()) + 1, 0);
    if (-1 == ret)
    {
        cerr << "send add group msg error" << endl;
    }
}

void groupChat(int cfd, string str)
{
    int index = str.find(":");
    if (-1 == index)
    {
        cerr << "invalid command input" << endl;
        return;
    }

    int group_id = atoi(str.substr(0, index).c_str());
    string msg = str.substr(index + 1);

    json js;
    js["msgid"] = (int)MsgType::GROUP_CHAT;
    js["userid"] = g_current_user.get_id();
    js["name"] = g_current_user.get_name();
    js["groupid"] = group_id;
    js["msg"] = msg;
    js["time"] = getCurrentTime();

    string request = js.dump();

    int ret = send(cfd, request.c_str(), strlen(request.c_str()) + 1, 0);
    if (-1 == ret)
    {
        cerr << "send group chat msg error" << endl;
    }
}

void loginout(int cfd, string str)
{
    json js;
    js["msgid"] = MsgType::LOGINOUT_MSG;
    js["id"] = g_current_user.get_id();

    string request = js.dump();

    int ret = send(cfd, request.c_str(), strlen(request.c_str()) + 1, 0);
    if (-1 == ret)
    {
        cerr << "send loginout msg error" << endl;
    }
    else
    {
        close(cfd);
        g_menu_running = false;
        g_current_friends_list.clear();
        g_current_group_list.clear();
    }
}
