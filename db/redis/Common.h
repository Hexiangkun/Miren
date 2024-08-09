#pragma once


namespace RedisConn
{
    enum RedisCode
    {
        kSuccess = -100,
        kNormalError = -101,
        kNetworkError = -102,
        kSpaceNotEnough = -103,
    };

    struct RedisConnConfig
    {
        std::string redisHost;
        unsigned int port;
        unsigned int timeoutMs;

        RedisConnConfig() : redisHost("127.0.0.1"), port(6379), timeoutMs(0) {}
        RedisConnConfig(const std::string& ip, unsigned int pt, unsigned int timeout)
                :redisHost(ip),
                port(pt),
                timeoutMs(timeout)
                {
                    
                }
    };
} // namespace redis

