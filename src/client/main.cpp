#include "json.hpp"
#include "group.hpp"
#include "public.hpp"
#include "user.hpp"

#include <iostream>
#include <thread>
#include <vector>
#include <string>
#include <chrono>
#include <ctime>
#include <unistd.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <cstdio>

using namespace std;
using json = nlohmann::json;

//记录当前系统登录的用户信息
User g_currentUser;
//记录当前登陆用户的好友列表信息
vector<User> g_currentUserFriendList;
//记录当前登录用户的群组列表信息
vector<Group> g_currentUserGroupList;
//显示当前登录成功用户的基本信息
void showCurrentUserData();
//控制主菜单页面程序
bool isMainMenuRunning=false;

//接收线程
void readTaskHandler(int clientfd);
//获取系统时间（聊天信息需要添加时间戳）
string getCurrentTime();
//主聊天界面
void mainMenu(int clientfd);

//聊天客户端程序实现，main线程用作发送线程，子线程用作接收线程
int main(int argc, char** argv){
    if(argc < 3){
        cout<<"command invalid! example: "<<"./ChatClient <ip> <port>"<<endl;
        exit(-1);
    }

    //解析命令行参数，获取服务器的ip和端口号
    char* ip = argv[1];
    uint16_t port = atoi(argv[2]);

    //创建client端的socket
    int clientfd = socket(AF_INET,SOCK_STREAM,0);
    if(clientfd == -1){
        cerr<<"socket create error!"<<endl;
        exit(-1);
    }

    //填写client需要连接的server信息ip+port
    sockaddr_in server{};
    server.sin_family = AF_INET;
    server.sin_port = htons(port);
    server.sin_addr.s_addr = inet_addr(ip);

    //连接到服务器
    if(connect(clientfd, (sockaddr*)&server, sizeof(server)) == -1){
        cerr<<"connect error!"<<endl;
        close(clientfd);
        exit(-1);
    }

    //main线程用于接收用户输入，负责发送数据
    while(true){
        //显示首页面菜单 登录 注册 退出
        cout<<"==================================="<<endl;
        cout<<"|            1. login             |"<<endl;
        cout<<"|            2. register          |"<<endl;
        cout<<"|            3. quit              |"<<endl;
        cout<<"==================================="<<endl;
        cout<<"choice:";
        int choice = 0;
        //因为输入整数后 缓冲区会残留一个回车，若是下面输入字符串则会直接读取到这个回车
        cin >> choice;
        //处理掉输入数字后的回车
        cin.get(); 

        switch(choice){
            case 1://登陆业务
            {
                int id=0;
                char pwd[50]{0};
                cout<<"userid:";
                cin>>id;
                cin.get();
                cout<<"password:";
                cin.getline(pwd,50);

                json js;
                js["msgid"]=LOGIN_MSG;
                js["id"]=id;
                js["password"]=pwd;
                string request = js.dump();

                int len=send(clientfd, request.c_str(), strlen(request.c_str())+1, 0);
                if(len == -1){
                    cerr<<"send login msg error!"<<request<<endl;
                }
                else{
                    char buffer[2048]{0};
                    len = recv(clientfd, buffer, sizeof(buffer), 0);
                    if(len==-1){
                        cerr<<"recv login response error!"<<endl;
                    }
                    else{
                        json response = json::parse(buffer);
                        if(response["errno"].get<int>() == 0){
                            //登录成功，记录当前用户的id和name
                            g_currentUser.setId(response["id"].get<int>());
                            g_currentUser.setName(response["name"]);

                            //记录当前用户的好友列表信息
                            if(response.contains("friends")){
                                g_currentUserFriendList.clear();
                                vector<string> friendArr = response["friends"];
                                for(string& friendstr : friendArr){
                                    json friendJson = json::parse(friendstr);
                                    User user;
                                    user.setId(friendJson["id"].get<int>());
                                    user.setName(friendJson["name"]);
                                    user.setState(friendJson["state"]);
                                    g_currentUserFriendList.push_back(user);
                                }
                            }

                            //记录当前用户的群组列表信息
                            if(response.contains("groups")){
                                g_currentUserGroupList.clear();
                                vector<string> groupArr = response["groups"];
                                for(string& groupstr : groupArr){
                                    json groupJson = json::parse(groupstr);
                                    Group group;
                                    group.setId(groupJson["id"].get<int>());
                                    group.setName(groupJson["groupname"]);
                                    group.setDesc(groupJson["groupdesc"]);

                                    vector<string> usersArr = groupJson["users"];
                                    for(string& userstr : usersArr){
                                        json userJson = json::parse(userstr);
                                        GroupUser user;
                                        user.setId(userJson["id"].get<int>());
                                        user.setName(userJson["name"]);
                                        user.setState(userJson["state"]);
                                        user.setRole(userJson["role"]);
                                        group.getUsers().push_back(user);
                                    }
                                    g_currentUserGroupList.push_back(group);
                                }
                            }

                            //显示当前用户的基本信息
                            showCurrentUserData();

                            //显示当前用户的离线消息，个人聊天消息或者群组消息
                            if(response.contains("offlinemsg")){
                                vector<string> vec=response["offlinemsg"];
                                cout<<"=================离线消息================="<<endl;
                                for(string& str:vec){
                                    json js = json::parse(str);
                                    if(js.contains("msgid")){
                                        if(js["msgid"].get<int>() == ONE_CHAT_MSG){
                                            cout<<js["time"].get<std::string>()<<"["<<js["id"]<<"]"<<js["name"].get<std::string>()
                                                <<" said: "<<js["msg"].get<std::string>()<<endl;
                                            continue;
                                        }
                                        else{
                                            cout<<"群消息["<<js["groupid"]<<"]:"<<js["time"].get<string>()<<"["<<js["id"]<<"]:"
                                                <<js["name"].get<string>()<<" said: "<<js["msg"].get<string>()<<endl;
                                        }
                                    }
                                }
                                cout<<"=========================================="<<endl;
                            }

                            //登录成功，启动接收线程负责接收数据
                            static int readThreadNumber=0;
                            if(readThreadNumber==0){
                                std::thread readTask(readTaskHandler, clientfd);
                                readTask.detach();
                                readThreadNumber++;
                            }
                            
                            //进入聊天主菜单页面
                            isMainMenuRunning=true;
                            mainMenu(clientfd);
                        }
                        else{
                            //登录失败，显示错误信息
                            cout<<"login failed! reason:"<<response["errmsg"]<<endl;
                        }
                    }
                }
            }
            break;
            case 2://注册业务
            {
                char name[50]{0};
                char pwd[50]{0};
                cout<<"username:";
                cin.getline(name,50);
                cout<<"password:";
                cin.getline(pwd,50);

                json js;
                js["msgid"]=REG_MSG;
                js["name"]=name;
                js["password"]=pwd;
                string request = js.dump();

                int len = send(clientfd, request.c_str(), strlen(request.c_str())+1, 0);
                if(len == -1){
                    cerr<<"send reg msg error!"<<request<<endl;
                }
                else{
                    char buffer[1024]{0};
                    len = recv(clientfd, buffer, sizeof(buffer), 0);
                    if(len==-1){
                        cerr<<"recv reg response error!"<<endl;
                    }
                    else{
                        json response = json::parse(buffer);
                        if(response["errno"].get<int>() == 0){
                            //注册成功
                            cout<<name<<"register success, userid is "<<response["id"]<<",do not forget it!"<<endl;
                        }
                        else{
                            //注册失败
                            cout<<name<<"is already exist,register failed!"<<endl;
                        }
                    }
                }
            }
            break;
            case 3://退出业务
                close(clientfd);
                exit(0);
            default:
                cout<<"invalid input!"<<endl;
                break;
        }
    }
    return 0;
}

