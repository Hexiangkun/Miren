
#include "db/redis/RedisConn.h"
#include <iostream>

void test_connect()
{

}

void test_redisCache() {
    RedisConn::RedisConnection* rc = new RedisConn::RedisConnection();
    bool an = rc->connectSvr("127.0.0.1", 6379);
    std::cout << an << std::endl;
    int res = rc->setKeyVal("yunfei", "22");
    if (res) {
        std::cout << "fail! setKeyVal" << std::endl;
        return;
    }
    std::cout << "set success!" << std::endl;
    std::string val;
    res = rc->getKey("yunfei", val);
    if (!res) {
        std::cout << "fail! getKey" << std::endl;
        return;
    }
    std::cout << val << std::endl;

    // rc->lpushList("lan", "Java");
    // rc->lpushList("lan", "Go");
    // rc->lpushList("lan", "C++");

    // std::vector<std::string> ans;
    // res = rc->rangeList("lan", 0, 100, ans);
    // if(res) {
    //     std::cout << "fail! rangeList" << std::endl;
    //     return;
    // }
    // for(auto& it : ans) {
    //     std::cout << it << std::endl;
    // }

    // res = rc->hasKey("yunfei");
    // std::cout << "has " << res << std::endl;

    // res = rc->delKey("yunfei");
    // if (res) {
    //     std::cout << "fail del key!" << std::endl;
    //     return;
    // }
    // std::cout << "del key success!" << std::endl;
}


int main() {
    test_redisCache();
}
