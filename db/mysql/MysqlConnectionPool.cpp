#include "db/mysql/MysqlConnectionPool.h"
#include<thread>

namespace SqlConn
{

MysqlConnConfig MysqlConnectionPool::_config;

//构造函数私有化，单例
MysqlConnectionPool::MysqlConnectionPool()
{
  //创建初始数量的连接
  for(int i = 0; i < _initSize; ++i)
  {
    MySqlConnection* p = new MySqlConnection();
    // 刷新连接的起始空闲时间
    p->refreshAliveTime();
    _connectionQue.push(p);
    _connectionCnt++;
  }

  //启动一个新的线程，作为连接的生产者,成员方法作为一个单独的线程函数必须绑定this指针
  std::thread produce(std::bind(&MysqlConnectionPool::produceConnectionTask,this));
  //分离线程，设为守护线程角色
  produce.detach();

  // 启动一个新的定时线程，扫描超过maxIdleTime时间的空闲连接，进行回收
  std::thread scanner(std::bind(&MysqlConnectionPool::scannerConnectionTask,this));
  scanner.detach();

}

 //获取连接池对象实例
MysqlConnectionPool* MysqlConnectionPool::getConnectionPool()
{
    static MysqlConnectionPool pool;
    return &pool;
}

//从连接池中获取一个可用空闲连接
std::shared_ptr<MySqlConnection>MysqlConnectionPool::getConnection()
{
    std::unique_lock<std::mutex> lock(_queueMutex);
    if(_connectionQue.empty())
    {
        //若队列为空，则阻塞等待
        if(std::cv_status::timeout == _cv.wait_for(lock, std::chrono::milliseconds(_connectionTimeOut)))
        {
            //若阻塞等待达到最大获取连接时间，仍未获取连接则返回空指针
            if(_connectionQue.empty())
            {
                // LOG("获取空闲连接超时...获取连接失败！");
                return nullptr;
            }
        }
    }

    //阻塞途中被唤醒或队列不为空，则直接获取连接

    //shared_ptr智能指针析构时，会调用connection析构函数，故需要在这里自定义一下shared_ptr的资源释放方式
    std::shared_ptr<MySqlConnection> sp(_connectionQue.front(),
                              [&](MySqlConnection *ptcon)
                              {
                                std::unique_lock<std::mutex> lock_m(_queueMutex);
                                // 刷新连接的起始空闲时间
                                ptcon->refreshAliveTime();
                                _connectionQue.push(ptcon);
                              });

    _connectionQue.pop();
    //消费完连接以后，通知生产者线程检查一下，若队列为空，赶紧生产
    _cv.notify_all();

    return sp;

}

//运行在独立线程中，专门负责生产新连接
void MysqlConnectionPool::produceConnectionTask()
{
  while(1)
  {
    std::unique_lock<std::mutex> lock(_queueMutex);

    while(!_connectionQue.empty())
    {
        //若队列不为空阻塞,并释放锁
        _cv.wait(lock);
    }

    //连接数量没有达到上限，则继续创建
    if (_connectionCnt < _maxSize)
    {
      MySqlConnection *p = new MySqlConnection();
      // 刷新连接的起始空闲时间
      p->refreshAliveTime();
      _connectionQue.push(p);
      _connectionCnt++;
    }

    //通知消费者线程，可以连接
    _cv.notify_all();
  }
}

// 扫描超过maxIdleTime时间的空闲连接，进行回收
void MysqlConnectionPool::scannerConnectionTask()
{
  while(1)
  {
    // 模拟定时效果
    std::this_thread::sleep_for(std::chrono::seconds(_maxIdleTime));

    //扫描整个队列，释放多余连接
    std::unique_lock<std::mutex>lock(_queueMutex);
    while(_connectionCnt > _initSize)
    {
        //获取队首连接
        MySqlConnection *p = _connectionQue.front();
        if(p->getAliveTime() > (_maxIdleTime*1000))
        {
          _connectionQue.pop();
          _connectionCnt--;
          // 释放连接，调用~MySqlConnection()
          delete p;
        }
        else
        {
          //队头元素未超时，则其余都不会超时
          break;
        }
    }
  }
}


} // namespace SqlConn

