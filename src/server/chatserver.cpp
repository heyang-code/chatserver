#include "chatserver.hpp"
#include"chatservice.hpp"
#include "json.hpp"
#include<string>
using namespace std;
using namespace std::placeholders;
using json=nlohmann::json;

ChatServer::ChatServer(EventLoop *loop, const InetAddress &listenAddr, const string &nameArg) 
                        : _server(loop, listenAddr, nameArg), _loop(loop)
{

    /*
    这里这两个函数为什么要用bind绑定，因为回调函数setConnectionCallback的形参是一个函数对象类型，我们在绑定函数回调时
    实参肯定是传一个符合其类型的函数对象指针，但是该函数对象指针本身也是有参数的，函数本身也需要实参的传递，因此我们用
    bind绑定函数对象本身的实参

    什么时候用bind:
    当某一个东西(函数的形参，map/vector容器)接收一个函数对象类型时functional<...>,我们在给他们传函数对象实参时，
    同时也想给函数对象的形参传递实参，就可以使用bind将自身的实参绑定上去

    */

    //给服务器注册用户连接的创建和断开回调
     _server.setConnectionCallback(std::bind(&ChatServer::onConnection,this,_1)); 
    //给服务器注册用户读写事件回调
     _server.setMessageCallback(std::bind(&ChatServer::onMessage,this,_1,_2,_3));
    //设置服务器端的线程数量 1个I/O线程 ，3个工作线程
    _server.setThreadNum(4);


}

//启动服务
void ChatServer::start()
{
    _server.start();
}

//上报连接相关信息的回调函数
void ChatServer::onConnection(const TcpConnectionPtr & conn)
{

    //客户端断开连接
    if(!conn->connected())
    {   
        ChatService::instance()->clientCloseException(conn);
        conn->shutdown();
    }

}

//上报读写事件相关信息的回调函数
void ChatServer::onMessage(const TcpConnectionPtr & conn, Buffer * buffer, Timestamp time)
{   
    //将缓冲区的数据读出保存到string
    string buf=buffer->retrieveAllAsString();

    //数据的反序列化
    json js=json::parse(buf);
    //达到目的 ，完全解耦网络模块和代码和业务模块的代码
    //通过js["msgid"]获取-》业务handler-》conn js time
    //get方法将msgid所对应的值强转为int
    auto msghandler=ChatService::instance()->getHnadler(js["msgid"].get<int>());
    //回调消息绑定好的事件处理器，来执行相应的业务处理
    msghandler(conn,js,time);

}

