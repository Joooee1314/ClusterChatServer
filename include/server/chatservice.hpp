#pragma once
#include "json.hpp"
#include "usermodel.hpp"
#include "offlinemessagemodel.hpp"
#include "friendmodel.hpp"
#include "groupmodel.hpp"

#include <mymuduo/TcpConnection.h>
#include <unordered_map>
#include <functional>
#include <mutex>
using namespace std::placeholders;
using json = nlohmann::json;
using MsgHandler = std::function<void(const TcpConnectionPtr& conn,
                            json& js,
                            Timestamp time)>;

//聊天服务器业务类
class ChatService{
public:
    //获取单例对象的接口函数
    static ChatService* instance(); 
    
    //处理登录业务
    void login(const TcpConnectionPtr& conn, json& js, Timestamp time);
    
    //处理注册业务
    void reg(const TcpConnectionPtr& conn, json& js, Timestamp time);

    //提供一对一聊天业务
    void oneChat(const TcpConnectionPtr& conn, json& js, Timestamp time);

    //添加好友业务
    void addFriend(const TcpConnectionPtr& conn, json& js, Timestamp time);

    //创建群组业务
    void createGroup(const TcpConnectionPtr& conn, json& js, Timestamp time);

    //加入群组业务
    void addGroup(const TcpConnectionPtr& conn, json& js, Timestamp time);
    
    //群聊业务
    void groupChat(const TcpConnectionPtr& conn, json& js, Timestamp time);

    //处理注销业务
    void logout(const TcpConnectionPtr& conn, json& js, Timestamp time);
    
    //获取消息对应的处理器
    MsgHandler getHandler(int msgid);

    //处理客户端异常退出
    void clientCloseException(const TcpConnectionPtr& conn);

    //服务器异常，业务重置方法
    void reset();
private:
    //单例模式
    ChatService();

    //存储消息id和对应的业务处理方法
    std::unordered_map<int, MsgHandler> msgHandlerMap_; 
    
    //互斥锁，保护userConnMap_的线程安全
    std::mutex connMutex_;
    //存储在线用户的通信连接 noMessage本身在不同工作线程回调，因此需要考虑线程安全
    std::unordered_map<int, TcpConnectionPtr> userConnMap_;

    //数据操作类对象
    UserModel userModel_;
    OfflineMsgModel offlineMsgModel_;
    FriendModel friendModel_;
    GroupModel groupModel_;

};