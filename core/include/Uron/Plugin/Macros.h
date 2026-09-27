#pragma once
#include <Uron/Plugin/PluginRegistry.h>
#include <Uron/Plugin/PluginID.h>

#define URON_PLUGIN(NS, CLASS)                                          \
    namespace Uron::Plugin::NS {                                        \
        struct CLASS##Registrar {                                       \
            CLASS##Registrar() {                                        \
                ::Uron::Plugin::PluginRegistry::get()                   \
                    .registerFactory(                                   \
                        ::Uron::Plugin::makePluginID(#NS "." #CLASS),   \
                        []() -> ::Uron::Plugin::IPlugin* {              \
                            return new CLASS();                         \
                        });                                             \
            }                                                           \
        };                                                              \
        static CLASS##Registrar g_##CLASS##Registrar;                   \
    }

#define URON_PLUGIN_ID(NS, CLASS) \
    ::Uron::Plugin::makePluginID(#NS "." #CLASS)
