#include "usermodel.hpp"
#include "db.h"
#include<iostream>
using namespace std;

//mysql_insert_id() 返回给定的 connection 中上一步 INSERT 查询中产生的 AUTO_INCREMENT 的 ID 号。
//如果没有指定connection 则使用上一个打开的连接

//user表的增加方法
 bool UserModel::insert(User & user)
 {
     //组装sql语句
     char sql[1024]={0};
     //sprintf函数打印到字符串sql中 ,id由mysql自动生成
     //注意mysql的user表中字段name是unique的，不允许注册相同的name，否则会注册失败
     sprintf(sql,"insert into user (name ,password ,state) values ('%s', '%s', '%s')",user.getName().c_str()
     ,user.getPassword().c_str(),user.getState().c_str());

     MySQL mysql;
     if(mysql.connect()){ 
//update将sql语句insert into user(name password state) values ('%s', '%s', '%s')发送给mysql执行，为user表添加该用户
         if(mysql.update(sql)){
             //获取插入成功的用户数据生成的主键id
             //mysql_insert_id函数为用户随机生成一个id账号，类似注册时系统生成的QQ号
             user.setId(mysql_insert_id(mysql.getConnection()));   
             return true;
         } 
     }
     return false;

 }


 User UserModel::query(int id)
 {
     //组装sql语句
     char sql[1024]={0};
     //sprintf函数打印到字符串sql中
     sprintf(sql,"select * from user where id=%d",id);
     MySQL mysql;
     if(mysql.connect()){ 

        MYSQL_RES * res=  mysql.query(sql); 
        if(res!=nullptr){
            //获取结果的一行，由于每个id都是独一无二的，只要找到了就仅此一个           
            MYSQL_ROW row= mysql_fetch_row(res);
            if(row!=nullptr)
            {
                User user;
                user.setId(id);
                user.setName(row[1]);
                user.setPassword(row[2]);
                user.setState(row[3]);
                mysql_free_result(res);
                return user;
            }
        }
        
     }
     return User();
 }

 //更新用户的状态信息
bool UserModel::updateState(User user)
{
    //组装sql语句
     char sql[1024]={0};
     //sprintf函数打印到字符串sql中
     sprintf(sql,"update user set state='%s' where id=%d", user.getState().c_str(),user.getId());
     MySQL mysql;
     if(mysql.connect()){ 
//update将sql语句insert into user(name password state) values ('%s', '%s', '%s')发送给mysql执行，为user表添加该用户
         if(mysql.update(sql)){
             
             return true;
         } 
     }
     return false;
}

//重置用户的状态信息
void UserModel::resetState()
{
    //组装sql语句
     char sql[1024]={0};
     //sprintf函数打印到字符串sql中
     sprintf(sql,"update user set state = 'offline' where state='online' ");
     MySQL mysql;
     if(mysql.connect()){ 
//update将sql语句insert into user(name password state) values ('%s', '%s', '%s')发送给mysql执行，为user表添加该用户
        mysql.update(sql);          
                
     }
   
}