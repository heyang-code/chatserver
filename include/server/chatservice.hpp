#ifndef CHATSERVICE_H
#define CHATSERVICE_H

#include<unordered_map>
#include<muduo/net/TcpConnection.h>
#include"json.hpp"
using namespace std;
using namespace muduo;
using namespace muduo::net;
using json=nlohmann::json;
#include<functional>
#include"usermodel.hpp"
#include<mutex>
#include"offlinemessagemodel.hpp"
#include"friendmodel.hpp"
#include"groupmodel.hpp"
#include"redis.hpp"

//function 是一个类模板，用来创建一定形式的可调用对象或者作为某个可调用对象的副本
//下面的function表示声明了一个返回值是void,接收形参为conn, js ,time的可调用对象
//表示处理消息的事件回调方法类型
using MsgHandler=std::function<void(const TcpConnectionPtr& conn, json & js ,Timestamp time)>;

//聊天服务器业务类
class ChatService
{
    public:
        //获取单例对象的接口
        static ChatService* instance();
        //处理登录业务
        void login(const TcpConnectionPtr& conn, json & js ,Timestamp time);
        //处理注销业务
        void loginout(const TcpConnectionPtr& conn, json & js ,Timestamp time);
        //处理注册业务
        void reg(const TcpConnectionPtr& conn, json & js ,Timestamp time);   
        //一对一聊天业务
        void oneChat(const TcpConnectionPtr& conn, json & js ,Timestamp time);
        //添加好友业务
        void addFriend(const TcpConnectionPtr& conn, json & js ,Timestamp time);
        //创建群组业务 
        void creatGroup(const TcpConnectionPtr& conn, json & js ,Timestamp time);
        //加入群组业务
        void addGroup(const TcpConnectionPtr& conn, json & js ,Timestamp time);
        //群组聊天业务
        void groupChat(const TcpConnectionPtr& conn, json & js ,Timestamp time);
        //获取消息对应的处理器
        MsgHandler getHnadler(int msgid);

        //处理客户端异常退出
        void clientCloseException(const TcpConnectionPtr & conn);

        //服务器异常,业务重置方法
        void reset();

        //从 redis消息队列中获取订阅的消息方法
        void handleRedisSubscribeMessage(int userid , string msg);
    private:
        ChatService();

        //存储消息id和其对应的业务处理方法
        unordered_map<int ,MsgHandler> _msgHnadlerMap;

        //存储在线用户的通信连接  该连接被多线程共享读写，必须上锁
        unordered_map<int , TcpConnectionPtr> _userConnMap;

        ///定义互斥锁，保证_userConnMap的线程安全
        mutex _connMutex;


        //数据操作类对象
        UserModel _userModel;
        OfflineMsgModel _offlineMsgModel;
        FriendModel _friendModel;
        GroupModel _groupModel;
        

        //redis操作对象
        Redis _redis;
};

#endif