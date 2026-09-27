// ============================================================================
//  IPluginApi.h
//  ---------------------------------------------------------------------------
//  QUE ES: La interfaz que el PLUGIN expone al USUARIO.
//  CONTIENE: template IPluginApi<T> con api().
//  PARA QUE: Que el usuario pueda acceder a la API concreta del plugin
//            de forma tipada: engine.plugin<T>().api().
//  QUIEN LO USA: Los plugins (la implementan), el usuario (la consume).
//  NOTA: Es un template, no una clase normal, porque cada plugin tiene
//        su propio tipo de API.
// ============================================================================
#pragma once

namespace Uron::Plugin {

template<typename T>
class IPluginApi {
public:
    virtual ~IPluginApi() = default;
    virtual T* api() = 0;
};

}