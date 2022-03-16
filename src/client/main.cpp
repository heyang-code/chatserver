#include"json.hpp"
#include<iostream>
using namespace std;
#include<vector>
#include<string>
#include<ctime>

using json=nlohmann::json;

#include<unistd.h>
#include<thread>  //c++11的接口，封装了pthread,可以跨平台 
#include<sys/socket.h>
#include<sys/types.h>
#include<netinet/in.h>
#include<arpa/inet.h>
#include<semaphore.h>
#include<atomic>
//atomic对int、char、bool等数据结构进行了原子性封装，在多线程环境中，对std::atomic对象的访问不会造成竞争-冒险。
//利用std::atomic可实现数据结构的无锁设计。并通过这个新的头文件提供了多种原子操作数据类型，例如，atomic_bool,atomic_int等等，
//如果我们在多个线程中对这些类型的共享资源进行操作，编译器将保证这些操作都是原子性的，也就是说，
//确保任意时刻只有一个线程对这个资源进行访问，编译器将保证，多个线程访问这个共享资源的正确性。从而避免了锁的使用，提高了效率


#include"group.hpp"
#include"user.hpp"
#include"public.hpp"

//exit()就是退出，传入的参数是程序退出时的状态码，0表示正常退出，其他表示非正常退出，一般都用-1或者1，定义在stdlib.h中


// 记录当前系统登录的用户信息
User g_currentUser;
// 记录当前登录用户的好友列表信息,服务器返回时保存到本地，而不必每次去服务器读取
vector<User> g_currentUserFriendList;
// 记录当前登录用户的群组列表信息
vector<Group> g_currentUserGroupList;

// 控制主菜单页面程序 登录成功时进入了主菜单，注销登陆时要退出该页面，回到登录注册页面
bool isMainMenuRunning = false;

// 用于读写线程之间的通信
sem_t rwsem;
// 记录登录状态
atomic_bool g_isLoginSuccess{false};


// 接收服务器转发消息的子线程
void readTaskHandler(int clientfd);
// 获取系统时间（聊天信息需要添加时间信息）
string getCurrentTime();
// 主聊天页面程序
void mainMenu(int clinetfd);
// 显示当前登录成功用户的基本信息
void showCurrentUserData();


//客户端程序实现 ，main线程用作发送线程 ，子线程用作接收线程
int main(int argc ,char ** argv)
{
    if(argc<3)
    {   //cerr为标准错误输出，写入到标准错位
        cerr<<"command invaild ! example :  ./chatClient 127.0.0.1 6000"<<endl;
        exit(-1);
    }

    char * ip= argv[1];
    int port=atoi(argv[2]);

    //创建client端的Socket
    int clientfd=socket(AF_INET,SOCK_STREAM,0);
    if(clientfd==-1)
    {
        cerr<<" socket create failed"<<endl;
        exit(-1);
    }

    sockaddr_in server;
    bzero(&server,sizeof(server));  //或者使用 memset一样的

    server.sin_family=AF_INET;
    server.sin_port=htons(port);
    server.sin_addr.s_addr=inet_addr(ip);

    //client 和 server进行连接
    if(-1==connect(clientfd,(sockaddr *)&server,sizeof(server)))
    {
        cerr<<"client connect  server error"<<endl;
        close(clientfd);
        exit(-1);
    }

    //初始化读写线程通信的信号量
    sem_init(&rwsem,0,0);
    
    //连接成功，启动接收线程专门负责接收服务器数据                            
    std::thread readTask(readTaskHandler,clientfd);  //创建子线程对象,就是开启了子线程
    readTask.detach(); //在主线程里面给子线程设置线程分离


    //main线程用于接收用户输入 ，负责发送数据
    for(; ;)
    {
        //显示首页面菜单： 登录、注册、退出
        cout << "========================" << endl;
        cout << "1. login" << endl;
        cout << "2. register" << endl;
        cout << "3. quit" << endl;
        cout << "========================" << endl;
        cout << "choice:";
        int choice = 0;
        cin >> choice; //输入选择后还自带一个输入后的确定选择的回车用于结束这个输入，而cin只能读出整数，换行回车则残留在缓冲区
        cin.get(); // 读掉缓冲区残留的回车 ，否则后面的字符串会读入该回车

        switch (choice)
        {
        case 1: //login业务
        {
            int id = 0;
            char pwd[50] = {0};
            cout << "userid:";
            cin >> id;
            cin.get(); // 读掉缓冲区残留的回车
            cout << "userpassword:";
            //读取字符串时不要用cin，cin会自动忽略开头的空白(空格符，换行符，制表符等)，并从第一个字符开始读
            //遇到下一处空白结束，这样无法保留输入字符串中的空白，getline读取一行，遇到换行符结束读取，换行符也会被读取进来
            //在返回函数结果时换行符又会被丢弃
            cin.getline(pwd, 50);

            json js;
            js["msgid"] = LOGIN_MSG;
            js["id"] = id;
            js["password"] = string(pwd);
            string request = js.dump();

            g_isLoginSuccess = false;

            int len = send(clientfd, request.c_str(), strlen(request.c_str()) + 1, 0);
            if (len == -1)
            {
                cerr << "send login msg error:" << request << endl;
            }

            //阻塞等待信号量，由子线程处理完登录的响应消息后，通知这里 
            sem_wait(&rwsem); 
            //如果登录成功
            if(g_isLoginSuccess)
            {
                //进入聊天主页面菜单
                isMainMenuRunning=true;
                mainMenu(clientfd);
            }       

            break;
        }
        case 2:
        {
            char name[50] = {0};
            char pwd[50] = {0};
            cout << "username:";
            cin.getline(name, 50);
            cout << "userpassword:";
            cin.getline(pwd, 50);
        //给服务器发送注册json
            json js;
            js["msgid"] = REG_MSG;
            js["name"] = name;
            js["password"] = pwd;
            string request = js.dump();

            int len = send(clientfd, request.c_str(), strlen(request.c_str()) + 1, 0);
            if (len == -1)
            {
                cerr << "send reg msg error:" << request << endl;
            }


            sem_wait(&rwsem); 
           
            break;
        } 
           
        case 3:
        {   
            close(clientfd);
            sem_destroy(&rwsem);
            exit(0);
        }
        default:
            cerr<<" invaild input!"<<endl;
            break;
        }



    }

    return 0;
}

