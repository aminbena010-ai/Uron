#pragma once
#include <Uron/Plugin/PluginID.h>

namespace Uron::Plugin {

struct Event;
class  PluginContext;

class IPlugin {
public:
    virtual ~IPlugin() = default;

    virtual PluginID    id()      const = 0;
    virtual const char* name()    const = 0;
    virtual const char* version() const = 0;
    virtual const char* author()  const = 0;

    virtual bool onLoad(PluginContext& ctx)              = 0;
    virtual void onUnload(PluginContext& ctx)            = 0;
    virtual void onUpdate(PluginContext& ctx, float dt)  = 0;
    virtual void onEvent(PluginContext&, const Event&) {}

    bool isLoaded() const { return m_loaded; }
    void markLoaded(bool v) { m_loaded = v; }

private:
    bool m_loaded = false;
};

}