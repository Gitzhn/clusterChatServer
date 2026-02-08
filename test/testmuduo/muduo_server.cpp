/*
提供TcpServer类和TcpClient类

epoll + 线程池
好处：能够把网络I/O的代码和业务代码分开
*/

#include <muduo/net/TcpServer.h>
#include <muduo/net/EventLoop.h>
#include <iostream>
#include <functional> // 绑定器都在functional中
#include <string>

using namespace std;

/*
服务器程序：
1.组合TcpServer对象
2.创建Eventloop事件循环指针，方便关闭服务器程序
3.明确TcpServer构造函数的参数（没有默认构造）
4.在当前服务器类的构造函数中，注册处理连接的回调函数和处理读写事件的回调函数
5.设置合适的服务器端线程数量，muduo库会自动分配I/O线程和worker线程

*/

class ChatServer{
public:
    ChatServer(muduo::net::EventLoop *loop, // 事件循环
                const muduo::net::InetAddress &listenAddr, // IP + Port
                const string &nameArg) // 服务器的名字
            : _server(loop,listenAddr,nameArg)
    {
        // 给服务器注册用户连接的创建和断开回调
        // placeholders是参数占位符
        _server.setConnectionCallback(std::bind(&ChatServer::onConnection,this,placeholders::_1));

        // 给服务器注册用户读写事件回调
        _server.setMessageCallback(std::bind(&ChatServer::onMessage,this,placeholders::_1,placeholders::_2,placeholders::_3));
                
        // 设置服务器端的线程数量（muduo网络库会自适应有多少个线程？？？）
        _server.setThreadNum(4); // 1个I/O线程，监听新用户的连接事件；3个worker线程，响应用户的断开连接

    }

    // 启动服务器
    void start()
    {
        cout<<"ChatServer start"<<endl;
        _server.start(); // 启动服务器
    }



private:
    // 专门处理用户的连接创建和断开     epoll   listenfd    accept
    void onConnection(const muduo::net::TcpConnectionPtr& conn)
    {
        if(conn->connected()){
            cout<<conn->peerAddress().toIpPort()<<" -> "<<conn->localAddress().toIpPort()<<" state:online"<<endl;
        }else{
            cout<<conn->peerAddress().toIpPort()<<" -> "<<conn->localAddress().toIpPort()<<" state:offline"<<endl;
            conn->shutdown(); // 关闭连接close(fd)
            // _loop->quit(); // 退出事件循环
        }
    }

    // 专门处理用户的读写事件
    void onMessage(const muduo::net::TcpConnectionPtr& conn, // 连接
                   muduo::net::Buffer* buffer, //缓冲区
                    muduo::Timestamp time) // 接收到数据的时间
    {
        string buf = buffer->retrieveAllAsString(); // 读取缓冲区中的所有数据
        cout<<"recv data:"<<buf<<" time:"<<time.toString()<<endl;
        conn->send(buf); // 回显数据
    }
    muduo::net::TcpServer _server;
    muduo::net::EventLoop *_loop;
};

int main(){
    muduo::net::EventLoop loop; // epoll
    muduo::net::InetAddress addr("127.0.0.1",6000); // IP + Port
    ChatServer server(&loop,addr,"ChatServer"); // 服务器对象
    server.start(); // listenfd epoll_ctl=>epoll
    loop.loop(); // epoll_wait以阻塞方式等待新用户连接，已连接用户的读写事件等
    return 0;
}