#include "offlinemessagemodel.hpp"
#include"db.h"

//储存用户的离线消息
    void OfflineMsgModel::insert(int userid , string msg)
    {
        char sql [1024]={0};
        sprintf(sql,"insert into offlinemessage values(%d,'%s')",userid,msg.c_str());
        MySQL mysql;
        if(mysql.connect())
        {
            mysql.update(sql);  
        }
    }

//查询用户的离线消息
    vector<string> OfflineMsgModel::query(int userid)
    {
        char sql [1024]={0};
        sprintf(sql,"select message from offlinemessage where userid=%d", userid);
        MySQL mysql;
        vector<string> vec;
        if(mysql.connect())
        {              
            MYSQL_RES *res=mysql.query(sql); 
            if(res!=nullptr)
            {
                //把userid的多行离线消息放入vec中
                MYSQL_ROW row;  //这是一个字符串数组类型
                while((row=mysql_fetch_row(res))!=nullptr)
                {
                    vec.push_back(row[0]);
                }
                mysql_free_result(res);
                return vec;
            } 

        }
        return vec;
    }

//删除用户的离线消息
    void OfflineMsgModel::remove(int userid)
    {
        char sql [1024]={0};
        sprintf(sql,"delete from offlinemessage where userid=%d",userid);
        MySQL mysql;
        if(mysql.connect())
        {
            mysql.update(sql);  
        }
    }
