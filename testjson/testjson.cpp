#include "json.hpp"
using json=nlohmann::json;
#include <iostream>
#include <vector>
#include <map>
#include <string>
using namespace std;

string func(){
    json js;
    js["msg_type"]=2;
    js["from"]="ywc";
    js["to"]="zwx";
    js["msg"]="hello,l like you";
    cout<<js<<endl;
    string sendbuf=js.dump();
    return sendbuf;
}

void func2(){
    json js;
    js["id"]={1,2,3,4,5};
    js["msg"]["ywc"]="你好啊";
    js["msg"]["zwx"]="那我当然好啊";
    //上面的形式相当于下面的形式，因此会被下面的代码覆盖
    js["msg"]={{"ywc","你好个蛋好"},{"zwx","去你的"}};
    cout<<js<<endl;
}

void func3(){
    json js;

    //直接序列化一个vector容器
    vector<int>vec;
    vec.push_back(1);
    vec.push_back(2);
    vec.push_back(3);

    js["list"]=vec;

    //直接序列化一个map容器
    map<int,string>m;
    m.insert({1,"喝喝"});
    m.insert({2,"不要"});
    m.insert({3,"噢噢"});

    js["path"]=m;
    
    cout<<js<<endl;
}

int main(){
    string recbuf=func();
    json recjs=json::parse(recbuf);
    cout<<recjs["msg_type"]<<endl;
    cout<<recjs["from"]<<endl;
    cout<<recjs["to"]<<endl;
    cout<<recjs["msg"]<<endl;
    return 0;
}