//处理登录响应的业务逻辑 
void doLoginResponse(json & responsejs)
{
    //登录失败
    if (0 != responsejs["errno"].get<int>())
    {
        cerr << to_string(responsejs["errmsg"]) << endl;
        g_isLoginSuccess=false;
    }
    //登录成功
    else
    {
        //记录当前用户的id和name
        g_currentUser.setId(responsejs["id"]);
        g_currentUser.setName(responsejs["name"]);

        //记录当前用户的好友列表信息
        if (responsejs.contains("friends"))
        {
            vector<string> vec = responsejs["friends"];
            for (const string &str : vec)
            {
                json js = json::parse(str);
                User user;
                user.setId(js["id"]);
                user.setName(js["name"]);
                user.setState(js["state"]);
                g_currentUserFriendList.push_back(user);
            }
        }

        //记录当前用户的群组信息列表
        if (responsejs.contains("groups"))
        {
            vector<string> vec1 = responsejs["groups"];
            for (const string &groupstr : vec1)
            {
                json grpjs = json::parse(groupstr);
                Group group;
                group.setId(grpjs["groupid"]);
                group.setName(grpjs["groupname"]);
                group.setDesc(grpjs["groupdesc"]);
                vector<string> vec2 = grpjs["users"];
                for (const string &usrstr : vec2)
                {
                    json usrjs = json::parse(usrstr);
                    GroupUser groupuser;
                    groupuser.setId(usrjs["id"]);
                    groupuser.setName(usrjs["name"]);
                    groupuser.setState(usrjs["state"]);
                    groupuser.setRole(usrjs["role"]);
                    group.getUsers().push_back(groupuser);
                }
                g_currentUserGroupList.push_back(group);
            }
        }
        //显示当前用户的基本信息
        showCurrentUserData();

        //显示当前用户的离线消息  个人聊天信息或者群组信息
        if (responsejs.contains("offlinemsg"))
        {
            vector<string> vec = responsejs["offlinemsg"];
            for (string &str : vec)
            {
                json js = json::parse(str);
                // time + [id] + name + " said: " + xxx
                if (ONE_CHAT_MSG == js["msgid"].get<int>())
                {
                    cout << js["time"].get<string>() << " [" << js["id"] << "]" << js["name"].get<string>()
                         << " said: " << js["msg"].get<string>() << endl;
                }
                else
                {
                    cout << "群消息[" << js["groupid"] << "]:" << js["time"].get<string>() << " [" << js["id"] << "]" << js["name"].get<string>()
                         << " said: " << js["msg"].get<string>() << endl;
                }
            }     
        }

        g_isLoginSuccess=true;
    }
}

//处理注册响应的业务逻辑
void doRegResponse(json & responsejs)
{   
    string name=responsejs["name"];
    if(0 != responsejs["errno"].get<int>()) //注册失败
    {
        cerr << name<< " is already exist , register error !" << endl;
    }
    else
    {
        cout << name << " register success, userid is" << responsejs["id"]
             << ", do not forget it" << endl;
    }
}

