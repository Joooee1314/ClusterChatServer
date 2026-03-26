#include "groupmodel.hpp"
#include "db.h"

//创建群组
bool GroupModel::createGroup(Group& group){
    //组装sql语句
    std::string sql = "insert into AllGroup(groupname,groupdesc) values('";
    sql+=group.getName() + "','" + group.getDesc() + "')";
    
    MySQL mysql;
    if(mysql.connect()){
        if(mysql.update(sql)){
            group.setId(mysql_insert_id(mysql.getConnection()));
            return true;
        }
    }
    return false;    
}

//加入群组
void GroupModel::addGroup(int userid,int groupid,std::string role){
    //组装sql语句
    std::string sql = "insert into GroupUser values('" + 
                        std::to_string(groupid) + "','" + std::to_string(userid) + "','"+role+"')";
    
    MySQL mysql;
    if(mysql.connect()){
        mysql.update(sql);
    }    
}

//查询用户所在的群组信息
std::vector<Group> GroupModel::queryGroups(int userid){
    /**
     * 1.先根据userid在groupuser表中查询出该用户所属的群组信息
     * 2.在根据群组信息，查询属于该群组的所有用户的userid，并且和user表进行多表联合查询，查出用户的详细信息
     */
    std::string sql="select a.id,a.groupname,a.groupdesc from AllGroup a inner join GroupUser b on b.groupid = a.id where b.userid = " + std::to_string(userid);

    std::vector<Group>vec;
    MySQL mysql;
    if(mysql.connect()){
        MYSQL_RES* res = mysql.query(sql);
        if(res != nullptr){
            MYSQL_ROW row;
            while((row = mysql_fetch_row(res))!=nullptr){
                Group group;
                group.setId(std::stoi(row[0]));
                group.setName(row[1]);
                group.setDesc(row[2]);
                vec.push_back(group);
            }
            mysql_free_result(res);
        }
    }

    //查询群组的用户信息
    for(Group& group:vec){
        std::string sql="select a.id,a.name,a.state,b.grouprole from User a inner join GroupUser b on b.userid = a.id where b.groupid = " + std::to_string(group.getId());

        MYSQL_RES* res = mysql.query(sql);
        if(res != nullptr){
            MYSQL_ROW row;
            while((row = mysql_fetch_row(res))!=nullptr){
                GroupUser user;
                user.setId(std::stoi(row[0]));
                user.setName(row[1]);
                user.setState(row[2]);
                user.setRole(row[3]);
                group.getUsers().push_back(user);
            }
            mysql_free_result(res);
        }    
    }
    return vec;
}

//根据指定groupid查询群组用户id列表，除了userid自己，主要用于群聊业务给群组其他成员发送消息
std::vector<int> GroupModel::queryGroupUsers(int groupid,int userid){
    std::string sql="select userid from GroupUser where groupid = " + std::to_string(groupid) + " and userid != " + std::to_string(userid);

    std::vector<int>vec;
    MySQL mysql;
    if(mysql.connect()){
        MYSQL_RES* res = mysql.query(sql);
        if(res != nullptr){
            MYSQL_ROW row;
            while((row = mysql_fetch_row(res))!=nullptr){
                vec.push_back(std::stoi(row[0]));
            }
            mysql_free_result(res);
        }
    }
    return vec;
}