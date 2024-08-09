#ifndef BERT_TRY_H
#define BERT_TRY_H

/*
 * This class is modified from facebook folly
 * Thanks to it for handling the nasty void thing!
 */

/*
 * Copyright 2016 Facebook, Inc.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *   http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include <exception>
#include <stdexcept>
#include <cassert>

namespace Miren {

// 定义一个名为Try的模板类，用于封装可能抛出异常的操作结果。
template <typename T>
class Try {
    // 内部枚举类型，表示Try对象的三种状态：无值、有值、有异常。
    enum class State {
        None,
        Exception,
        Value,
    };
public:
    Try() : state_(State::None) {
    }

    Try(const T& t) :
        state_(State::Value),
        value_(t) {
    }

    Try(T&& t) :
        state_(State::Value),
        value_(std::move(t)) {
    }

    Try(std::exception_ptr e) :
        state_(State::Exception),
        exception_(std::move(e)) {
    }

    // move
    Try(Try<T>&& t) :
        state_(t.state_) {
        if (state_ == State::Value)
            new (&value_)T(std::move(t.value_));
        else if (state_ == State::Exception)
            new (&exception_)std::exception_ptr(std::move(t.exception_));
    }

    Try<T>& operator=(Try<T>&& t) {
        if (this == &t)
            return *this;

        this->~Try();// 销毁当前对象

        state_ = t.state_;
        if (state_ == State::Value)
            new (&value_)T(std::move(t.value_));
        else if (state_ == State::Exception)
            new (&exception_)std::exception_ptr(std::move(t.exception_));

        return *this;
    }

    // copy
    Try(const Try<T>& t) :
        state_(t.state_) {
        if (state_ == State::Value)
            new (&value_)T(t.value_);
        else if (state_ == State::Exception)
            new (&exception_)std::exception_ptr(t.exception_);
    }

    Try<T>& operator=(const Try<T>& t) {
        if (this == &t)
            return *this;

        this->~Try();

        state_ = t.state_;
        if (state_ == State::Value)
            new (&value_)T(t.value_);
        else if (state_ == State::Exception)
            new (&exception_)std::exception_ptr(t.exception_);

        return *this;
    }

    ~Try() {
        if (state_ == State::Exception)
            exception_.~exception_ptr();
        else if (state_ == State::Value)
            value_.~T();
    }

    // implicity convertion
    operator const T& () const & {
        return Value();
    }
    operator T& () & { return Value(); }
    operator T&& () && { return std::move(Value()); }

    // get value
    const T& Value() const & {
        Check();
        return value_;
    }

    T& Value() & {
        Check();
        return value_;
    }

    T&& Value() && {
        Check();
        return std::move(value_);
    }

    // get exception
    const std::exception_ptr& Exception() const & {
        if (!HasException())
            throw std::runtime_error("Not exception state");

        return exception_;
    }

    std::exception_ptr& Exception() & {
        if (!HasException())
            throw std::runtime_error("Not exception state");

        return exception_;
    }

    std::exception_ptr&& Exception() && {
        if (!HasException())
            throw std::runtime_error("Not exception state");

        return std::move(exception_);
    }

    bool HasValue() const {
        return state_ == State::Value;
    }
    bool HasException() const {
        return state_ == State::Exception;
    }

    const T& operator*() const {
        return Value();
    }
    T& operator*() {
        return Value();
    }

    struct UninitializedTry {};

    void Check() const {
        if (state_ == State::Exception)
            std::rethrow_exception(exception_);
        else if (state_ == State::None)
            throw UninitializedTry();
    }

    // Amazing! Thanks to folly
    template <typename R>
    R Get() {
        return std::forward<R>(Value());
    }

private:
    State state_;
    union {
        T value_;
        std::exception_ptr exception_;
    };
};

// 对于void类型的特化，因为void不能实例化，所以简化实现。
template <>
class Try<void> {
    enum class State {
        Exception,
        Value,
    };

public:
    Try() :
        state_(State::Value) {
    }

    explicit Try(std::exception_ptr e) :
        state_(State::Exception),
        exception_(std::move(e)) {
    }

    // move
    Try(Try<void>&& t) :
        state_(t.state_) {
        if (state_ == State::Exception)
            new (&exception_)std::exception_ptr(std::move(t.exception_));
    }

    Try<void>& operator=(Try<void>&& t) {
        if (this == &t)
            return *this;

        this->~Try();

        state_ = t.state_;
        if (state_ == State::Exception)
            new (&exception_)std::exception_ptr(std::move(t.exception_));

        return *this;
    }

    // copy
    Try(const Try<void>& t) :
        state_(t.state_) {
        if (state_ == State::Exception)
            new (&exception_)std::exception_ptr(t.exception_);
    }

    Try<void>& operator=(const Try<void>& t) {
        if (this == &t)
            return *this;

        this->~Try();

        state_ = t.state_;
        if (state_ == State::Exception)
            new (&exception_)std::exception_ptr(t.exception_);

        return *this;
    }

    ~Try() {
        if (state_ == State::Exception)
            exception_.~exception_ptr();
    }

    // get exception
    const std::exception_ptr& Exception() const & {
        if (!HasException())
            throw std::runtime_error("Not exception state");

        return exception_;
    }

    std::exception_ptr& Exception() & {
        if (!HasException())
            throw std::runtime_error("Not exception state");

        return exception_;
    }

    std::exception_ptr&& Exception() && {
        if (!HasException())
            throw std::runtime_error("Not exception state");

        return std::move(exception_);
    }

    bool HasValue() const {
        return state_ == State::Value;
    }
    bool HasException() const {
        return state_ == State::Exception;
    }

    void Check() const {
        if (state_ == State::Exception)
            std::rethrow_exception(exception_);
    }

    // Amazing! Thanks to folly
    template <typename R>
    R Get() {
        return std::forward<R>(*this);
    }

private:
    State state_;
    std::exception_ptr exception_;
};

// TryWrapper是一个辅助模板结构体，用于判断类型T是否已经是Try类型，如果是则直接使用，否则包裹成Try<T>。
// TryWrapper<T> : if T is Try type, then Type is T otherwise is Try<T>
template <typename T>
struct TryWrapper {
    using Type = Try<T>;
};

template <typename T>
struct TryWrapper<Try<T>> {
    using Type = Try<T>;
};

/*
类型推导与检查：使用SFINAE（Substitution Failure Is Not An Error，替换失败不是错误）技术，
通过std::enable_if和std::is_same来确保只有当F(Args...)的返回类型Type不是void时，该模板才会被实例化。
try-catch块：
在try块中，尝试调用f函数，并将其结果转发给TryWrapper<Type>::Type构造函数，这意味着如果f执行成功，其结果会被封装进一个成功的Try对象中。
如果在执行f过程中抛出了异常（捕获到std::exception& e），则在catch块中，当前的异常（通过std::current_exception()获取）被封装进一个失败的Try对象中返回。
*/
// Wrap function f(...) return by Try<T>
template <typename F, typename... Args>
typename std::enable_if<
!std::is_same<typename std::result_of<F (Args...)>::type, void>::value,
typename TryWrapper<typename std::result_of<F (Args...)>::type >::Type > ::type
WrapWithTry(F&& f, Args&&... args) {
    using Type = typename std::result_of<F(Args...)>::type;

    try {
        return typename TryWrapper<Type>::Type(std::forward<F>(f)(std::forward<Args>(args)...));
    } catch (std::exception& e) {
        return typename TryWrapper<Type>::Type(std::current_exception());
    }
}
/*
类型推导与检查：这里检查确保F(Args...)的返回类型确实是void，此时启用此模板。
try-catch块：
在try块中直接调用函数f，因为没有返回值需要处理。
如果函数执行期间抛出异常，同样在catch块中捕获并封装当前异常到一个失败的Try<void>对象中。
*/
// Wrap void function f(...) return by Try<void>
template <typename F, typename... Args>
typename std::enable_if <
std::is_same<typename std::result_of<F (Args...)>::type, void>::value,
    Try<void>> ::type