//子线程-接收线程
void readTaskHandler(int clientfd)
{
    for (;;)
    {
        char buffer[1024] = {0};
        int len = recv(clientfd, buffer, 1024, 0);
        if (len == -1 || len == 0)
        {
            close(clientfd);
            exit(-1);
        }
        json js = json::parse(buffer);
        // 接收ChatServer转发的数据，反序列化生成json数据对象

        //处理登录响应的业务逻辑:如果子线程接收到登录响应，做登录后的信息显示处理，并唤醒主现场主线程进入聊天菜单页面
        if (js["msgid"] == LOGIN_MSG_ACK) 
        {
            //处理登录响应的业务逻辑
            doLoginResponse(js);
            //通知主线程登录结果处理完成
            sem_post(&rwsem);
            continue;
        }
        //处理注册响应
        else if(js["msgid"] == REG_MSG_ACK)
        {   
            doRegResponse(js);
            sem_post(&rwsem);
            continue;

        }
        else if (js["msgid"] == ONE_CHAT_MSG)
        {
            cout << js["time"].get<string>() << " [" << js["id"] << "]" << js["name"].get<string>()
                 << " said: " << js["msg"].get<string>() << endl;
            continue;
        }
        else if (js["msgid"] == GROUP_CHAT_MSG)
        {
            cout << "群消息 "
                 << "[" << js["groupid"] << "] : " << js["time"].get<string>() << " [" << js["id"] << "]" << js["name"].get<string>()
                 << " said: " << js["msg"].get<string>() << endl;
            continue;
        }
        
    }
}

// "help" command handler
void help(int fd = 0, string str = "");
// "chat" command handler
void chat(int, string);
// "addfriend" command handler
void addfriend(int, string);
// "creategroup" command handler
void creategroup(int, string);
// "addgroup" command handler
void addgroup(int, string);
// "groupchat" command handler
void groupchat(int, string);
// "loginout" command handler
void loginout(int, string);

// 系统支持的客户端命令列表
unordered_map<string, string> commandMap = {
    {"help", "显示所有支持的命令,格式help"},
    {"chat", "一对一聊天,格式chat:friendid:message"},
    {"addfriend", "添加好友,格式addfriend:friendid"},
    {"creategroup", "创建群组,格式creategroup:groupname:groupdesc"},
    {"addgroup", "加入群组,格式addgroup:groupid"},
    {"groupchat", "群聊,格式groupchat:groupid:message"},
    {"loginout", "注销,格式loginout"}};

// 系统支持的客户端命令处理函数对      这里的function来指定一个可调用对象类型要会使用
unordered_map<string, function<void(int, string)>> commandHandlerMap = {
    {"help", help},
    {"chat", chat},
    {"addfriend", addfriend},
    {"creategroup", creategroup},
    {"addgroup", addgroup},
    {"groupchat", groupchat},
    {"loginout", loginout}};

void mainMenu(int clientfd)
{
    help();
    char buffer[1024] = {0};

    while (isMainMenuRunning)
    {
        //获取客户端输入命令
        cin.getline(buffer, 1024);
        string commandbuf(buffer);
        string command; //存储命令

        int index = commandbuf.find(":");
        if (-1 == index)
        { // help和loginout命令是没有冒号的
            command = commandbuf;
        }
        else
        {
            command = commandbuf.substr(0, index);
        }
        auto it = commandHandlerMap.find(command);
        if (it == commandHandlerMap.end())
        {
            cerr << " invalid input command !" << endl;
            continue;
        }
        //调用相应命令的事件处理回调，mainMenu对修改封闭，添加新功能不需要修改该函数
        it->second(clientfd, commandbuf.substr(index + 1));
    } // end of for(;;)
}

void help(int clientfd, string str)
{
    cout << "show command list" << endl;
    for (const auto &ele : commandMap)
    {
        cout << ele.first << " : " << ele.second << endl;
    }
    cout << endl;
}

void addfriend(int clientfd, string str)
{
    int friendid = atoi(str.c_str());
    json js;
    js["msgid"] = ADD_FRIEND_MSG;
    js["friendid"] = friendid;
    js["id"] = g_currentUser.getId();
    string buffer = js.dump();
    int len = send(clientfd, buffer.c_str(), strlen(buffer.c_str()), 0);
    if (-1 == len)
    {
        cerr << "send message addfriend error" << endl;
    }
}