//显示当前登陆成功用户的基本信息
void showCurrentUserData(){
    cout<<"===================当前用户信息==================="<<endl;
    cout<<"用户id:"<<g_currentUser.getId()<<" 用户名:"<<g_currentUser.getName()<<endl;
    cout<<"---------------------好友列表---------------------"<<endl;
    if(!g_currentUserFriendList.empty()){
        for(User& user : g_currentUserFriendList){
            cout<<user.getId()<<" "<<user.getName()<<" "<<user.getState()<<endl;
        }
    }
    cout<<"---------------------群组列表---------------------"<<endl;
    if(!g_currentUserGroupList.empty()){
        for(Group& group : g_currentUserGroupList){
            cout<<group.getId()<<" "<<group.getName()<<" "<<group.getDesc()<<endl;
            for(GroupUser& user : group.getUsers()){
                cout<<user.getId()<<" "<<user.getName()<<" "
                    <<user.getState()<<" "<<user.getRole()<<endl;
            }
            cout<<endl;
        }
    }
    cout<<"================================================="<<endl;
}

//接收线程
void readTaskHandler(int clientfd){
    while(true){
        char buffer[1024]{0};
        int len = recv(clientfd, buffer, sizeof(buffer), 0);
        if(len == -1 || len == 0){
            cerr<<"recv error or server closed the connection!"<<endl;
            close(clientfd);
            exit(-1);
        }
        else{
            json js = json::parse(buffer);
            if(js.contains("msgid")){
                int msgid = js["msgid"].get<int>();
                if(msgid == ONE_CHAT_MSG){
                    cout<<js["time"].get<std::string>()<<"["<<js["id"]<<"]"<<js["name"].get<std::string>()
                        <<" said: "<<js["msg"].get<std::string>()<<endl;
                    continue;
                }
                
                if(msgid == GROUP_CHAT_MSG){
                    cout<<"群消息["<<js["groupid"]<<"]:"<<js["time"].get<string>()<<"["<<js["id"]<<"]:"
                        <<js["name"].get<string>()<<" said: "<<js["msg"].get<string>()<<endl;
                }
            }
        }
    }
}

