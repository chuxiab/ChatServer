#ifndef GROUPUSER_H
#define GROUPUSER_H

#include "User.hpp"

using namespace std;

class GroupUser : public User
{
public:
    inline void set_role(std::string role)
    {
        role_ = role;
    }

    inline std::string get_role()
    {
        return this->role_;
    }

private:
    string role_;
};

#endif