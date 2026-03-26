#include "chatservice.hpp"
#include "public.hpp"

#include <mymuduo/Logger.h>
#include <vector>
#include <iostream>
#include <cstdio>

//获取单例对象的接口函数
ChatService* ChatService::instance() {
    static ChatService service;
    return &service;
}

//注册信息以及对应的handler回调操作
ChatService::ChatService() {
    //注册消息id和对应的业务处理方法
    msgHandlerMap_[LOGIN_MSG] = std::bind(&ChatService::login, this, _1, _2, _3);
    msgHandlerMap_[REG_MSG] = std::bind(&ChatService::reg, this, _1, _2, _3);
    msgHandlerMap_[ONE_CHAT_MSG] = std::bind(&ChatService::oneChat, this, _1, _2, _3);
    msgHandlerMap_[ADD_FRIEND_MSG] = std::bind(&ChatService::addFriend, this, _1, _2, _3);
    msgHandlerMap_[ADD_FRIEND_MSG] = std::bind(&ChatService::addFriend, this, _1, _2, _3);
    msgHandlerMap_[CREATE_GROUP_MSG] = std::bind(&ChatService::createGroup, this, _1, _2, _3);
    msgHandlerMap_[ADD_GROUP_MSG] = std::bind(&ChatService::addGroup, this, _1, _2, _3);
    msgHandlerMap_[GROUP_CHAT_MSG] = std::bind(&ChatService::groupChat, this, _1, _2, _3);
    msgHandlerMap_[LOGOUT_MSG] = std::bind(&ChatService::logout, this, _1, _2, _3);
}

void ChatService::reset(){
    //把online状态用户重置为offline
    userModel_.resetState();
}

//获取消息对应的处理器
MsgHandler ChatService::getHandler(int msgid){
    //记录错误日志，msgid没有对应的事件回调
    auto it = msgHandlerMap_.find(msgid);
    if (it != msgHandlerMap_.end()) {
        return it->second;
    }
    //返回默认的处理器空操作，若返回空指针可能会程序崩溃
    return [=](const TcpConnectionPtr& conn, json& js, Timestamp time) {
        LOG_ERROR("msgid:%d can not find handler!\n", msgid);
    };
}

// 实现登录业务逻辑
void ChatService::login(const TcpConnectionPtr& conn, json& js, Timestamp time) {
    int id=js["id"].get<int>();
    std::string pwd=js["password"];

    User user = userModel_.query(id);
    if(user.getId() == id && user.getPassword() == pwd){
        if(user.getState() == "online"){
            //该用户已经在线，不允许重复登陆
            json response;
            response["msgid"] = REG_MSG_ACK;
            response["errno"] = 2;
            response["errmsg"]="this acount is using,input another!";
            conn->send(response.dump());
        }
        else{
            {
                std::lock_guard<std::mutex> lock(connMutex_);
                //登录成功,记录用户连接信息
                userConnMap_[id] = conn;
            }
            //登录成功,更新用户状态信息
            user.setState("online");
            userModel_.updateState(user);

            json response;
            response["msgid"] = LOGIN_MSG_ACK;
            response["errno"] = 0;
            response["id"]=user.getId();
            response["name"]=user.getName();
            //查询该用户是否有离线消息
            std::vector<std::string>vec=offlineMsgModel_.query(id);
            if(!vec.empty()){
                response["offlinemsg"]=vec;
                offlineMsgModel_.remove(id);
            }
            //查询该用户的好友信息并返回
            std::vector<User>userVec=friendModel_.query(id);
            if(!userVec.empty()){
                std::vector<std::string>vec2;
                for(User &user: userVec){
                    json js;
                    js["id"]=user.getId();
                    js["name"]=user.getName();
                    js["state"]=user.getState();
                    vec2.push_back(js.dump());
                }
                response["friends"]=vec2;
            }
            //查询该用户的群组列表并返回
            std::vector<Group>groupVec=groupModel_.queryGroups(id);
            if(!groupVec.empty()){
                std::vector<std::string>vec3;
                for(Group& group:groupVec){
                    json grpjson;
                    grpjson["id"]=group.getId();
                    grpjson["groupname"]=group.getName();
                    grpjson["groupdesc"]=group.getDesc();
                    std::vector<std::string>userV;
                    for(GroupUser &user: group.getUsers()){
                        json js;
                        js["id"]=user.getId();
                        js["name"]=user.getName();
                        js["state"]=user.getState();
                        js["role"]=user.getRole();
                        userV.push_back(js.dump());
                    }
                    grpjson["users"]=userV;
                    vec3.push_back(grpjson.dump());
                }
                response["groups"]=vec3;
            }

            conn->send(response.dump());
        }
    }
    else{
        //登陆失败:用户不存在或者用户存在但密码错误
        json response;
        response["msgid"] = REG_MSG_ACK;
        response["errno"] = 1;
        response["errmsg"]="id or password is invalid!";
        conn->send(response.dump());
    }
}

