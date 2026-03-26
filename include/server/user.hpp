#pragma once

#include <string>

//user表的字段类，映射表
class User{
public:
    User(int id=-1,std::string name= "",std::string password="",std::string state="offline")
        :id(id),name(name),password(password),state(state){}

    void setId(int id){this->id = id;}
    void setName(const std::string& name){this->name = name;}
    void setPassword(const std::string& password){this->password = password;}
    void setState(const std::string& state){this->state = state;}

    int getId() const{return id;}
    std::string getName() const{return name;}
    std::string getPassword() const{return password;}
    std::string getState() const{return state;}
protected:
    int id;
    std::string name;
    std::string password;
    std::string state;
};