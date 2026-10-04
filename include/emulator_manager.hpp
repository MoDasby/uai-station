#pragma once

#include <memory>
#include <vector>

#include <emulator.hpp>
#include <game.hpp>

#include <list>
#include <span>

enum class EmulatorStatus
{
    IDLE,
    RUNNING,
    PAUSED
};

struct EmulatorState
{
    EmulatorStatus status;
    Game* current_game;
    size_t width;
    size_t height;
    std::span<uint32_t> current_frame;
};

class EmulatorManager
{

private:
    std::unique_ptr<IEmulator> emulator;
    std::unique_ptr<IEmulator> create_emulator(const Game &game);

    std::vector<uint32_t> frame_buffer;
    std::span<uint32_t> current_frame_buffer;

    Game* current_game = nullptr;

    EmulatorStatus status = EmulatorStatus::IDLE;

    void refresh_frame();

public:
    void play_game(const Game &game);
    void updateKeyboard(uint16_t key, bool state);
    void pause_game();
    void resume_game();
    void stop_game();
    void update(double dt);

    EmulatorStatus get_status() const { return status; }

    EmulatorState get_state() const;

    std::list<Game> list_games() const
    {
        return {
            {"astrododge", "Astrododge", "Jogo de chip8 legal", GamePlatform::CHIP8, nullptr, "assets/games/astro_dodge.ch8"},
            {"re2", "Resident Evil 2", "A survival horror game developed and published by Capcom.", GamePlatform::PS1, nullptr, "assets/games/re/Resident Evil 2 (Disc 1).cue"},
            {"gta2", "Grand Theft Auto 2", "A action-adventure game developed by DMA Design and published by Rockstar Games.", GamePlatform::PS1, nullptr, "assets/games/gta2/gta2.bin"}};
    }
};