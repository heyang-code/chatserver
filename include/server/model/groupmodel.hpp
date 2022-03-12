#ifndef GROUPMODEL_H
#define GROUPMODEL_H
#include "group.hpp"
#include<string>
#include<vector>
//维护群组信息的操作接口方法 ,数据的业务操作层
class GroupModel
{
public:
    //创建群组
    bool createGroup(Group& group);
    //加入群组
    void addGroup(int userid, int groupid, string role);
    //查询用户所在群组信息,查询自己有那些群聊以及每个群聊的成员信息
    vector<Group> queryGroups(int userid);
    //用户群聊业务给群组其他成员群发消息
    vector<int> queryGroupUsers(int userid, int groupod);
    
};


#endif