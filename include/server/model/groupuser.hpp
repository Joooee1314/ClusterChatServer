#pragma once

#include "user.hpp"

class GroupUser:public User{
public:
    void setRole(const std::string& role){this->role = role;}
    std::string getRole() const{return role;}
private:
    std::string role;
};