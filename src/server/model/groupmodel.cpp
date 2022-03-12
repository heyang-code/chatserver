#include "groupmodel.hpp"
#include "db.h"
#include<iostream>

//创建群组
bool GroupModel::createGroup(Group& group)
{
    char sql [1024]={0};
    //这里群id由MYSQL自动生成
    sprintf(sql,"insert into allgroup(groupname,groupdesc) values('%s','%s')",
    group.getName().c_str(),group.getDesc().c_str());
    MySQL mysql;
    if(mysql.connect())
    {
        if(mysql.update(sql))
        {
            group.setId(mysql_insert_id(mysql.getConnection()));
            return true;
        }
    }
    return false;
}
//加入群组
void GroupModel::addGroup(int userid, int groupid, string role)
{
    char sql [1024]={0};
    sprintf(sql,"insert into groupuser values(%d, %d,'%s')",groupid,userid,role.c_str());
    MySQL mysql;
    if(mysql.connect())
    {
        mysql.update(sql);
    }
    
}
//查询用户所在群组信息,查询自己有那些群聊以及每个群聊的成员信息
vector<Group> GroupModel::queryGroups(int userid)
{
    //先由userid在groupuser表中查询该用户所在所有群组信息
    //再由群组信息查询群组其他成员的uerid,和user表联合查询user信息
    char sql[1024]={0};
    vector<Group> groupVec;
    sprintf(sql,"select a.id,a.groupname,a.groupdesc from allgroup as a inner join  groupuser as b on a.id=b.groupid where b.userid=%d",userid);
    std::cout<<"------------groupmodel.cpp----43"<<endl;
    MySQL mysql;
    if(mysql.connect())
    {
        MYSQL_RES * res=mysql.query(sql);
        if(res!=nullptr)
        {   
            MYSQL_ROW row;
            while((row=mysql_fetch_row(res))!=nullptr)
            {
                Group group;
                group.setId(atoi(row[0]));
                group.setName(row[1]);
                group.setDesc(row[2]);
                groupVec.push_back(group);
            }
            mysql_free_result(res);
        }
    }
   std::cout<<"------------groupmodel.cpp----61"<<endl;
    //查询每个群组的用户信息，并将他们存入群组的用户信息容器中
  
    for(auto & grp:groupVec){
       
        sprintf(sql,"select a.id,a.name,a.state,b.grouprole from user a inner join groupuser b on b.userid=a.id where b.groupid=%d",grp.getId());
        std::cout<<"------------groupmodel.cpp----67"<<endl;
        MYSQL_RES * res=mysql.query(sql);
        std::cout<<"------------groupmodel.cpp----69"<<endl;
        if(res!=nullptr)
        {   
            MYSQL_ROW row;
            while((row=mysql_fetch_row(res))!=nullptr)
            {
                GroupUser groupUser;
                groupUser.setId(atoi(row[0]));
                groupUser.setName(row[1]);
                groupUser.setState(row[2]);
                groupUser.setRole(row[3]);
                grp.getUsers().push_back(groupUser);
            }
            mysql_free_result(res);
        }
    
    }
    return groupVec;

}

//用户群聊业务给群组其他成员群发消息
vector<int> GroupModel::queryGroupUsers(int userid, int groupid)
{
    char sql[1024]={0};
    sprintf(sql,"select userid from groupuser where groupid=%d and userid!=%d",groupid,userid);
    MySQL mysql;
    vector<int>  idVec;
    if(mysql.connect())
    {
        MYSQL_RES * res=mysql.query(sql);
        if(res!=nullptr)
        {   
            MYSQL_ROW row;
            while((row=mysql_fetch_row(res))!=nullptr)
            {
                idVec.push_back(atoi(row[0]));
            }
            mysql_free_result(res);
        }
    }
    return idVec;
};