#include "MySQL.hpp"
#include "GroupModel.hpp"

bool GroupModel::create_group(Group &group)
{
    char sql[1024] = "";
    sprintf(sql, "insert into AllGroup(groupname, groupdesc) values('%s', '%s')", group.get_name().c_str(), group.get_desc().c_str());

    MySQL mysql;
    if (mysql.connect())
    {
        if (mysql.update(sql))
        {
            group.set_id(mysql_insert_id(mysql.get_Connection()));
            return true;
        }
    }
    return false;
}

bool GroupModel::add_group(int user_id, int group_id, string desc)
{
    char sql[1024] = "";
    sprintf(sql, "insert into GroupUser values(%d, %d, '%s')", group_id, user_id, desc.c_str());

    MySQL mysql;
    if (mysql.connect())
    {
        if (mysql.update(sql))
        {
            return true;
        }
    }
    return false;
}

vector<Group> GroupModel::query_group(int user_id)
{
    char sql[1024] = "";
    sprintf(sql, "select a.id, a.groupname, a.groupdesc from AllGroup a inner join GroupUser b on b.groupid = a.id where b.userid = %d", user_id);

    MySQL mysql;
    vector<Group> vec;
    if (mysql.connect())
    {
        MYSQL_RES *res = mysql.query(sql);
        if (res != nullptr)
        {
            MYSQL_ROW row;
            while ((row = mysql_fetch_row(res)) != nullptr)
            {
                Group group;
                group.set_id(atoi(row[0]));
                group.set_name(row[1]);
                group.set_desc(row[2]);
                vec.push_back(group);
            }
            mysql_free_result(res);
        }
    }

    // 查询群组内所有成员信息
    for (auto &tmp : vec)
    {
        sprintf(sql, "select a.id, a.name, a.state, b.grouprole from User a inner join GroupUser b on b.userid = a.id where b.groupid = %d", tmp.get_id());

        MYSQL_RES *res = mysql.query(sql);
        if (res != nullptr)
        {
            MYSQL_ROW row;
            while ((row = mysql_fetch_row(res)) != nullptr)
            {
                GroupUser group_user;
                group_user.set_id(atoi(row[0]));
                group_user.set_name(row[1]);
                group_user.set_state(row[2]);
                group_user.set_role(row[3]);
                tmp.get_vec().push_back(group_user);
            }
            mysql_free_result(res);
        }
    }

    return vec;
}

vector<int> GroupModel::query_group_users(int user_id, int group_id)
{
    char sql[1024] = "";
    sprintf(sql, "select userid from GroupUser where groupid = %d and userid != %d", group_id, user_id);

    MySQL mysql;
    vector<int> vec;
    if (mysql.connect())
    {
        MYSQL_RES *res = mysql.query(sql);
        if (res != nullptr)
        {
            MYSQL_ROW row;
            while ((row = mysql_fetch_row(res)) != nullptr)
            {
                vec.push_back(atoi(row[0]));
            }
            free(res);
        }
    }
    return vec;
}