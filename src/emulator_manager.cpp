#include <emulator_manager.hpp>
#include <chip8.hpp>
#include <ps1_libretro.hpp>

#include <memory>

std::unique_ptr<IEmulator> EmulatorManager::create_emulator(const Game &game) {
    switch (game.platform) {
    case GamePlatform::CHIP8:
        return std::make_unique<Chip8>(game.metadata);
    case GamePlatform::PS1:
        return std::make_unique<Ps1LibretroAdapter>(game.metadata);
    case GamePlatform::UNKNOWN:
        throw std::runtime_error("Unknown game platform for game: " + game.name);
    }

    return nullptr;
}

void EmulatorManager::refresh_frame() {
    if (!emulator) {
        frame_buffer.clear();
        current_frame_buffer = {};
        return;
    }

    const size_t width = emulator->width();
    const size_t height = emulator->height();
    const size_t pixel_count = width * height;

    if (frame_buffer.size() != pixel_count) {
        frame_buffer.assign(pixel_count, 0);
    }

    current_frame_buffer = std::span<uint32_t>(frame_buffer.data(), frame_buffer.size());
    emulator->render_frame(current_frame_buffer);
}

void EmulatorManager::play_game(const Game &game) {
    emulator.reset();

    emulator = create_emulator(game);

    if (!emulator->load_rom(game.rom_path)) {
        current_game = nullptr;
        status = EmulatorStatus::IDLE;
        refresh_frame();
        return;
    }

    current_game = const_cast<Game*>(&game);
    status = EmulatorStatus::RUNNING;
    refresh_frame();
}

void EmulatorManager::updateKeyboard(uint16_t key, bool state) {
    if (emulator) {
        emulator->updateKeyboard(key, state);
    }
}

void EmulatorManager::pause_game() {
    if (status == EmulatorStatus::RUNNING && emulator) {
        emulator->pause();
        status = EmulatorStatus::PAUSED;
    }
}

void EmulatorManager::resume_game() {
    if (status == EmulatorStatus::PAUSED && emulator) {
        emulator->resume();
        status = EmulatorStatus::RUNNING;
    }
}

void EmulatorManager::stop_game() {
    if (status != EmulatorStatus::IDLE) {
        emulator.reset();
        status = EmulatorStatus::IDLE;
        current_game = nullptr;
        refresh_frame();
    }
}

void EmulatorManager::update(double dt) {
    if (status == EmulatorStatus::RUNNING && emulator) {
        emulator->update(dt);
        refresh_frame();
    }
}

EmulatorState EmulatorManager::get_state() const {
    const size_t width = emulator ? emulator->width() : 0;
    const size_t height = emulator ? emulator->height() : 0;

    return {status, current_game, width, height, current_frame_buffer};
}