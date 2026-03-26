#include "db.h"

#include <mymuduo/Logger.h>

//数据库配置信息
static std::string server = "127.0.0.1";
static std::string user = "root";
static std::string password = "123456";
static std::string dbname = "chat";

//初始化数据库连接
MySQL::MySQL(){
    conn_ = mysql_init(nullptr);
}

//释放数据库连接资源
MySQL::~MySQL(){
    if(conn_){
        mysql_close(conn_);
    }
}

//连接数据库
bool MySQL::connect(){
    MYSQL* p = mysql_real_connect(conn_, server.c_str(), user.c_str(), 
                                password.c_str(), dbname.c_str(), 3306, nullptr, 0);
    if(p!= nullptr){
       //c和c++代码默认的编码字符是ASCII，需设置才不会让MYSQL拉下来的中文显示？
       mysql_query(conn_, "set names utf8mb4");
       LOG_DEBUG("连接数据库成功!\n");
    }
    else{
        LOG_ERROR("连接数据库失败: %s\n", mysql_error(conn_));
    }
    return p;
}

//更新操作
bool MySQL::update(const std::string& sql){
    if(mysql_query(conn_, sql.c_str())){
        LOG_ERROR("更新失败: %s\n", mysql_error(conn_));
        return false;
    }
    return true;
}

//查询操作
MYSQL_RES* MySQL::query(const std::string& sql){
    if(mysql_query(conn_, sql.c_str())){
        LOG_ERROR("查询失败: %s\n", mysql_error(conn_));
        return nullptr;
    }
    return mysql_store_result(conn_);
}