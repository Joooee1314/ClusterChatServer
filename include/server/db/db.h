#pragma once

#include <mymuduo/Logger.h>
#include <mysql/mysql.h>

#include <string>


class MySQL{
public:
    //初始化数据库连接
    MySQL();

    //释放数据库连接资源
    ~MySQL();

    //连接数据库
    bool connect();

    //更新操作
    bool update(const std::string& sql);

    //查询操作
    MYSQL_RES* query(const std::string& sql);

    MYSQL* getConnection(){return conn_;}
private:
    MYSQL* conn_;
};