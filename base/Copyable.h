#pragma once

namespace Miren
{
namespace base
{
    // 通过继承Copyable类，其他类可以获得默认的拷贝构造函数和拷贝赋值运算符重载函数，从而支持对象的拷贝操作
    class Copyable
    {
        protected:
            Copyable() = default;
            ~Copyable() = default;
    };
    
} // namespace base
}