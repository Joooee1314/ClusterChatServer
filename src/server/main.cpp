#include "chatserver.hpp"
#include "chatservice.hpp"

#include <iostream>
#include <signal.h>
using namespace std;

//处理服务器ctrl+c重置user的状态信息
void resetHandler(int){
    ChatService::instance()->reset();
    exit(0);
}

int main(){
    signal(SIGINT,resetHandler);

    signal(SIGABRT, resetHandler);

    EventLoop loop;
    InetAddress listenAddr(8000,"127.0.0.1");
    ChatServer server(&loop, listenAddr, "ChatServer");

    server.start();
    loop.loop();

    return 0;
}