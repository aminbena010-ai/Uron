// ============================================================================
//  Logger.h
//  ---------------------------------------------------------------------------
//  QUE ES: Sistema de logs del motor.
//  CONTIENE: enum LogLevel, clase Logger, macros URON_INFO, URON_ERROR, etc.
//  PARA QUE: Que el motor y los plugins impriman mensajes por consola
//            con niveles (Trace, Info, Warn, Error, Fatal).
//  QUIEN LO USA: El motor internamente, los plugins, y el usuario si quiere.
//  EJEMPLO: URON_INFO("Motor iniciado");
// ============================================================================
#pragma once
#include <functional>
#include <string>
#include <string_view>

namespace Uron {

enum class LogLevel {
    Trace,
    Info,
    Warn,
    Error,
    Fatal
};

class Logger {
public:
    static void setLevel(LogLevel level);
    static LogLevel level();

    static void trace(std::string_view msg);
    static void info (std::string_view msg);
    static void warn (std::string_view msg);
    static void error(std::string_view msg);
    static void fatal(std::string_view msg);

    static void log(LogLevel level, std::string_view msg);

    // Se invoca justo antes de terminar el proceso tras un URON_FATAL.
    // Por defecto: std::exit(EXIT_FAILURE). El handler debe ser rapido;
    // si vuelve, el proceso termina igualmente.
    static void setFatalHandler(std::function<void()> handler);

private:
    static LogLevel s_level;
    static std::function<void()> s_fatalHandler;
};

}

#define URON_TRACE(...) ::Uron::Logger::trace(__VA_ARGS__)
#define URON_INFO(...)  ::Uron::Logger::info (__VA_ARGS__)
#define URON_WARN(...)  ::Uron::Logger::warn (__VA_ARGS__)
#define URON_ERROR(...) ::Uron::Logger::error(__VA_ARGS__)
#define URON_FATAL(...) ::Uron::Logger::fatal(__VA_ARGS__)