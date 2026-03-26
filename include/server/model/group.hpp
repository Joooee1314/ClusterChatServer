#pragma once

#include "groupuser.hpp"

#include <vector>
#include <string>

class Group{
public:
    Group(int id=-1,std::string name ="",std::string desc=""):
        id(id),name(name),desc(desc){}
    
    void setId(int id){this->id=id;}
    void setName(std::string name){this->name=name;}
    void setDesc(std::string desc){this->desc=desc;}
    
    int getId() const{return id;}
    std::string getName() const{return name;}
    std::string getDesc() const{return desc;}
    std::vector<GroupUser>& getUsers(){return userVec;}

private:
    int id;
    std::string name;
    std::string desc;

    //群成员列表
    std::vector<GroupUser> userVec;
};