void help(int fd=0, string str="");
void chat(int, string);
void addfriend(int, string);
void creategroup(int, string);
void addgroup(int, string);
void groupchat(int, string);
void logout(int, string);

//系统支持的客户端命令列表
unordered_map<string,string>commandMap = {
    {"help","显示所有支持的命令,格式help"},
    {"chat","一对一聊天,格式chat:friendid:message"},
    {"addfriend","添加好友,格式addfriend:friendid"},
    {"creategroup","创建群组, 格式creategroup:groupname:groupdesc"},
    {"addgroup","加入群组, 格式addgroup:groupid"},
    {"groupchat","群聊,格式groupchat:groupid:message"},
    {"logout","注销, 格式logout"}
};

//注册系统支持的客户端命令处理
unordered_map<string, function<void(int, string)>>commandHandlerMap ={
    {"help", help},
    {"chat", chat},
    {"addfriend",addfriend},
    {"creategroup", creategroup},
    {"addgroup", addgroup},
    {"groupchat",groupchat},
    {"logout", logout}
};

//主聊天页面程序
void mainMenu(int clientfd){
    help();
    char buffer[1024]={0};
    while(isMainMenuRunning){
        cin.getline(buffer,1024);
        string commandbuf(buffer);
        string command;
        int idx=commandbuf.find(":");
        if(idx==-1){
            command=commandbuf;
        }
        else{
            command = commandbuf.substr(0,idx);
        }
        auto it =commandHandlerMap.find(command);
        if(it==commandHandlerMap.end()){
            cerr<<"invalid input command!"<<endl;
            continue;
        }
        it->second(clientfd,commandbuf.substr(idx+1,commandbuf.size()-idx));
    }
}

void help(int,string){
    cout<<"show command list >>>"<<endl;
    for(auto & c:commandMap){
        cout<<c.first<<":"<<c.second<<endl;
    }
    cout<<endl;
}

void addfriend(int clientfd,string str){
    int friendid=stoi(str);
    json js;
    js["msgid"]=ADD_FRIEND_MSG;
    js["id"]=g_currentUser.getId();
    js["friendid"]=friendid;
    string buffer=js.dump();

    int len=send(clientfd,buffer.c_str(),buffer.size()+1,0);
    if(len==-1){
        cerr<<"send addfriend msg error ->"<<buffer<<endl;
    }
}

