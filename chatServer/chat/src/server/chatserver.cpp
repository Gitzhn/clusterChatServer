#include "chatServer.hpp"
#include "json.hpp"
#include "chatService.hpp"

#include <iostream>
#include <functional>
#include <string>
#include <arpa/inet.h>
using namespace std;
using namespace placeholders;
using json = nlohmann::json;

namespace
{
// 保护服务器，限制单条消息最大长度，防止恶意客户端申请过大内存
constexpr uint32_t MAX_MESSAGE_LENGTH = 64 * 1024; // 64KB，根据业务需要可调整
}

// 初始化聊天服务器对象
ChatServer::ChatServer(EventLoop *loop,
                       const InetAddress &listenAddr,
                       const string &nameArg)
    : _server(loop, listenAddr, nameArg), _loop(loop)
{
    // 注册链接回调
    _server.setConnectionCallback(std::bind(&ChatServer::onConnection, this, _1));

    // 注册消息回调
    _server.setMessageCallback(std::bind(&ChatServer::onMessage, this, _1, _2, _3));

    // 设置线程数量（一个线程占8MB，过大会占大量内存且调度开销大）
    _server.setThreadNum(32);
}

// 启动服务
void ChatServer::start()
{
    _server.start();
}

// 上报链接相关信息的回调函数
void ChatServer::onConnection(const TcpConnectionPtr &conn)
{
    // 客户端断开链接
    if (!conn->connected())
    {
        ChatService::instance()->clientCloseException(conn);
        conn->shutdown();
    }
}

// 上报读写事件相关信息的回调函数
void ChatServer::onMessage(const TcpConnectionPtr &conn,
                           Buffer *buffer,
                           Timestamp time)
{
    // 长度前缀协议：[4字节网络序长度L][L字节JSON文本]
    while (buffer->readableBytes() >= sizeof(uint32_t))
    {
        // 只读头部，不移动读指针
        uint32_t netlen = 0;
        ::memcpy(&netlen, buffer->peek(), sizeof(netlen));
        uint32_t bodyLen = ntohl(netlen);

        if (bodyLen == 0 || bodyLen > MAX_MESSAGE_LENGTH)
        {
            // 非法长度，关闭连接
            cerr << "invalid message length: " << bodyLen << endl;
            conn->shutdown();
            return;
        }

        // 当前buffer中的数据还不够一条完整消息，等待更多数据
        if (buffer->readableBytes() < sizeof(uint32_t) + bodyLen)
        {
            break;
        }

        // 丢弃头部
        buffer->retrieve(sizeof(uint32_t));

        // 取出完整的JSON消息体
        std::string buf(buffer->peek(), bodyLen);
        buffer->retrieve(bodyLen);

        // 打印收到的原始JSON
        cout << buf << endl;

        // 反序列化并分发到业务层
        json js = json::parse(buf);
        auto msgHandler = ChatService::instance()->getHandler(js["msgid"].get<int>());
        msgHandler(conn, js, time);
    }
}