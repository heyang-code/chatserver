#include "chatservice.hpp"
#include "public.hpp"
#include <muduo/base/Logging.h>
#include <string>
using namespace muduo;
using namespace std;
using namespace std::placeholders;
#include <vector>
#include <map>
#include <iostream>

// 创建lock_guard对象时，它将尝试获取提供给它的互斥锁的所有权。当控制流离开lock_guard对象的作用域时，lock_guard析构并释放互斥量。
// 它的特点如下：
// 创建即加锁，作用域结束自动析构并解锁，无需手工解锁
// 不能中途解锁，必须等作用域结束才解锁
// 不能复制

//获取单例对象的接口
ChatService *ChatService::instance()
{
    static ChatService service;
    return &service;
}

//注册消息类型msgid所对应的Handler回调操作
ChatService::ChatService()
{
    _msgHnadlerMap.insert({LOGIN_MSG, std::bind(&ChatService::login, this, _1, _2, _3)});
    _msgHnadlerMap.insert({REG_MSG, std::bind(&ChatService::reg, this, _1, _2, _3)});
    _msgHnadlerMap.insert({ONE_CHAT_MSG, std::bind(&ChatService::oneChat, this, _1, _2, _3)});
    _msgHnadlerMap.insert({ADD_FRIEND_MSG, std::bind(&ChatService::addFriend, this, _1, _2, _3)});
    _msgHnadlerMap.insert({CREAT_GROUP_MSG, std::bind(&ChatService::creatGroup, this, _1, _2, _3)});
    _msgHnadlerMap.insert({ADD_GROUP_MSG, std::bind(&ChatService::addGroup, this, _1, _2, _3)});
    _msgHnadlerMap.insert({GROUP_CHAT_MSG, std::bind(&ChatService::groupChat, this, _1, _2, _3)});
    _msgHnadlerMap.insert({LOGINOUT_MSG, std::bind(&ChatService::loginout, this, _1, _2, _3)});

    //连接redis服务器
    if (_redis.connect())
    {
        //设置向业务层上报信息的回调
        _redis.init_notify_handler(std::bind(&ChatService::handleRedisSubscribeMessage, this, _1, _2));
    }
}

MsgHandler ChatService::getHnadler(int msgid)
{ //记录错误日志，msgid没有对应的事件处理回调
    auto it = _msgHnadlerMap.find(msgid);
    if (it == _msgHnadlerMap.end())
    {
        //返回一个默认的处理器，空操作
        return [=](const TcpConnectionPtr &conn, json &js, Timestamp time)
        {
            LOG_ERROR << "msgid:" << msgid << "can not find handler";
        };
    }

    else
    {
        return _msgHnadlerMap[msgid];
    }
}

//处理登录业务
void ChatService::login(const TcpConnectionPtr &conn, json &js, Timestamp time)
{
    int id = js["id"];
    string pwd = js["password"];
    User user = _userModel.query(id);
    //数据库里存在这个id 且密码正确
    if (user.getId() == id && user.getPassword() == pwd)
    {

        if (user.getState() == "online")
        {
            //该用户已经登录 ，不允许重复登录
            json response;
            response["msgid"] = LOGIN_MSG_ACK;
            response["errno"] = 2;
            response["errmsg"] = "this account is using , input another";
            conn->send(response.dump());
        }

        else
        {
            //登录成功，记录用户连接信息
            {
                //加锁，利用加作用域来减小锁的粒度，锁的智能指针是局部对象，出作用域就被析构释放解锁
                lock_guard<mutex> lock(_connMutex);
                _userConnMap.insert({id, conn});
            }

            // id用户登录成功后，向redis订阅channel(id) ,以id命名通道
            _redis.subscribe(id);

            //登录成功 ,更新用户状态信息
            user.setState("online");
            _userModel.updateState(user);
            //下面数据都在栈上，线程的栈是独立的
            json response;
            response["msgid"] = LOGIN_MSG_ACK;
            response["errno"] = 0;
            response["id"] = user.getId();
            response["name"] = user.getName();
            cout << "----------96--------" << endl;
            //查询用户是否有离线消息,如果有以键offlinemsg在response中一并回给用户
            vector<string> vec = _offlineMsgModel.query(id);
            if (!vec.empty())
            {
                response["offlinemsg"] = vec;
                //读取该用户的离线消息后，把所有离线消息都删除
                _offlineMsgModel.remove(id);
            }

            //查询该用户的好友信息并返回
            vector<User> userVec = _friendModel.query(id);
            if (!userVec.empty())
            {
                vector<string> vec2;
                for (User &user : userVec)
                {
                    json js;
                    js["id"] = user.getId();
                    js["name"] = user.getName();
                    js["state"] = user.getState();
                    vec2.push_back(js.dump());
                }
                response["friends"] = vec2;
            }

            //查询用户的群组消息
            cout << "----------123--------" << endl;
            vector<Group> groupuserVec = _groupModel.queryGroups(id);
            if (!groupuserVec.empty())
            {
                cout << "----------126--------" << endl;
                vector<string> groupVec;
                //对于群组里的每个群
                for (Group &grp : groupuserVec)
                {
                    json grpjson;
                    grpjson["groupid"] = grp.getId();
                    grpjson["groupname"] = grp.getName();
                    grpjson["groupdesc"] = grp.getDesc();
                    vector<string> userVec;
                    //对于每个群里的所有用户
                    for (GroupUser &user : grp.getUsers())
                    {
                        json groupUserJson;
                        groupUserJson["id"] = user.getId();
                        groupUserJson["name"] = user.getName();
                        groupUserJson["state"] = user.getState();
                        groupUserJson["role"] = user.getRole();
                        userVec.push_back(groupUserJson.dump());
                    }
                    //这里二维的json实际上就是二维的vector<string>。response["groups"]["users"];vector<string> groupVec中
                    //里面的string实际就是vector<string> userVec，只不过userVec赋值给json后又转化为string
                    grpjson["users"] = userVec;
                    groupVec.push_back(grpjson.dump());
                }
                response["groups"] = groupVec;
            }
            cout << "----------155--------" << endl;
            conn->send(response.dump());
        }
    }

    else
    {
        // 该用户不存在，用户名或者密码错误，登录失败
        json response;
        response["msgid"] = LOGIN_MSG_ACK;
        response["errno"] = 1;
        response["errmsg"] = "username or passwoed error";
        conn->send(response.dump());
    }
}

