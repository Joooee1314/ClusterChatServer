# ClusterChatServer (集群聊天服务器)

[Project Badge: C++11 | Linux | mymuduo | Nginx | Redis | MySQL]

## 🚀 项目简介
这是一个基于 C++11、自研网络库 mymuduo 驱动的高性能集群聊天服务器。项目通过 Nginx 的 stream 模块实现负载均衡，打破单机并发瓶颈；利用 Redis 的发布订阅（Pub/Sub）模式构建跨服务器消息总线，解决了异构节点间的消息路由问题。后端整合 MySQL，确保了用户信息、好友关系及离线消息的可靠持久化。

## 🏗️ 分布式集群架构 (Architecture)

      +------------------+      +------------------+
      |      Client      |      |      Client      |
      |   (zhang san)    |      |     (li si)      |
      +--------+---------+      +--------+---------+
               | (ID: 1)                | (ID: 2)
               v                        v
      +--------+------------------------+---------+
      |           Nginx Load Balancer             |
      |          (TCP Stream Module)              |
      +--------+------------------------+---------+
               | (Round Robin)          |
               v                        v
      +--------+---------+      +--------+---------+
      |  ChatServer 1    |      |  ChatServer 2    |
      | (On Loop 1)      |      | (On Loop 2)      |
      | zhang san online |      |   li si online   |
      +---+----+---------+      +---+----+---------+
          |    |                    |    |
          |    |   Subscribe(1)     |    |
          |    +------------------->+    |
          |        Publish(msg to 1)|    |
          |    <--------------------+    |
          |                              |
      +---v----+--------------------v----+---------+
      |           Redis Message Bus               |
      |          (Pub/Sub Mode)                   |
      +--------+------------------------+---------+
               |                        |
      +--------v---------+      +--------v---------+
      |      MySQL       |      |      MySQL       |
      | (Persistence)    |      | (Persistence)    |
      +------------------+      +------------------+

[消息路由流程]：
1. li si (ID: 2) 连接在 Server 2。
2. li si 向 zhang san (ID: 1) 发消息。
3. Server 2 发现对方不在本机，将消息发布到 Redis 的 "1" 频道。
4. Server 1 订阅了上线用户的频道，收到 Redis 转发的消息后，通过本地 TCP 转发给 zhang san。

## ✨ 核心技术亮点与深度 Bug 排查

* 自研网络库 mymuduo：基于 Reactor 模型与非阻塞 I/O，实现高并发长连接管理。
* 集群扩展性：配置 Nginx 负载均衡实现多节点横向扩展（Scale Out）。
* 跨节点通信：利用 hiredis 封装 Redis 接口，解决异构服务器间的消息路由问题。

[稳定性深度优化]：解决高并发下的“16字节内存污染”漏洞
- 现象：跨线程发送 JSON 消息导致接收端前 16 字节乱码。
- 根因：TcpConnection::send 异步调用时 bind 了局部变量的 c_str() 指针。因 ptmalloc 回收机制，前 16 字节被复用为空闲链表指针。
- 修复：改为 Lambda 表达式值捕获 string 副本，并引入 shared_from_this() 守护对象生命周期。

## 🛠️ 环境要求与快速启动

1. 环境：Linux, CMake, G++(C++11), Nginx(stream), Redis, MySQL, hiredis.
2. 运行：
   - 启动 Redis/MySQL/Nginx。
   - 实例1: ./ChatServer 127.0.0.1 6000
   - 实例2: ./ChatServer 127.0.0.1 6002
   - 客户端: ./ChatClient 127.0.0.1 8000 (Nginx 监听端口)

-----------------------------------------------------------
Github Repository: https://github.com/Joooee1314/ClusterChatServer
If this project helps you, please give it a Star! ⭐