void chat(int clientfd, string str)
{
    int index = str.find(":"); // friendid:message
    if (-1 == index)
    {
        cerr << "chat command invaild" << endl;
        return;
    }

    int friendid = atoi(str.substr(0, index).c_str());
    string message(str.substr(index + 1));

    json js;
    js["msgid"] = ONE_CHAT_MSG;
    js["id"] = g_currentUser.getId();
    js["name"] = g_currentUser.getName();
    js["msg"] = message;
    js["to"] = friendid;
    js["time"] = getCurrentTime();
    string buffer = js.dump();
    int len = send(clientfd, buffer.c_str(), strlen(buffer.c_str()) + 1, 0);
    if (-1 == len)
    {
        cerr << "send chat message error " << endl;
    }
}

void creategroup(int clientfd, string str)
{
    int index = str.find(":"); // groupname:groupdesc
    if (-1 == index)
    {
        cerr << "creatGroup command invaild" << endl;
        return;
    }

    string groupname = str.substr(0, index);
    string groupdesc(str.substr(index + 1));

    json js;
    js["msgid"] = CREAT_GROUP_MSG;
    js["id"] = g_currentUser.getId();
    js["groupname"] = groupname;
    js["groupdesc"] = groupdesc;
    string buffer = js.dump();
    int len = send(clientfd, buffer.c_str(), strlen(buffer.c_str()) + 1, 0);
    if (-1 == len)
    {
        cerr << "send creatGroup message error " << endl;
    }
}

void addgroup(int clientfd, string str)
{
    int groupid = atoi(str.c_str());
    json js;
    js["msgid"] = ADD_GROUP_MSG;
    js["groupid"] = groupid;
    js["id"] = g_currentUser.getId();
    string buffer = js.dump();
    int len = send(clientfd, buffer.c_str(), strlen(buffer.c_str()), 0);
    if (-1 == len)
    {
        cerr << "send message addgroup error" << endl;
    }
}

void groupchat(int clientfd, string str)
{
    // groupid:message
    int index = str.find(":");
    if (-1 == index)
    {
        cerr << "groupChat command invaild" << endl;
        return;
    }

    int groupid = atoi(str.substr(0, index).c_str());
    string message(str.substr(index + 1));

    json js;
    js["msgid"] = GROUP_CHAT_MSG;
    js["id"] = g_currentUser.getId();
    js["name"] = g_currentUser.getName();
    js["msg"] = message;
    js["time"] = getCurrentTime();
    js["groupid"] = groupid;
    string buffer = js.dump();
    int len = send(clientfd, buffer.c_str(), strlen(buffer.c_str()) + 1, 0);
    if (-1 == len)
    {
        cerr << "send groupchat message error " << endl;
    }
}

void loginout(int clientfd, string str)
{

    json js;
    js["msgid"] = LOGINOUT_MSG;
    js["id"] = g_currentUser.getId();
    string buffer = js.dump();
    int len = send(clientfd, buffer.c_str(), strlen(buffer.c_str()), 0);
    if (-1 == len)
    {
        cerr << "send message loginout error" << endl;
    }
    else
    {
        //结束mainMenu函数，退出该页面回到登录页面
        isMainMenuRunning = false;
    }
}

// 获取系统时间（聊天信息需要添加时间信息）
string getCurrentTime()
{
    auto tt = std::chrono::system_clock::to_time_t(std::chrono::system_clock::now());
    struct tm *ptm = localtime(&tt);
    char date[60] = {0};
    sprintf(date, "%d-%02d-%02d %02d:%02d:%02d",
            (int)ptm->tm_year + 1900, (int)ptm->tm_mon + 1, (int)ptm->tm_mday,
            (int)ptm->tm_hour, (int)ptm->tm_min, (int)ptm->tm_sec);
    return std::string(date);
}

// 显示当前登录成功用户的基本信息界面
void showCurrentUserData()
{
    //当前用户
    cout << "======================login user======================" << endl;
    cout << "current login user => id:" << g_currentUser.getId() << " name:" << g_currentUser.getName() << endl;
    //好友列表
    cout << "----------------------friend list---------------------" << endl;
    if (!g_currentUserFriendList.empty())
    {
        for (User &user : g_currentUserFriendList)
        {
            cout << user.getId() << " " << user.getName() << " " << user.getState() << endl;
        }
    }
    //群组信息
    cout << "----------------------group list----------------------" << endl;
    if (!g_currentUserGroupList.empty())
    {
        for (Group &group : g_currentUserGroupList)
        {
            cout << group.getId() << " " << group.getName() << " " << group.getDesc() << endl;
            for (GroupUser &user : group.getUsers())
            {
                //群里面的用户信息
                cout << user.getId() << " " << user.getName() << " " << user.getState()
                     << " " << user.getRole() << endl;
            }
        }
    }
    cout << "======================================================" << endl;
}