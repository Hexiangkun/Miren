#include "base/Exception.h"
#include "base/ErrorInfo.h"

namespace Miren::base
{
    Exception::Exception(std::string msg) : message_(std::move(msg)),
                stack_(ErrorInfo::stackTrace(false))
    {

    }
}