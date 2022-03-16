#include"chatserver.hpp"
#include"chatservice.hpp"
#include<iostream>
#include<signal.h>
using namespace std;

//处理服务器ctrl+c结束后，重置客户的状态信息 
void resetHandler(int)
{
    ChatService::instance()->reset(); 
    exit(0);
}


int main(int argc, char ** argv){
    if(argc!=2)
    {
        cerr<<"invaild input , example :  ./ChatServer 6000"<<endl;
        return 0;
    }
    
    signal(SIGINT,resetHandler);

    EventLoop loop; //epoll
    InetAddress addr("127.0.0.1",atoi(argv[1]));
    ChatServer server(&loop,addr,"ChatServer");

    server.start();  //listenfd 通过 epoll_ctl 添加到 epoll上 吧
    loop.loop();    //epoll_wait以阻塞方式等待新用户连接，已连接用户的读写事件等
    return 0;

}