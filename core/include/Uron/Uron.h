#pragma once

#define URON_VERSION_MAJOR 0
#define URON_VERSION_MINOR 4
#define URON_VERSION_PATCH 0
#define URON_VERSION_STRING "0.4.0"

namespace Uron {
    inline constexpr const char* ENGINE_NAME    = "Uron";
    inline constexpr const char* ENGINE_VERSION = URON_VERSION_STRING;
    inline constexpr const char* ENGINE_TAGLINE = "Ligero, flexible, letal.";
    inline constexpr const char* ENGINE_MASCOT  = "ferret";
    inline constexpr const char* ENGINE_AUTHOR  = "Uron Team";
}

#include <Uron/Types.h>

#include <Uron/math/Vec2.h>
#include <Uron/math/Vec3.h>
#include <Uron/math/Vec4.h>
#include <Uron/math/Mat3.h>
#include <Uron/math/Mat4.h>
#include <Uron/math/Quat.h>
#include <Uron/math/Math.h>

#include <Uron/render/Color.h>
#include <Uron/render/Renderer.h>
#include <Uron/render/Mesh.h>
#include <Uron/render/Texture.h>
#include <Uron/render/Shader.h>
#include <Uron/render/Camera.h>
#include <Uron/render/Transform.h>

#include <Uron/Logger.h>
#include <Uron/Window.h>
#include <Uron/Engine.h>

#include <Uron/scene/Node2D.h>
#include <Uron/scene/Scene2D.h>
#include <Uron/scene/Sprite2D.h>
#include <Uron/scene/Camera2D.h>

#include <Uron/Plugin/IPlugin.h>
#include <Uron/Plugin/PluginID.h>
#include <Uron/Plugin/PluginContext.h>
#include <Uron/Plugin/PluginManager.h>
#include <Uron/Plugin/PluginRegistry.h>
#include <Uron/Plugin/PluginConfig.h>
#include <Uron/Plugin/ApiPlugin.h>
#include <Uron/Plugin/IPluginApi.h>
#include <Uron/Plugin/ServiceRegistry.h>
#include <Uron/Plugin/EventBus.h>
#include <Uron/Plugin/Macros.h>