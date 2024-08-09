//
// Created by 37496 on 2024/6/4.
//

#ifndef SERVER_TYPES_H
#define SERVER_TYPES_H

#include <string.h>
#include <stdint.h>
#include <string>

#ifndef NDEBUG
#include <assert.h>
#endif

namespace Miren
{
namespace base
{
    inline void MemoryZero(void* p, size_t n) {
        memset(p, 0, n);
    }


// 使用implicit_cast作为static_cast或const_cast的安全版本
//用于在类型层次结构中进行向上转换（即，将指向Foo的指针
//转换为指向SuperclassOfFoo的指针，或将指向Foo的指针
//转换为指向Foo的const指针）。
//当您使用implicit_cast时，编译器会检查转换是否安全。
//在C++要求精确类型匹配而不是可转换为目标类型的参数类型的情况下，这种显式implicit_cast是必需的。
//
//可以推断出From类型，因此使用implicit_cast的首选语法与static_cast等相同：
//
//implicit_cast <ToType> (expr)
//
//implicit_cast本应是C++标准库的一部分，
//但提案提交得太晚了。它可能会在未来进入语言。
    template<typename To, typename From>
    inline To implicit_cast(From const& f)
    {
        return f;
    }

// 当您向上转换（即将指针从 Foo 类型转换为 SuperclassOfFoo 类型）时，使用 implicit_cast<> 是可以的，因为向上转换
// 总是会成功。当您向下转换（即将指针从 Foo 类型转换为 SubclassOfFoo 类型）时，static_cast<> 并不安全，因为
// 您如何知道指针实际上是 SubclassOfFoo 类型？它
// 可能是裸 Foo，也可能是 DifferentSubclassOfFoo 类型。因此，
// 当您向下转换时，应该使用此宏。在调试模式下，我们
// 使用 dynamic_cast<> 来仔细检查向下转换是否合法（如果不正确，我们将失败）。在正常模式下，我们改为执行高效的 static_cast<>
//。因此，在调试模式下进行测试以确保
// 转换合法非常重要！
// 这是代码中唯一应该使用 dynamic_cast<> 的地方。
// 尤其是，您不应该使用 dynamic_cast<> 来
// 执行 RTTI（例如，像这样的代码：
// if (dynamic_cast<Subclass1>(foo)) HandleASubclass1Object(foo);
// if (dynamic_cast<Subclass2>(foo)) HandleASubclass2Object(foo);
// 您应该以其他方式设计代码，这样就不需要这样做了。
    template<typename To, typename From>
    inline To down_cast(From* f)
    {
        // 确保 To 是 From * 的子类型。此测试仅用于编译时类型检查，在运行时对优化构建没有任何开销，因为它将被完全优化。
        if(false) {
            implicit_cast<From*, To>(0);
        }

#if !defined(NDEBUG)
        assert(f == nullptr || dynamic_cast<To>(f) != nullptr);
#endif
        return static_cast<To>(f);
    }
}
}

#endif //SERVER_TYPES_H
