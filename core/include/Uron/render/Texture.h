#pragma once
#include <Uron/Types.h>
#include <string>
#include <vector>

namespace Uron {

enum class TextureFilter {
    Nearest,
    Linear
};

enum class TextureWrap {
    Repeat,
    Clamp,
    Mirror
};

struct TextureDesc {
    TextureFilter filter  = TextureFilter::Linear;
    TextureWrap   wrap    = TextureWrap::Clamp;
    bool          mipmaps = true;
};

class Texture {
public:
    Texture() = default;
    ~Texture();

    Texture(const Texture&)            = delete;
    Texture& operator=(const Texture&) = delete;
    Texture(Texture&&) noexcept;
    Texture& operator=(Texture&&) noexcept;

    bool loadFromFile(const std::string& path, const TextureDesc& desc = {});
    bool loadFromMemory(const u8* pixels, u32 w, u32 h,
                        const TextureDesc& desc = {});

    void destroy();

    bool isValid() const { return m_handle != 0; }
    u32  width()  const { return m_width;  }
    u32  height() const { return m_height; }

    u64 handle() const { return m_handle; }

    const u8* pixels() const { return m_pixels.empty() ? nullptr : m_pixels.data(); }
    u32       stride() const { return m_width * 4; }

private:
    u64 m_handle = 0;
    u32 m_width  = 0;
    u32 m_height = 0;
    std::vector<u8> m_pixels;
};

}