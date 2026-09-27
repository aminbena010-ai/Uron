// ============================================================================
//  PluginID.h
//  ---------------------------------------------------------------------------
//  QUE ES: Identificador unico de cada plugin.
//  CONTIENE: using PluginID (uint64_t), makePluginID() constexpr,
//            y los IDs reservados del core (Renderer, Input, Scene).
//  PARA QUE: Que cada plugin tenga un ID unico generado en compilacion,
//            sin colisiones, sin strings en runtime.
//  QUIEN LO USA: IPlugin, PluginManager, PluginRegistry.
//  EJEMPLO:
//     constexpr PluginID id = makePluginID("Physics");
// ============================================================================
#pragma once
#include <cstdint>
#include <type_traits>

namespace Uron::Plugin {

using PluginID = uint64_t;

namespace detail {
// Detecta `static constexpr PluginID ID` en T (estandar de plugins,
// CLAUDE.md §6). Vive aqui para que Engine.h y PluginManager.h lo compartan.
template<typename T, typename = void>
struct HasPluginID : std::false_type {};
template<typename T>
struct HasPluginID<T, std::void_t<decltype(T::ID)>> : std::true_type {};
}

constexpr PluginID makePluginID(const char* str) {
    uint64_t hash = 1469598103934665603ULL;
    while (*str) {
        hash ^= static_cast<uint64_t>(*str++);
        hash *= 1099511628211ULL;
    }
    return hash;
}

namespace CorePlugins {
    constexpr PluginID Renderer = makePluginID("Uron.Core.Renderer");
    constexpr PluginID Input    = makePluginID("Uron.Core.Input");
    constexpr PluginID Scene    = makePluginID("Uron.Core.Scene");
}

}