#include "debug_log.hpp"
#include "raylib.h"

#include <chrono>
#include <cstdarg>
#include <cstdio>
#include <ctime>
#include <fstream>
#include <string>




static std::ofstream logger;
static std::ofstream curr_logger;

void init_logger() {
    std::time_t timer = std::time(0);
    std::tm* now = std::localtime(&timer);

    char buffer[80];
    sprintf(buffer, "Logs/%d%d%d-%d%d%d.log", 
        now->tm_year + 1900, now->tm_mon, now->tm_mday, 
        now->tm_hour, now->tm_min, now->tm_sec);

    MakeDirectory("Logs/");

    logger = std::ofstream(buffer, std::ios::app);
    curr_logger = std::ofstream("Logs/Latest.log", std::ios::app);
}

void debug_log(const char* format, ...) {
    if(!format) return;
    
    va_list args;
    va_start(args, format);
    
    static char buffer[512];

    vsnprintf(buffer, sizeof(buffer), format, args);
    
    logger << buffer << std::endl;
    curr_logger << buffer << std::endl;

    va_end(args);
}