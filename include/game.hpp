#pragma once

#include <string>
#include <filesystem>
#include <list>
#include <map>

enum class GamePlatform {
    CHIP8,
    PS1,
    UNKNOWN
};

struct Game {
    std::string id;
    std::string name;
    std::string description;
    GamePlatform platform;
    std::map<std::string, std::string>* metadata;

    std::filesystem::path rom_path;
};