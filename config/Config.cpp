#include "config/Config.h"

namespace Miren
{
namespace config
{
    template<>
    std::string Config::query(const char *query_str, const std::string &default_value) {
        auto it = _map.find(query_str);
        if(it == _map.end()){
            return default_value;
        }
        nlohmann::json node = it->second;
        if(node.type() == nlohmann::json::value_t::string){
            std::string p = node.get<std::string>();
            return p;
        }
        return default_value;
    }

    template<>
    int Config::query<int>(const char* query_str, const int &default_value){
        auto it = _map.find(query_str);
        if(it == _map.end()){
            return default_value;
        }
        auto node = it->second;
        if(node.type() == nlohmann::json::value_t::number_integer){
            const auto p = node.get<int>();
            return p;
        }
        return default_value;
    }

    template<>
    unsigned int Config::query<unsigned int>(const char* query_str, const unsigned int &default_value){
        auto it = _map.find(query_str);
        if(it == _map.end()){
            return default_value;
        }
        auto node = it->second;
        if(node.type() == nlohmann::json::value_t::number_integer){
            const auto p = node.get<unsigned int>();
            return p;
        }
        return default_value;
    }

    template<>
    double Config::query<double>(const char* query_str, const double& default_value){
        auto it = _map.find(query_str);
        if(it == _map.end()){
            return default_value;
        }
        auto node = it->second;
        if(node.type() == nlohmann::json::value_t::number_float){
            const auto p = node.get<double>();
            return p;
        }
        return default_value;
    }

    template<>
    bool Config::query<bool>(const char* query_str, const bool& default_value){
        auto it = _map.find(query_str);
        if(it == _map.end()){
            return default_value;
        }
        auto node = it->second;
        if(node.type() == nlohmann::json::value_t::boolean){
            const auto p = node.get<bool>();
            return p;
        }
        return default_value;
    }
}

} // namespace Miren
