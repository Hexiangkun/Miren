//
// Created by 37496 on 2024/6/16.
//

#ifndef SERVER_ERRORINFO_H
#define SERVER_ERRORINFO_H
#include <string>

namespace Miren
{
namespace base
{
namespace ErrorInfo
{
    const char* strerror_tl(int savedErrno);
    std::string stackTrace(bool demangle = false);  
    
} // namespace ErrorInfo 
} // namespace base
}

#endif //SERVER_ERRORINFO_H
