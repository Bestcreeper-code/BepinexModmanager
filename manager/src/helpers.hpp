#pragma once

#include "Debug/debug_log.hpp"
#include "config.hpp"
#include <string>
#include <unordered_map>

#ifdef DEBUG_BUILD
    #define errorf(frmt,...) \
        printf("\e[31mERROR: " frmt "\e[0m\n", ##__VA_ARGS__);debug_log("ERROR: " frmt "\n", ##__VA_ARGS__)
#else
    #define errorf(frmt,...)
#endif

#ifdef DEBUG_BUILD
    #define debugf(frmt, ...) \
        printf("\e[36mINFO: " frmt "\e[0m\n", ##__VA_ARGS__);debug_log("INFO: " frmt "\n", ##__VA_ARGS__)
#else
    #define debugf(frmt, ...) 
#endif

#ifdef LOG_INFOS
    #define infof(frmt, ...) \
        printf("\e[36mINFO: " frmt "\e[0m\n", ##__VA_ARGS__);debug_log("INFO: " frmt "\n", ##__VA_ARGS__)
#else
    #define infof(frmt, ...) 
#endif

template <typename K, typename V>
V strmap_get_or_default(const std::unordered_map<std::string, V>& m, const K& key, const V& def) {
    auto it = m.find(key);
    return (it != m.end()) ? it->second : def;
}



#define DARKERGRAY   CLITERAL(Color){ 60, 60, 60, 255 }