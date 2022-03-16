#include "db.h"
#include <muduo/base/Logging.h>


/* mysql_query(MYSQL *, const char * sql) 函数用于向 MySQL 发送并执行 SQL 语句sql。
对于没有数据返回结果集的 SQL ，如 UPDATE、DELETE 等在执行成功时返回 TRUE，出错时返回 FALSE；对于 SELECT，SHOW，EXPLAIN 
或 DESCRIBE 语句返回一个资源标识符，如果查询执行不正确则返回 FALSE */

// mysql_init
// 这个函数用来分配或者初始化一个MYSQL对象，用于连接mysql服务端。如果你传入的参数是NULL指针，它将自动为你分配一个MYSQL对象，
// 如果这个MYSQL对象是它自动分配的，那么在调用mysql_close的时候，会释放这个对象。那么，这就有可能出现一个问题，就是，
// 当你主动调用mysql_close之后，可能因为某些原因，又调用一次mysql_close,第二次调用，有可能因为mysql已经是野指针导致程序崩溃。
// 另外，在我的实际运用过程中，有出现一些api调用过程中出错时，自动调用了mysql_close的情况。由于这种自动调用不会被程序感知，
// 因此程序主动close的时候会导致崩溃。为了安全起见，建议不要让mysql_init api自动创建MYSQL对象，
// 而由自己管理这个对象，并且传入地址让它完成初始化，这样，即使你多次调用close函数，也不会出现程序崩溃的现象

// 对于每个可以产生一个结果集的命令（比如select、show、describe, explain, check_table等等），
// 发起mysql_query或者mysql_real_query之后，你都需要调用mysql_store_result或者mysql_use_result语句获取一个结果集

// 对于每个可以产生一个结果集的命令（比如select、show、describe, explain, check_table等等），发起mysql_query或者mysql_real_query之后，
// 你都需要调用mysql_store_result或者mysql_use_result语句，处理完结果集后需要使用mysql_free_result释放。
// Mysql_use_result初始化一个取回结果集但是它并不像mysql_store_result那样实际的读取结果放到客户端。相反，
// 每一个列结果都是通过调用mysql_fecth_row独立取回的。它直接从服务器读取一个查询的结果而不是存储它到一个临时表或者一个客户端的缓存里面。
// 因此对于mysql_use_result而言它比mysql_store_result快一些并且使用更少的内存。
// 客户端只有在当前的列或者通信的缓存即将超过max_allowed_packet才申请内存。
// 另外方面有些情况你不能使用mysql_use_result接口，当你对于每一列在客户端要做很多的的处理，或者输出发生到屏幕用户可能通过
// ctrl-s退出，这样会挂起服务器，而阻止其他线程去更新客户端正在获取数据的这些表。
// 当使用mysql_use_result时，你必须执行mysql_fetch_row直到NULL值返回。否则那些没有被获取的列将作为你下个请求的一部分返回。
// 如果你忘了这个部分，对于C的API接口将报错” Commands out of sync; you can'trun this command now”。
// 你最好不要对mysql_use_result的结果，调用mysql_data_seek(),mysql_row_seek(),mysql_row_tell(),mysql_num_rows或者
// mysql_affected_row()，也不要在mysql_use_result没有结束的时候发起其他的请求（当你已经获取所以列以后，
// mysql_num_rows可以正确返回你获取的列数）。
// 一旦你处理完所有的结果集，你必须调用mysql_free_result去释放。

//数据库配置信息
static string server="116.62.103.27";
static string user="root";
static string password="cx5201314?";
static string dbname="chat";

    //初始化数据库连接
    MySQL::MySQL()
    {
        _conn=mysql_init(nullptr);
    }

    //释放数据库连接资源
    MySQL::~MySQL()
    {
        if(_conn!=nullptr)
        {
            mysql_close(_conn);
        }
    }
    //连接数据库
    bool MySQL::connect()
    {
     MYSQL *p=mysql_real_connect(_conn,server.c_str(),user.c_str(),password.c_str(),dbname.c_str(),3306,nullptr,0);
//c和c++代码默认的编码字符是ASCII，如果不设置，从MYSQL上获取的数据的中文将无法正常显示显示？
     if(p!=nullptr) {
     mysql_query(_conn,"set names gbk");   
     LOG_INFO<<"connect mysql success!";
     }
     else 
     {
         LOG_INFO<<"connect mysql fail";
     }

     return p;
    }

    //更新操作
    bool MySQL::update (string sql)
    {
        if(mysql_query(_conn,sql.c_str()))
        {
            LOG_INFO<<__FILE__<<":"<<__LINE__<<":"
            <<sql<<"更新失败";
            return false;
        }
        return true;
    }

    //查询操作
    MYSQL_RES * MySQL::query(string sql)
    {
        if(mysql_query(_conn,sql.c_str()))
        {
            LOG_INFO<<__FILE__<<":"<<__LINE__<<":"
            <<sql<<"查询失败";
            return nullptr;
        }
        return mysql_use_result(_conn);
    }

     MYSQL * MySQL::getConnection()
     {
         return _conn;
     }