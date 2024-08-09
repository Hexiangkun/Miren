#pragma once

#include <fstream>
#include <list>
#include "third_party/nlohmann/json.hpp"
#include "base/log/Logging.h"

namespace Miren
{
namespace config
{
    static const char* defaultConfigFile = "/root/hxk/server/conf/config.json";
    class Config
    {
    public:
        static Config& getInstance() {
            static Config config1;
            return config1;
        }

        template<class T>
        T query(const char* query_str, const T& default_value);
    private:
        Config()
        {
            std::ifstream in(defaultConfigFile);
            if (!in.is_open()) {
               LOG_FATAL << "read " << defaultConfigFile << " file error";
                return;
            }
            try {
                in >> _json;
            }
            catch (nlohmann::json::exception& e) {
                in.close();
                return;
            }

            if(_json.type() != nlohmann::json::value_t::object ) {
                return;
            }
            std::list<std::pair<std::string, nlohmann::json>> all_nodes;
            listAllMember("", _json, all_nodes);

            for(const auto& it : all_nodes) {
                _map.insert({it.first, it.second});
            }
        }

        void listAllMember(const std::string& prefix, nlohmann::json node,
                           std::list<std::pair<std::string, nlohmann::json>>& output)
        {
            if(prefix.size()) {
                output.push_back({prefix, node});
            }
            if(node.is_object()) {
                for(const auto& it : node.items()) {
                    listAllMember(prefix.empty() ? it.key() : prefix + "." + it.key(),
                                  it.value(), output);
                }
            }
        }

    private:
        nlohmann::json _json;
        std::unordered_map<std::string , nlohmann::json > _map;
    };

    template<typename T>
    const T GET_CONFIG(const char* str, const T& default_val) { return Config::getInstance().query<T>(str,default_val); }
}

}