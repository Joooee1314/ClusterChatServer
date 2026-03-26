#include "offlinemessagemodel.hpp"
#include "db.h"

//存储用户的离线消息
void OfflineMsgModel::insert(int userid,std::string msg){
    //组装sql语句
    std::string sql = "insert into OfflineMessage(userid, message) values('" + std::to_string(userid) + "','" +msg + "')";
    
    MySQL mysql;
    if(mysql.connect()){
        mysql.update(sql);
    }
}

//删除用户的离线消息
void OfflineMsgModel::remove(int userid){
    //组装sql语句
    std::string sql = "delete from OfflineMessage where userid="+std::to_string(userid);
    
    MySQL mysql;
    if(mysql.connect()){
        mysql.update(sql);
    }
}

//查询用户的离线消息
std::vector<std::string> OfflineMsgModel::query(int userid){
    std::string sql="select message from OfflineMessage where userid = " + std::to_string(userid);

    std::vector<std::string>vec;
    MySQL mysql;
    if(mysql.connect()){
        MYSQL_RES* res = mysql.query(sql);
        if(res != nullptr){
            //把userid用户的所有离线消息放入vec中返回
            MYSQL_ROW row;
            while((row = mysql_fetch_row(res))!=nullptr){
                vec.push_back(row[0]);
            }
            mysql_free_result(res);
        }
    }
    return vec;
}