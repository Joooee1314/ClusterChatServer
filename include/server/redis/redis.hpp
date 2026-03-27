#pragma once

#include <hiredis/hiredis.h>
#include <thread>
#include <functional>

using namespace std;

class Redis{
public:
    Redis();
    ~Redis();

    //连接redis服务器
    bool connect();

    //向redis指定的channel通道发布消息
    bool publish(int channel,string message);

    //向redis指定的通道订阅消息
    bool subscribe(int channel);

    //向redis指定的通道取消订阅消息
    bool unsubscribe(int channel);

    //在独立线程中接收订阅通道的消息
    void observer_channel_message();

    //初始化向业务层上报通道消息的回调对象
    void init_notify_handler(function<void(int,string)> fn);

private:
    //hiredis同步上下文对象，负责publish
    redisContext* publish_context_;

    //hiredis同步上下文对象，负责subscribe
    redisContext* subscribe_context_;

    //回调操作，收到订阅消息，给server层上报
    function<void(int,string)> notify_message_handler_;
};