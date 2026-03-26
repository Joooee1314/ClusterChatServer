#pragma once

#include <mymuduo/TcpServer.h>
#include <string>
class ChatServer{
public:
    ChatServer(EventLoop* loop, 
            const InetAddress& listenAddr,
            const std::string& nameArg);
    void start();
private:

    void onConnection(const TcpConnectionPtr& conn);
    void onMessage(const TcpConnectionPtr& conn,
            Buffer* buffer,
            Timestamp receiveTime);

    TcpServer server_;
    EventLoop* loop_;
};
