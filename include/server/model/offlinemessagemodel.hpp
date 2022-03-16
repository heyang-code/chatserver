#ifndef OFFLINEMESSAGEMODEL_H
#define OFFLINEMESSAGEMODEL_H
#include<vector>
#include<string>
using namespace  std;

    



class OfflineMsgModel
{
 public:
    //储存用户的离线消息
    void insert(int userid , string msg);

    //查询用户的离线消息
    vector<string> query(int userid);

    //删除用户的离线消息
    void remove(int userid);

};


#endif