//处理注册业务 name password
void ChatService::reg(const TcpConnectionPtr &conn, json &js, Timestamp time)
{
    string name = js["name"];
    string password = js["password"];

    User user;
    user.setName(name);
    user.setPassword(password);
    bool state = _userModel.insert(user);
    if (state)
    {
        //注册成功
        json response;
        response["msgid"] = REG_MSG_ACK;
        response["errno"] = 0;
        response["id"] = user.getId();
        conn->send(response.dump());
    }

    else
    {
        //注册失败
        json response;
        response["msgid"] = REG_MSG_ACK;
        response["errno"] = 1;
        conn->send(response.dump());
    }
}

//处理客户异常退出
void ChatService::clientCloseException(const TcpConnectionPtr &conn)
{
    User user;
    {
        lock_guard<mutex> lock(_connMutex);
        for (auto it = _userConnMap.begin(); it != _userConnMap.end(); it++)
        {
            if (it->second == conn)
            {
                //从map表删除用户的连接信息
                user.setId(it->first);
                _userConnMap.erase(it);
                break;
            }
        }
    }

    //在redis中取消订阅通道
    _redis.unsubscribe(user.getId());

    //更新用户的状态信息
    user.setState("offline");
    _userModel.updateState(user);
}

void ChatService::reset()
{
    //把online状态的用户重置为offline
    _userModel.resetState();
}

//一对一聊天业务
void ChatService::oneChat(const TcpConnectionPtr &conn, json &js, Timestamp time)
{
    int toid = js["to"];

    {
        lock_guard<mutex> lock(_connMutex);
        auto it = _userConnMap.find(toid);
        if (it != _userConnMap.end())
        {
            // toid在线，转发消息  服务器主动推送消息给toid用户
            it->second->send(js.dump());
            return;
        }
    }

    //查询toid是否在其他服务器在线,如果在线，就向其id通道发布消息
    User user = _userModel.query(toid);
    if (user.getState() == "online")
    {
        _redis.publish(toid, js.dump());
    }

    else
    {
        // toid不在线，存储离线消息
        _offlineMsgModel.insert(toid, js.dump());
    }
}

//添加好友 msgid id  friendid
void ChatService::addFriend(const TcpConnectionPtr &conn, json &js, Timestamp time)
{
    int id = js["id"];
    int friendid = js["friendid"];
    //存储好友信息
    _friendModel.insert(id, friendid);
}

//创建群组业务
void ChatService::creatGroup(const TcpConnectionPtr &conn, json &js, Timestamp time)
{
    int userid = js["id"];
    string name = js["groupname"];
    string desc = js["groupdesc"];

    Group group(userid, name, desc);
    if (_groupModel.createGroup(group))
    {
        //存储群组创建人信息
        _groupModel.addGroup(userid, group.getId(), "creator");
    }
}
//加入群组业务
void ChatService::addGroup(const TcpConnectionPtr &conn, json &js, Timestamp time)
{
    int userid = js["id"];
    int groupid = js["groupid"];
    _groupModel.addGroup(userid, groupid, "normal");
}
//群组聊天业务
void ChatService::groupChat(const TcpConnectionPtr &conn, json &js, Timestamp time)
{
    int userid = js["id"];
    int groupid = js["groupid"];
    //获取该群组的所有其他用户id
    vector<int> userIdVec = _groupModel.queryGroupUsers(userid, groupid);
    lock_guard<mutex> lock(_connMutex);
    for (const auto &id : userIdVec)
    {

        auto it = _userConnMap.find(id);
        if (it != _userConnMap.end())
        {
            //用户在线,通过该Tcp连接给用户发消息
            it->second->send(js.dump());
            return;
        }

        User user = _userModel.query(id);
        if (user.getState() == "online")
        {
            _redis.publish(id, js.dump());
        }
        else
        {
            //离线，则存储离线消息
            _offlineMsgModel.insert(id, js.dump());
        }
    }
}

//注销业务
void ChatService::loginout(const TcpConnectionPtr &conn, json &js, Timestamp time)
{
    int userid = js["id"];
    {
        lock_guard<mutex> lock(_connMutex);
        auto it = _userConnMap.find(userid);
        if (it != _userConnMap.end())
        {
            _userConnMap.erase(it);
        }
    }

    //用户注销，相当于就是下线，在redis中取消订阅消息
    _redis.unsubscribe(userid);

    //更新用户状态信息
    User user;
    user.setId(userid);
    user.setState("offline");
    _userModel.updateState(user);
}

//将跨服务器msg消息发给用户userid
void ChatService::handleRedisSubscribeMessage(int userid, string msg)
{

    lock_guard<mutex> lock(_connMutex);
    auto it = _userConnMap.find(userid);
    if (it != _userConnMap.end())
    {
        it->second->send(msg);
        return;
    }

    //存储离线消息
    _offlineMsgModel.insert(userid, msg);
}