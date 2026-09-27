#define STB_IMAGE_IMPLEMENTATION
#include "stb/stb_image.h"

#include <Uron/render/Texture.h>
#include <Uron/Logger.h>
#include <cstring>

namespace Uron {

Texture::~Texture() {
    destroy();
}

Texture::Texture(Texture&& o) noexcept
    : m_handle(o.m_handle)
    , m_width(o.m_width)
    , m_height(o.m_height)
    , m_pixels(std::move(o.m_pixels))
{
    o.m_handle = 0;
    o.m_width  = 0;
    o.m_height = 0;
}

Texture& Texture::operator=(Texture&& o) noexcept {
    if (this != &o) {
        destroy();
        m_handle = o.m_handle;
        m_width  = o.m_width;
        m_height = o.m_height;
        m_pixels = std::move(o.m_pixels);

        o.m_handle = 0;
        o.m_width  = 0;
        o.m_height = 0;
    }
    return *this;
}

bool Texture::loadFromFile(const std::string& path, const TextureDesc& desc) {
    (void)desc;

    int w = 0, h = 0, channels = 0;
    stbi_set_flip_vertically_on_load(1);

    unsigned char* pixels = stbi_load(path.c_str(), &w, &h, &channels, 4);
    if (!pixels) {
        URON_ERROR(std::string("No se pudo cargar textura: ") + path);
        return false;
    }

    bool ok = loadFromMemory(pixels, static_cast<u32>(w),
                             static_cast<u32>(h), desc);
    stbi_image_free(pixels);

    if (ok) {
        URON_INFO(std::string("Textura cargada: ") + path +
                  " (" + std::to_string(w) + "x" + std::to_string(h) + ")");
    }
    return ok;
}

bool Texture::loadFromMemory(const u8* pixels, u32 w, u32 h,
                             const TextureDesc& desc) {
    (void)desc;

    if (!pixels || w == 0 || h == 0) {
        URON_ERROR("loadFromMemory: pixeles invalidos");
        return false;
    }

    m_width  = w;
    m_height = h;
    m_pixels.resize(static_cast<size_t>(w) * h * 4);
    std::memcpy(m_pixels.data(), pixels, m_pixels.size());
    m_handle = 1;

    return true;
}

void Texture::destroy() {
    m_handle = 0;
    m_width  = 0;
    m_height = 0;
    m_pixels.clear();
}

}