WrapWithTry(F&& f, Args&&... args) {
    try {
        std::forward<F>(f)(std::forward<Args>(args)...);
        return Try<void>();
    } catch (std::exception& e) {
        return Try<void>(std::current_exception());
    }
}

/*
类型推导与检查：使用SFINAE（Substitution Failure Is Not An Error，替换失败不是错误）技术，
通过std::enable_if和std::is_same来确保只有当F()的返回类型Type不是void时，该模板才会被实例化。
try-catch块：
在try块中，尝试调用f函数，并将其结果转发给TryWrapper<Type>::Type构造函数，这意味着如果f执行成功，其结果会被封装进一个成功的Try对象中。
如果在执行f过程中抛出了异常（捕获到std::exception& e），则在catch块中，当前的异常（通过std::current_exception()获取）被封装进一个失败的Try对象中返回。
*/
// f's arg is void, but return Type
// Wrap return value of function Type f(void) by Try<Type>
template <typename F>
typename std::enable_if<
    !std::is_same<typename std::result_of<F ()>::type, void>::value,
    typename TryWrapper<typename std::result_of<F ()>::type >::Type >::type
    WrapWithTry(F&& f, Try<void>&& arg) {
    using Type = typename std::result_of<F()>::type;

    try {
        return typename TryWrapper<Type>::Type(std::forward<F>(f)());
    } catch (std::exception& e) {
        return typename TryWrapper<Type>::Type(std::current_exception());
    }
}

// Wrap return value of function void f(void) by Try<void>
template <typename F>
typename std::enable_if <
    std::is_same<typename std::result_of<F ()>::type, void>::value,
    Try<typename std::result_of<F ()>::type >>::type
    WrapWithTry(F&& f, Try<void>&& arg) {
    try {
        std::forward<F>(f)();
        return Try<void>();
    } catch (std::exception& e) {
        return Try<void>(std::current_exception());
    }
}

} // end namespace Miren

#endif
