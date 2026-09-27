#include <Uron/Logger.h>
#include <cstdio>
#include <ctime>
#include <mutex>
#include <cstdlib>

namespace Uron {

LogLevel Logger::s_level = LogLevel::Info;

namespace {

std::mutex g_logMutex;

const char* levelToString(LogLevel l) {
    switch (l) {
        case LogLevel::Trace: return "TRACE";
        case LogLevel::Info:  return "INFO";
        case LogLevel::Warn:  return "WARN";
        case LogLevel::Error: return "ERROR";
        case LogLevel::Fatal: return "FATAL";
    }
    return "?";
}

const char* levelColor(LogLevel l) {
    switch (l) {
        case LogLevel::Trace: return "\033[90m";
        case LogLevel::Info:  return "\033[36m";
        case LogLevel::Warn:  return "\033[33m";
        case LogLevel::Error: return "\033[31m";
        case LogLevel::Fatal: return "\033[1;31m";
    }
    return "\033[0m";
}

constexpr const char* RESET = "\033[0m";

}

void Logger::setLevel(LogLevel level) {
    s_level = level;
}

LogLevel Logger::level() {
    return s_level;
}

void Logger::trace(std::string_view msg) { log(LogLevel::Trace, msg); }
void Logger::info (std::string_view msg) { log(LogLevel::Info,  msg); }
void Logger::warn (std::string_view msg) { log(LogLevel::Warn,  msg); }
void Logger::error(std::string_view msg) { log(LogLevel::Error, msg); }
void Logger::fatal(std::string_view msg) { log(LogLevel::Fatal, msg); }

void Logger::log(LogLevel lvl, std::string_view msg) {
    if (static_cast<int>(lvl) < static_cast<int>(s_level)) return;

    std::lock_guard<std::mutex> lock(g_logMutex);

    std::time_t now = std::time(nullptr);
    std::tm tmBuf{};
#if defined(_WIN32)
    localtime_s(&tmBuf, &now);
#else
    localtime_r(&now, &tmBuf);
#endif

    char timeBuf[16];
    std::strftime(timeBuf, sizeof(timeBuf), "%H:%M:%S", &tmBuf);

    std::printf("%s[%s][%s] %.*s%s\n",
                levelColor(lvl),
                timeBuf,
                levelToString(lvl),
                static_cast<int>(msg.size()),
                msg.data(),
                RESET);

    if (lvl == LogLevel::Fatal) {
        std::fflush(stdout);
        std::abort();
    }
}

}