// 实现注册业务逻辑
void ChatService::reg(const TcpConnectionPtr& conn, json& js, Timestamp time) {
    std::string name = js["name"];
    std::string pwd = js["password"];

    User user;
    user.setName(name);
    user.setPassword(pwd);
    if(userModel_.insert(user)){
        //注册成功
        json response;
        response["msgid"] = REG_MSG_ACK;
        response["errno"] = 0;
        response["id"]=user.getId();
        conn->send(response.dump());
    }
    else{
        //注册失败
        json response;
        response["msgid"] = REG_MSG_ACK;
        response["errno"] = 1;
        conn->send(response.dump());
    }
}

//客户端异常退出处理
void ChatService::clientCloseException(const TcpConnectionPtr& conn){
    User user;
    {
        std::lock_guard<std::mutex> lock(connMutex_);
        for(auto it = userConnMap_.begin();it!=userConnMap_.end();it++){
            if(it->second == conn){
                user.setId(it->first);
                //从map表删除用户连接信息
                userConnMap_.erase(it);
                break;
            }
        }
    }

    //更新用户的状态信息
    if(user.getId()!=-1){
        user.setState("offline");
        userModel_.updateState(user);
    }
}

//处理注销业务
void ChatService::logout(const TcpConnectionPtr& conn, json& js, Timestamp time){
    int userid=js["id"].get<int>();

    {
        std::lock_guard<std::mutex> lock(connMutex_);
        auto it=userConnMap_.find(userid);
        if(it!=userConnMap_.end()){
            userConnMap_.erase(it);
        }
    }
    //更新用户状态信息
    User user(userid,"","","offline");
    userModel_.updateState(user);
}

//提供一对一聊天业务
void ChatService::oneChat(const TcpConnectionPtr& conn, json& js, Timestamp time){
    int toid = js["toid"].get<int>();
    {
        std::lock_guard<std::mutex>lock(connMutex_);
        auto it = userConnMap_.find(toid);
        if(it!=userConnMap_.end()){
            //toid在线 转发消息
            it->second->send(js.dump());
            return;
        }
    }

    //toid不在线，存储离线消息
    offlineMsgModel_.insert(toid,js.dump());
}

    //添加好友业务
void ChatService::addFriend(const TcpConnectionPtr& conn, json& js, Timestamp time){
    int userid = js["id"].get<int>();
    int friendid = js["friendid"].get<int>();

    //存储好友信息
    friendModel_.insert(userid,friendid);
}

//创建群组业务
void ChatService::createGroup(const TcpConnectionPtr& conn, json& js, Timestamp time){
    int userid=js["id"].get<int>();
    std::string groupname=js["groupname"].get<std::string>();
    std::string groupdesc=js["groupdesc"].get<std::string>();

    //存储新创建的群组消息
    Group group(-1,groupname,groupdesc);
    if(groupModel_.createGroup(group)){
        //存储群组成员信息
        groupModel_.addGroup(userid,group.getId(),"creator");
        json response;
        response["msgid"]=CREATE_GROUP_MSG_ACK;
        response["groupid"]=group.getId();
        response["errno"]=0;
        conn->send(response.dump());
    }
    //可扩展业务，发响应json，但c++不需要特别看重业务模块
}

//加入群组业务
void ChatService::addGroup(const TcpConnectionPtr& conn, json& js, Timestamp time){
    int userid = js["id"].get<int>();
    int groupid = js["groupid"].get<int>();
    groupModel_.addGroup(userid,groupid,"normal");
}

//群聊业务
void ChatService::groupChat(const TcpConnectionPtr& conn, json& js, Timestamp time){
    int userid = js["id"].get<int>();
    int groupid = js["groupid"].get<int>();

    //查询群组其他成员的userid
    std::vector<int> useridVec = groupModel_.queryGroupUsers(groupid,userid);
    {
        //设置锁，保证通信的线程安全
        std::lock_guard<std::mutex> lock(connMutex_);
        for(int id: useridVec){
            auto it = userConnMap_.find(id);
            if(it!=userConnMap_.end()){
                //转发消息
                it->second->send(js.dump());
            }
            else{
                //存储离线消息
                offlineMsgModel_.insert(id,js.dump());
            }
        }
    }    
}