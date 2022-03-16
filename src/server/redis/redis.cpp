#include"redis.hpp"
#include<iostream>
using namespace std;

Redis::Redis(): _publish_context(nullptr),_subcribe_context(nullptr)
{
    
}
Redis::~Redis()
{
    if(_publish_context!=nullptr)
    {
        redisFree(_publish_context);
    }
    if(_subcribe_context!=nullptr)
    {
        redisFree(_subcribe_context);       
    }
}

// 连接redis服务器  一个上下文就是一个连接
bool Redis::connect()
{
    //负责publish发布消息的上下文连接
    _publish_context=redisConnect("127.0.0.1",6379);
    if(nullptr==_publish_context)
    {
        cerr<<"connect redis failed "<<endl;
        return false;
    }

    //负责subscribe订阅发布消息的上下文连接
    _subcribe_context=redisConnect("127.0.0.1",6379);
    if(nullptr==_subcribe_context)
    {
        cerr<<"connect redis failed "<<endl;
        return false;
    }
    //在单独的线程中，监听通道上的事件，有消息给业务层上报（订阅监听是一个阻塞事件，必须开多线程）
    thread t( [&] ()  {
        observer_channel_message();
    } );
    t.detach();

    cout<<" connect redis-server succcess"<<endl;
    

    return true;
}

// 向redis指定的通道channel发布消息
bool Redis::publish(int channel, string message)
{   
    //向redis发送命令 PUBLISH %d %s  发布命令执行后立马返回
    redisReply * reply=(redisReply *)redisCommand(_publish_context,"PUBLISH %d %s",channel,message);
    if(reply==nullptr)
    {
        cerr<<"publish command error"<<endl;
        return false;
    }   

    freeReplyObject(reply);
    return true;
    
}

// 向redis指定的通道subscribe订阅消息
 bool Redis::subscribe(int channel)
 {
     
    // SUBSCRIBE命令本身会造成线程阻塞等待通道里面发生消息，这里只做订阅通道，不接收通道消息
    // 通道消息的接收专门在observer_channel_message函数中的独立线程中进行
    // 只负责发送订阅命令，不阻塞接收redis server响应消息，否则和notifyMsg线程抢占响应资源
    //所以我们拆分redisCommand(_subscribe_context,"SUBSCRIBE %d %s",channel,message);该函数首先调用redisAppendCommand
    //函数将命令缓存到本地，然后调用 redisBufferWrite在把命令发送到redis-server上，然后调用
    //redisGetReply并以阻塞方式等待命令的执行结果，因此我们避开第三个函数的调用，直接调用前两个函数给redis发布订阅消息就结束
    //至于阻塞等待所订阅的通道的消息的发生函数redisGetReply则在独立线程中单独调用
      cout<<"-----------redis-78--------------"<<endl;
    if (REDIS_ERR == redisAppendCommand(this->_subcribe_context, "SUBSCRIBE %d", channel))
    {   
        cout<<"-----------redis-81--------------"<<endl;
        cerr << "subscribe command failed!" << endl;
        return false;
    }
    // redisBufferWrite可以循环发送缓冲区，直到缓冲区数据发送完毕（done被置为1）
    int done = 0;
    while (!done)
    {   cout<<"-----------redis-88--------------"<<endl;
        if (REDIS_ERR == redisBufferWrite(this->_subcribe_context, &done))
        {
            cerr << "subscribe command failed!" << endl;
            return false;
        }
    }
    // redisGetReply

    return true;
 }


// 向redis指定的通道unsubscribe取消订阅消息
 bool Redis::unsubscribe(int channel)
 {
    if (REDIS_ERR == redisAppendCommand(this->_subcribe_context, "UNSUBSCRIBE %d", channel))
    {
        cerr << "unsubscribe command failed!" << endl;
        return false;
    }
    // redisBufferWrite可以循环发送缓冲区，直到缓冲区数据发送完毕（done被置为1）
    int done = 0;
    while (!done)
    {
        if (REDIS_ERR == redisBufferWrite(this->_subcribe_context, &done))
        {
            cerr << "unsubscribe command failed!" << endl;
            return false;
        }
    }
    return true;
 }

    // 在独立线程中接收订阅通道中的消息
void Redis::observer_channel_message()
{
    redisReply *reply=nullptr;
    while(REDIS_OK==redisGetReply(this->_subcribe_context,(void **)&reply))
    {
        //订阅收到的消息是一个带3元素的数组element(redisReply结构体的成员。1是指向通道号的指针，2是指向消息的指针)
        if (reply != nullptr && reply->element[2] != nullptr && reply->element[2]->str != nullptr)
        {
            // 给业务层上报通道上发生的消息
            _notify_message_handler(atoi(reply->element[1]->str) , reply->element[2]->str);
        }

        freeReplyObject(reply);
    }

    cerr << ">>>>>>>>>>>>> observer_channel_message quit <<<<<<<<<<<<<" << endl;
    
}

// 初始化向业务层上报通道消息的回调对象 ,redis模块不负责此代码，由你的业务模块来决定给我怎样的业务处理函数
void Redis::init_notify_handler(function<void(int, string)> fn)
{
    this->_notify_message_handler=fn;
}