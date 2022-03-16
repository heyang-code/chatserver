#ifndef GROUP_H
#define GROUP_H
#include<string>
using namespace std;
#include<vector>
#include "groupuser.hpp"

//group的ORM类 数据层
class Group{
public:
    Group(int _id=-1,string _name="",string _desc=""):
    id(_id),name(_name),desc(_desc)
    {

    }

    void setId(int id){this->id=id;};
    void setName(string name){this->name=name;};
    void setDesc(string desc){this->desc=desc;};

    int getId(){ return this->id;};
    string getName(){ return this->name;};
    string getDesc(){return this->desc;};
    vector<GroupUser>& getUsers(){return this->users;};
private:
    int id;
    string name;
    string desc; //群简介
    vector<GroupUser> users;  //群里面所有用户

};


#endif

