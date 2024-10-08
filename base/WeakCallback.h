//
// Created by 37496 on 2024/6/24.
//

#ifndef SERVER_WEAKCALLBACK_H
#define SERVER_WEAKCALLBACK_H

#include <functional>
#include <memory>

namespace Miren
{
namespace base
{
    template<typename CLASS, typename... ARGS>
    class WeakCallback
    {
    public:
        WeakCallback(const std::weak_ptr<CLASS>& object,
                     const std::function<void(CLASS*, ARGS...)>& function)
                     :object_(object), function_(function)
        {

        }

        void operator()(ARGS&&... args) const
        {
            std::shared_ptr<CLASS> ptr(object_.lock());
            if(ptr) {
                function_(ptr.get(), std::forward<ARGS>(args)...);
            }
        }

    private:
        std::weak_ptr<CLASS> object_;
        std::function<void(CLASS*, ARGS...)> function_;
    };

    template<typename CLASS, typename... ARGS>
    WeakCallback<CLASS, ARGS...> makeWeakCallback(const std::shared_ptr<CLASS>& object,
                                                  void (CLASS::*function)(ARGS...))
    {
        return WeakCallback<CLASS, ARGS...>(object, function);
    }

    template<typename CLASS, typename... ARGS>
    WeakCallback<CLASS, ARGS...> makeWeakCallback(const std::shared_ptr<CLASS>& object,
                                                  void (CLASS::*function)(ARGS...) const)
    {
        return WeakCallback<CLASS, ARGS...>(object, function);
    }
}
}
#endif //SERVER_WEAKCALLBACK_H
