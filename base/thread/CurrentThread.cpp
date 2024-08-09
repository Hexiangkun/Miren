#include "base/thread/CurrentThread.h"



namespace Miren
{
namespace base
{
namespace CurrentThread
{
    __thread int t_cachedThreadId = 0;
    __thread char t_cachedThreadIdString[32];
    __thread int t_cachedThreadIdStrLength = 6;
    __thread const char* t_threadName = "unknown";

    static_assert(std::is_same<int, pid_t>::value, "pid_t should be int");
}
    
} // namespace base
}