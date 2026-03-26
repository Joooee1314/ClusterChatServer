#include "chatserver.hpp"
#include "chatservice.hpp"
#include "json.hpp"

#include <functional>
#include <string>
#include <mymuduo/Logger.h>
#include <iostream>
using namespace std;
using namespace placeholders;
using json = nlohmann::json;

ChatServer::ChatServer(EventLoop* loop, 
            const InetAddress& listenAddr,
            const std::string& nameArg)
            :server_(loop, listenAddr, nameArg),
            loop_(loop){
    //注册连接回调和消息回调
    server_.setConnectionCallback(std::bind(&ChatServer::onConnection, this, _1));
    server_.setMessageCallback(std::bind(&ChatServer::onMessage, this, _1, _2, _3));
    
    //设置线程数量
    server_.setThreadNum(4);
}

void ChatServer::start(){
    server_.start();
}

void ChatServer::onConnection(const TcpConnectionPtr& conn){
    //客户端断开连接
    if(!conn->connected()){
        ChatService::instance()->clientCloseException(conn);
        conn->shutdown();
    }
}

void ChatServer::onMessage(const TcpConnectionPtr& conn,
                            Buffer* buffer,
                            Timestamp receiveTime){
        string buf=buffer->retrieveAllAsString();
        //数据的反序列化 第三个参数false表示不抛异常
        json js = json::parse(buf,nullptr,false);
        if(!js.is_object()){
            LOG_ERROR("json格式错误,忽略!\n");
            return;
        }
        //完全解耦网络模块和业务模块
        //通过js["msgid"]获取消=>业务handler =>conn js time
        auto msgHandler = ChatService::instance()->getHandler(js["msgid"].get<int>());
        //回调信息绑定好的事件处理器，来执行相应的业务处理
        msgHandler(conn, js, receiveTime);
}

