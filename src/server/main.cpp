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

int main(int argc, char **argv){
    // 判断参数是否正确输入：./ChatServer <ip> <port>
    if (argc < 3) {
        cerr << "Usage: " << argv[0] << " <ip> <port>" << endl;
        exit(-1);
    }

    signal(SIGINT, resetHandler);
    signal(SIGABRT, resetHandler);

    EventLoop loop;

    // 从命令行读取 IP 和 Port
    const char *ip = argv[1];
    uint16_t port = static_cast<uint16_t>(atoi(argv[2]));

    InetAddress listenAddr(port, ip);  
    ChatServer server(&loop, listenAddr, "ChatServer");

    server.start();
    loop.loop();

    return 0;
}