//
// Created by 37496 on 2024/6/16.
//

#include "base/ErrorInfo.h"
#include <string.h>
#include <cxxabi.h>
#include <stdlib.h>
#include <execinfo.h>

namespace Miren
{
namespace base
{
namespace ErrorInfo
{
    __thread char t_errnobuf[512];

    const char* strerror_tl(int savedErrno)
    {
        return ::strerror_r(savedErrno, t_errnobuf, sizeof t_errnobuf);
    }

    std::string stackTrace(bool demangle) {
        std::string stack;
        const int max_frames = 200;
        void* frame[max_frames];        //指针数组，来保存地址，最多保存len个
        int nptrs = ::backtrace(frame, max_frames); //将栈回溯信息（函数地址）保存到buffer当中
        char** strings = ::backtrace_symbols(frame, nptrs);//将地址转换为函数名称（字符串要*,而指向多个字符串所以再*,因此**）
        if(strings) {
            size_t len = 256;
            char* demangled = demangle ? static_cast<char*>(::malloc(len)) : nullptr ;
            for(int i=1;i<nptrs;i++){
                if(demangle) {
                    char* left_par = nullptr;
                    char* plus = nullptr;
                    for(char *p = strings[i]; *p; ++p) {
                        if(*p == '(') {
                            left_par = p;
                        }
                        else if(*p == '+') {
                            plus = p;
                        }
                    }

                    if(left_par && plus) {
                        *plus = '\0';
                        int status = 0;
                        char* ret = abi::__cxa_demangle(left_par+1, demangled, &len, &status);
                        *plus = '+';
                        if(status == 0) {
                            demangled = ret;
                            stack.append(strings[i], left_par+1);
                            stack.append(demangled);
                            stack.append(plus);
                            stack.push_back('\n');
                            continue;
                        }
                    }
                }

                stack.append(strings[i]);
                stack.push_back('\n');
            }

            free(demangled);
            free(strings);
        }
        return stack;
    }
    
} // namespace ErrorInfo    
} // namespace base
}