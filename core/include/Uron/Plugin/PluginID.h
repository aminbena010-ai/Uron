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

namespace Uron::Plugin {

using PluginID = uint64_t;

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