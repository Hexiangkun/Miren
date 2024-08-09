#include "rpc/ZookeeperClient.h"
#include "config/Config.h"
#include "log/Logging.h"
#include <semaphore.h>

// #include "mprpcapplication.h"

namespace Miren
{
namespace rpc
{

namespace detail
{
// watcher 观察器
void global_watcher(zhandle_t *zh, int type, int state, const char *path, void *watcherCtx)
{
    //  回调的消息类型是和会话相关的消息类型
    if (type == ZOO_SESSION_EVENT)
    {
        //  zkclient和zkserver连接成功
        if (state == ZOO_CONNECTED_STATE)
        {
            sem_t *sem = (sem_t*)zoo_get_context(zh);
            sem_post(sem);
        }
    }
}
} // namespace detail


ZkClient::ZkClient() : zhandle_(nullptr) {}

ZkClient::~ZkClient()
{
    if (zhandle_ != nullptr) 
    {
        // 如果已经连接成功 则析构时释放资源
        zookeeper_close(zhandle_);
    }
}



/**
 * \brief create a handle to used communicate with zookeeper.
 * 
 * This method creates a new handle and a zookeeper session that corresponds
 * to that handle. Session establishment is asynchronous, meaning that the
 * session should not be considered established until (and unless) an
 * event of state ZOO_CONNECTED_STATE is received.
 * 
 * 此方法创建一个新句柄和一个对应的 zookeeper 会话
 * 到那个句柄。会话建立是 异步 的，这意味着
 * 会话不应被视为已建立，直到（除非）一个
 * 收到状态 ZOO_CONNECTED_STATE 事件。
 * 
 * 
 * 
 **/

// zkclient 启动连接 zkserver
void ZkClient::start()
{
    std::string host = config::Configuration::instance().getConfig("zookeeper")->get("host").template get<std::string>();
    std::string port = config::Configuration::instance().getConfig("zookeeper")->get("port").template get<std::string>();


    // std::string host = MprpcApplication::GetInstance().GetConfig().Load("zookeeperip");
    // std::string port = MprpcApplication::GetInstance().GetConfig().Load("zookeeperprot");
    std::string connstr = host + ":" + port;

    /*
        zookeeper_mt:多线程版本
        zookeeper 的 API 客户端程序提供了三个线程
        ① API 调用线程
        ② 网络 I/O  线程     pthread_create  poll
        ③ watcher 回调线程     pthread_create

        具体过程：
            首先起一个线程(main_thread)执行 zookeeper_init 方法，当执行后会启动另外两个线程
            thread1: 负责网络收发的 I/O 线程 + 心跳机制 timeout 1 / 3 的时间 (在此为 30s, 则心跳为 10s 一次)
            thread2: 负责回调 global_watcher 
    */
    zhandle_ = zookeeper_init(connstr.c_str(), detail::global_watcher, 30000, nullptr, nullptr, 0);
    
    // 上述方法结束并没有建立具体连接，而是先创建了 一个句柄 和 一个对应的 zookeeper 会话
    if (nullptr == zhandle_) 
    {
        LOG_ERROR << "zookeeper_init error!";
        exit(EXIT_FAILURE);
    }

    // 创建一个信号量，让当前句柄绑定一个信号量等待通知
    sem_t sem;
    sem_init(&sem, 0, 0);
    zoo_set_context(zhandle_, &sem);

    // main_thread 阻塞在此，需要等待是否连接成功，成功后会 post sem，此时才确认创建连接成功
    sem_wait(&sem);
    LOG_INFO << "zookeeper_init sucess!";
}


// 在 zkserver 上根据指定 节点path 创建 znode节点
void ZkClient::create(const char *path, const char *data, int datalen, int state)
{
    char path_buffer[128];
    int bufferlen = sizeof(path_buffer);
    int flag;

    //  先判断 path 表示的 znode 结点是否存在，如果存在就不再重复创建了
    flag = zoo_exists(zhandle_, path, 0, nullptr);
    
    //  表示 path 的 znode 结点不存在
    if(flag == ZNONODE)
    {
        
        //  创建指定path的znode结点
        flag = zoo_create(zhandle_, path, data, datalen, 
                            &ZOO_OPEN_ACL_UNSAFE, state, path_buffer, bufferlen);
        if(flag == ZOK)
        {
            LOG_INFO << "znode create success... path:" << path ;
        }
        else
        {
            LOG_ERROR << "flag:" << flag << ". znode create error... path:" << path;
            exit(EXIT_FAILURE);
        }
    }
}


// 根据指定路径，获取 znode节点 的值
std::string ZkClient::getData(const char *path)
{
    char buffer[64];
    int bufferlen = sizeof(buffer);
    int flag = zoo_get(zhandle_, path, 0, buffer, &bufferlen, nullptr);
    if(flag != ZOK)
    {
        LOG_ERROR << "get znode error... path:" << path;
        return "";
    }
    else
    {
        return buffer;
    }
}


} // namespace rpc

} // namespace Miren
