//server 和client 的公共文件

#ifndef PUBLIC_H
#define PUBLIC_H

//enmu的作用域能向外扩散一层，即该enum的变量是全局的
enum EnMsgType{
    LOGIN_MSG=1, //登录msg
     LOGIN_MSG_ACK, //登录响应
     LOGINOUT_MSG, //注销消息
    REG_MSG, //注册消息
     REG_MSG_ACK ,   //注册响应
    ONE_CHAT_MSG ,//聊天消息
    ADD_FRIEND_MSG, //添加好友


    CREAT_GROUP_MSG, //创建群组
    ADD_GROUP_MSG,//加入群组
    GROUP_CHAT_MSG //群聊天
};




#endif