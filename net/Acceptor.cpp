#include "net/Acceptor.h"
#include "net/EventLoop.h"
#include "net/sockets/InetAddress.h"
#include "net/sockets/SocketsOps.h"
#include "base/log/Logging.h"

#include <errno.h>
#include <fcntl.h>
#include <unistd.h>
namespace Miren
{
    namespace net
    {
        /*
        idleFd_作用防止busy loop：
        如果此时你的最大连接数达到了上限，而accept队列里可能还一直在增加新的连接等你接受，
        muduo用的是epoll的LT模式时，那么如果因为你连接达到了文件描述符的上限，
        此时没有可供你保存新连接套接字描述符的文件符了，
        那么新来的连接就会一直放在accept队列中，于是呼其对应的可读事件就会
        一直触发读事件(因为你一直不读，也没办法读走它)，此时就会造成我们常说的busy loop
        */
        Acceptor::Acceptor(EventLoop* loop, const InetAddress& listenAddr, bool reusePort)
            :loop_(loop), 
            acceptSocket_(sockets::createNoblockingOrDie(listenAddr.family())),     //创建socket fd
            acceptChannel_(loop, acceptSocket_.fd()),       //channel和fd绑定
            listening_(false),
            idleFd_(::open("/dev/null", O_RDONLY | O_CLOEXEC))  //打开一个空洞文件(/dev/null)后返回一空闲的文件描述符！
        {
            assert(idleFd_ >= 0);
            acceptSocket_.setReusePort(reusePort);  //不用等待Time_Wait状态结束
            acceptSocket_.setReuseAddr(true);
            acceptSocket_.bindAddress(listenAddr);      //bind
            acceptChannel_.setReadCallback(std::bind(&Acceptor::handleRead, this)); //当fd可读时调用回调函数hanleRead
        }

        Acceptor::~Acceptor()
        {
            acceptChannel_.disableAll();    //将其从poller监听集合中移除，此时为kDeleted状态
            acceptChannel_.remove();        //将其从EventList events_中移除，此时为kNew状态

            ::close(idleFd_);
        }

        void Acceptor::listen()
        {
            loop_->assertInLoopThread();
            listening_ = true;
            acceptSocket_.listen();
            acceptChannel_.enableReading();     //listen完毕才能读事件
        }

        //sockfd 可读，说明有新的连接到来，执行TcpServer::newConnection
        void Acceptor::handleRead()
        {
            loop_->assertInLoopThread();
            InetAddress peerAddr;
            int connfd = acceptSocket_.accept(&peerAddr);
            if(connfd >= 0) {   //接受连接成功，则执行连接回调
                LOG_TRACE << "Accepts of " << peerAddr.toIpPort();
                if(newConnectionCallback_) {   //回调函数在TcpServer构造函数中设置
                    newConnectionCallback_(connfd, peerAddr);//TcpServer::newConnection
                }
                else {//没有回调函数则关闭client对应的fd
                    sockets::close(connfd);
                }
            }
            else { //接收套接字失败
                LOG_SYSERR << "in Acceptor::handleRead";

                if(errno == EMFILE) { //返回的错误码为EMFILE(超过最大连接数)
                    ::close(idleFd_);//把提前拥有的idleFd_关掉，这样就腾出来文件描述符来接受新连接了
                    idleFd_ = ::accept(acceptSocket_.fd(), nullptr, nullptr);
                    ::close(idleFd_);
                    idleFd_ = ::open("/dev/null", O_RDONLY | O_CLOEXEC);//接受到新连接之后将其关闭，然后我们就再次获得一个空洞文件描述符保存到idleFd_中
                }
            }
        }
    } // namespace net
    
} // namespace Miren
