#include "db.h"
#include "usermodel.hpp"

#include <string>

//User表的增加方法
bool UserModel::insert(User& user){
    //组装sql语句
    std::string sql = "insert into User(name,password,state) values('";
    sql+=user.getName() + "','" + user.getPassword() + "','" + user.getState() + "')";
    
    MySQL mysql;
    if(mysql.connect()){
        if(mysql.update(sql)){
            //插入成功，获取插入成功的用户id
            user.setId(mysql_insert_id(mysql.getConnection()));
            return true;
        }
    }
    return false;
}

//根据用户id查询用户信息
User UserModel::query(int id){
    std::string sql="select * from User where id = " + std::to_string(id);

    MySQL mysql;
    if(mysql.connect()){
        MYSQL_RES* res = mysql.query(sql);
        if(res != nullptr){
            MYSQL_ROW row = mysql_fetch_row(res);
            if(row != nullptr){
                //将查询到的用户信息封装到User对象中
                User user;
                user.setId(std::stoi(row[0]));
                user.setName(row[1]);
                user.setPassword(row[2]);
                user.setState(row[3]);
                mysql_free_result(res);
                return user;
            }
            mysql_free_result(res);
        }
    }
    return User();
}

bool UserModel::updateState(User& user){
    std::string sql = "update User set state = '"+user.getState()+"' where id = " + std::to_string(user.getId());
    
    MySQL mysql;
    if(mysql.connect()){
        if(mysql.update(sql)){
            return true;
        }
    }
    return false;
}

void UserModel::resetState(){
    std::string sql = "update User set state = 'offline' where state= 'online'";
    
    MySQL mysql;
    if(mysql.connect()){
        mysql.update(sql);
    }
}