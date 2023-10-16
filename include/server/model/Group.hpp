#ifndef GROUP_H
#define GROUP_H

#include <iostream>
#include <vector>
#include <string>
#include "GroupUser.hpp"

using namespace std;

class Group
{
public:
    Group(int id = -1, string name = "", string desc = "")
        : id_(id), name_(name), desc_(desc)
    {
    }

    inline void set_id(int id)
    {
        id_ = id;
    }

    inline void set_name(string name)
    {
        name_ = name;
    }

    inline void set_desc(string desc)
    {
        desc_ = desc;
    }

    inline int get_id()
    {
        return id_;
    }

    inline string get_name()
    {
        return name_;
    }

    inline string get_desc()
    {
        return desc_;
    }

    inline vector<GroupUser> &get_vec()
    {
        return vec_;
    }

private:
    int id_;
    string name_;
    string desc_;
    vector<GroupUser> vec_;
};

#endif