void chat(int clientfd,string str){
    int idx=str.find(":");
    if(idx==-1){
        cerr<<"chat command invalid!"<<endl;
        return;
    }
    int friendid = stoi(str.substr(0,idx));
    string message=str.substr(idx+1,str.size()-idx);

    json js;
    js["msgid"]=ONE_CHAT_MSG;
    js["id"]=g_currentUser.getId();
    js["name"]=g_currentUser.getName();
    js["toid"]=friendid;
    js["msg"]=message;
    js["time"]=getCurrentTime();
    string buffer = js.dump();

    int len= send(clientfd,buffer.c_str(),buffer.size()+1,0);
    if(len==-1){
        cerr<<"send chat msg error ->"<<buffer<<endl;
    }
}

//获取系统时间 聊天信息需要添加时间信息
string getCurrentTime(){
    auto tt=std::chrono::system_clock::to_time_t(std::chrono::system_clock::now());
    tm *ptm = localtime(&tt);
    char timeBuf[100] = {0};
    sprintf(timeBuf, "%04d-%02d-%02d %02d:%02d:%02d",
        ptm->tm_year + 1900,  // 年份从1900开始算，要+1900
        ptm->tm_mon + 1,      // 月份0~11，要+1
        ptm->tm_mday,         // 日
        ptm->tm_hour,         // 时
        ptm->tm_min,          // 分
        ptm->tm_sec);         // 秒

    return string(timeBuf);
}

void creategroup(int clientfd, string str){
    int idx=str.find(":");
    if(idx==-1){
        cerr<<"creategroup command invalid!"<<endl;
        return;
    }
    
    string groupname=str.substr(0,idx);
    string groupdesc=str.substr(idx+1,str.size()-idx);

    json js;
    js["msgid"]=CREATE_GROUP_MSG;
    js["id"]=g_currentUser.getId();
    js["groupname"]=groupname;
    js["groupdesc"]=groupdesc;
    string buffer = js.dump();

    int len= send(clientfd,buffer.c_str(),buffer.size()+1,0);
    if(len==-1){
        cerr<<"send creategroup msg error ->"<<buffer<<endl;
    }
    else{
        char res[256]={0};
        int len=recv(clientfd,res,sizeof(res),0);
        json response=json::parse(res);
        if(response["errno"].get<int>()==0){
            cout<<"create success , remember your groupid:"<<response["groupid"].get<int>()<<endl;
        }
    }
}

void addgroup(int clientfd, string str){
    int groupid=stoi(str);
    json js;
    js["msgid"]=ADD_GROUP_MSG;
    js["id"]=g_currentUser.getId();
    js["groupid"]=groupid;
    string buffer=js.dump();

    int len=send(clientfd,buffer.c_str(),buffer.size()+1,0);
    if(len==-1){
        cerr<<"send addgroup msg error ->"<<buffer<<endl;
    }
}

void groupchat(int clientfd, string str){
int idx=str.find(":");
    if(idx==-1){
        cerr<<"groupchat command invalid!"<<endl;
        return;
    }
    
    int groupid = stoi(str.substr(0,idx));
    string message=str.substr(idx+1,str.size()-idx);

    json js;
    js["msgid"]=GROUP_CHAT_MSG;
    js["id"]=g_currentUser.getId();
    js["name"]=g_currentUser.getName();
    js["groupid"]=groupid;
    js["msg"]=message;
    js["time"]=getCurrentTime();
    string buffer = js.dump();

    int len= send(clientfd,buffer.c_str(),buffer.size()+1,0);
    if(len==-1){
        cerr<<"send groupchat msg error ->"<<buffer<<endl;
    }
}

void logout(int clientfd, string str){
    json js;
    js["msgid"]=LOGOUT_MSG;
    js["id"]=g_currentUser.getId();
    string buffer=js.dump();

    int len=send(clientfd,buffer.c_str(),buffer.size()+1,0);
    if(len==-1){
        cerr<<"send logout msg error ->"<<buffer<<endl;
    }
    else{
        isMainMenuRunning=false;
    }
}