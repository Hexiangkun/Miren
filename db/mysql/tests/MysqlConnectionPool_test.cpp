#include "db/mysql/MysqlConnection.h"
#include "db/mysql/MysqlConnectionPool.h"

#include <thread>
#include <mutex>
#include <chrono>
#include <iostream>

std::string sql = "INSERT INTO users (username, email, birthdate, is_active) VALUES ('test2', 'test2@runoob.com', '1988-11-25', false)";

// 未使用连接池单线程
void noPoolFun1(int datasize)
{   
  auto begin = std::chrono::high_resolution_clock::now();

  for (int i = 0; i < datasize; ++i)
  {
      SqlConn::MySqlConnection con;
      
      con.connect("127.0.0.1", "root", "root", "chat");
      con.ExecuteNonQuery(sql);
  }
  auto end = std::chrono::high_resolution_clock::now();

  auto res = std::chrono::duration_cast<std::chrono::milliseconds>(end - begin);

  std::cout << " 未使用连接池，单线程，数据量" << datasize << "，耗时：" << res.count() << "ms" << std::endl;

  std::cout<<"-------------------------------------------------\n";
}

// 使用连接池单线程
void usePoolFun1(int datasize)
{
    auto begin1 = std::chrono::high_resolution_clock::now();

    SqlConn::MysqlConnectionPool *cp = SqlConn::MysqlConnectionPool::getConnectionPool();
    for (int i = 0; i < datasize; ++i)
    { 
        std::shared_ptr<SqlConn::MySqlConnection> sp = cp->getConnection();
        sp->connect(SqlConn::MysqlConnectionPool::_config);
        sp->ExecuteReader(sql);
    }
    auto end1 = std::chrono::high_resolution_clock::now();

    auto res1 = std::chrono::duration_cast<std::chrono::milliseconds>(end1 - begin1);

    std::cout << " 使用连接池，单线程，数据量"<<datasize<<"，耗时：" << res1.count() << "ms" << std::endl;

    std::cout<<"-------------------------------------------------\n";
     
}

// 未使用连接池四线程
void noPoolFun2(int datasize)
{
    int n = datasize / 4;
    std::mutex mu;
    auto begin = std::chrono::high_resolution_clock::now();

    SqlConn::MySqlConnection con;
    con.connect("127.0.0.1", "root", "root", "chat");

    std::thread t1([&]()
              {
    SqlConn::MysqlConnectionPool* cp = SqlConn::MysqlConnectionPool::getConnectionPool();
    (void)cp;
    for(int i = 0; i < n; ++i)
    {          
         {
            std::lock_guard<std::mutex>lock(mu);
            con.ExecuteNonQuery(sql);
         }
 
    } });

    std::thread t2([&]()
              {
    SqlConn::MysqlConnectionPool* cp = SqlConn::MysqlConnectionPool::getConnectionPool();
    (void)cp;
    for(int i = 0; i < n; ++i)
    {          
      {
        std::lock_guard<std::mutex>lock(mu);
        con.ExecuteNonQuery(sql);
      }
 
    } });

    std::thread t3([&]()
              {
    SqlConn::MysqlConnectionPool* cp = SqlConn::MysqlConnectionPool::getConnectionPool();
    (void)cp;
    for(int i = 0; i < n; ++i)
    {          
      {
        std::lock_guard<std::mutex>lock(mu);
        con.ExecuteNonQuery(sql);
      }
    } });

    std::thread t4([&]()
              {
    SqlConn::MysqlConnectionPool* cp = SqlConn::MysqlConnectionPool::getConnectionPool();
    (void)cp;
    for(int i = 0; i < n; ++i)
    {          
      {
        std::lock_guard<std::mutex>lock(mu);
        con.ExecuteNonQuery(sql);
      }
    } });

    t1.join();
    t2.join();
    t3.join();
    t4.join();
    auto end = std::chrono::high_resolution_clock::now();

    auto res = std::chrono::duration_cast<std::chrono::milliseconds>(end - begin);

    std::cout << " 未使用连接池，4线程，数据量"<<datasize <<"，耗时：" << res.count() << "ms" << std::endl;
}



// 使用连接池四线程
void usePoolFun2(int datasize)
{
    int n = datasize / 4;
    auto begin = std::chrono::high_resolution_clock::now();
    
    SqlConn::MySqlConnection con;
    con.connect("127.0.0.1", "root", "root", "chat"); 
    
    std::thread t1([&]()
              {
    SqlConn::MysqlConnectionPool* cp = SqlConn::MysqlConnectionPool::getConnectionPool();
    for(int i = 0; i < n; ++i)
    {  
        std::shared_ptr<SqlConn::MySqlConnection> sp = cp->getConnection();
        sp->ExecuteNonQuery(sql);
    } });

    std::thread t2([&]()
              {
    SqlConn::MysqlConnectionPool* cp = SqlConn::MysqlConnectionPool::getConnectionPool();
    for(int i = 0; i < n; ++i)
    {    
        std::shared_ptr<SqlConn::MySqlConnection> sp = cp->getConnection();
        sp->ExecuteNonQuery(sql);
    } });

    std::thread t3([&]()
              {
    SqlConn::MysqlConnectionPool* cp = SqlConn::MysqlConnectionPool::getConnectionPool();
    for(int i = 0; i < n; ++i)
    {    
        std::shared_ptr<SqlConn::MySqlConnection> sp = cp->getConnection();
        sp->ExecuteNonQuery(sql);
    } });

    std::thread t4([&]()
              {
    SqlConn::MysqlConnectionPool* cp = SqlConn::MysqlConnectionPool::getConnectionPool();
    for(int i = 0; i < n; ++i)
    {    
        std::shared_ptr<SqlConn::MySqlConnection> sp = cp->getConnection();
        sp->ExecuteNonQuery(sql);
    } });

    t1.join();
    t2.join();
    t3.join();
    t4.join();
    auto end = std::chrono::high_resolution_clock::now();
   
    auto res = std::chrono::duration_cast<std::chrono::milliseconds>(end-begin);

    std::cout << " 使用连接池，4线程，数据量"<<datasize <<"，耗时：" <<res.count()<<"ms"<< std::endl;  

}


int main()
{   
    //使用单线程测试数据量 1000、5000、10000
    noPoolFun1(1000);
    noPoolFun1(5000);
    noPoolFun1(10000);

    //使用单线程测试数据量 1000、5000、10000
    usePoolFun1(1000);
    usePoolFun1(5000);
    usePoolFun1(10000);

    //使用4线程测试数据量 1000 
    noPoolFun2(1000);
    usePoolFun2(1000);

    //使用4线程测试数据量 5000 
    noPoolFun2(5000);
    usePoolFun2(5000);

    //使用4线程测试数据量 10000
    noPoolFun2(10000);
    usePoolFun2(10000);

    //使用4线程测试数据量 100000
    noPoolFun2(100000);
    usePoolFun2(100000);

    return 0;
}

