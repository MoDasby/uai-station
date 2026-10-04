#pragma once

#include <cstdint>
#include <filesystem>
#include <span>
#include <map>

class IEmulator {
public:

    virtual ~IEmulator() = default;

    IEmulator([[maybe_unused]] const std::map<std::string, std::string>* metadata = {}) {}

    virtual size_t width() = 0;
    virtual size_t height() = 0;

    virtual bool load_rom(
        const std::filesystem::path& path
    ) = 0;

    virtual void updateKeyboard(
        uint16_t key,
        bool state
    ) = 0;

    virtual void reset() = 0;

    virtual void pause() = 0;

    virtual void resume() = 0;

    virtual void update(double dt) = 0;

    virtual void render_frame(
        std::span<uint32_t> buffer
    ) = 0;
};