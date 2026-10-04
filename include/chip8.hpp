#pragma once

#include <array>
#include <cstdint>
#include <filesystem>
#include <random>
#include <span>
#include <string>
#include <emulator.hpp>
#include <map>

class Chip8 : public IEmulator {
private:
    std::array<uint8_t, 4096> memory{};
    uint16_t pc = 0x200;
    uint16_t i = 0;
    std::array<uint8_t, 16> v{};
    std::array<uint16_t, 16> stack{};
    uint16_t sp = 0;
    uint8_t delayTimer = 0;
    uint8_t soundTimer = 0;
    std::array<bool, 16> keyboard{};
    std::mt19937 rng{std::random_device{}()};
    std::uniform_int_distribution<int> randomByte{0, 255};
    double accumulator_ = 0.0;
    bool paused_ = false;

    static constexpr std::size_t WIDTH = 64;
    static constexpr std::size_t HEIGHT = 32;

    std::array<std::array<uint8_t, WIDTH>, HEIGHT> screen{};

    uint32_t PRIMARY_COLOR{0xFFFFFFFF};
    uint32_t SECONDARY_COLOR{0xFF000000};

public:
    Chip8(std::map<std::string, std::string>* metadata = nullptr);

    void loadFontset();

    bool load_rom(const std::filesystem::path& path) override;

    void reset() override;

    void pause() override;

    void resume() override;

    void update(double dt) override;

    void render_frame(std::span<uint32_t> buffer) override;

    void updateKeyboard(uint16_t key, bool state) override;

    void cycleFrame();

    uint16_t decode_instruction();

    void process(uint16_t instruction);

    size_t width() override {
        return WIDTH;
    }

    size_t height() override {
        return HEIGHT;
    }
};