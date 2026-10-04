#pragma once

#include <array>
#include <cstdint>
#include <filesystem>
#include <map>
#include <memory>
#include <span>
#include <string>
#include <vector>

#include <emulator.hpp>
#include <libretro.h>

class Ps1LibretroAdapter : public IEmulator {
public:
    Ps1LibretroAdapter(const std::map<std::string, std::string>* metadata = nullptr);
    ~Ps1LibretroAdapter() override;

    size_t width() override;
    size_t height() override;

    bool load_rom(const std::filesystem::path& path) override;
    void updateKeyboard(uint16_t key, bool state) override;
    void reset() override;
    void pause() override;
    void resume() override;
    void update(double dt) override;
    void render_frame(std::span<uint32_t> buffer) override;

private:
    enum class PixelFormat {
        Unknown,
        Xrgb8888,
        Rgb565,
        ZeroRgb1555,
    };

    using SetEnvironmentFn = void (*)(retro_environment_t cb);
    using SetVideoRefreshFn = void (*)(retro_video_refresh_t cb);
    using SetAudioSampleFn = void (*)(retro_audio_sample_t cb);
    using SetAudioSampleBatchFn = void (*)(retro_audio_sample_batch_t cb);
    using SetInputPollFn = void (*)(retro_input_poll_t cb);
    using SetInputStateFn = void (*)(retro_input_state_t cb);
    using InitFn = void (*)(void);
    using DeinitFn = void (*)(void);
    using ApiVersionFn = unsigned (*)(void);
    using GetSystemInfoFn = void (*)(retro_system_info* info);
    using GetSystemAvInfoFn = void (*)(retro_system_av_info* info);
    using SetControllerPortDeviceFn = void (*)(unsigned port, unsigned device);
    using ResetFn = void (*)(void);
    using RunFn = void (*)(void);
    using LoadGameFn = bool (*)(const retro_game_info* game);
    using UnloadGameFn = void (*)(void);

    struct CoreApi {
        void* handle = nullptr;
        SetEnvironmentFn set_environment = nullptr;
        SetVideoRefreshFn set_video_refresh = nullptr;
        SetAudioSampleFn set_audio_sample = nullptr;
        SetAudioSampleBatchFn set_audio_sample_batch = nullptr;
        SetInputPollFn set_input_poll = nullptr;
        SetInputStateFn set_input_state = nullptr;
        InitFn init = nullptr;
        DeinitFn deinit = nullptr;
        ApiVersionFn api_version = nullptr;
        GetSystemInfoFn get_system_info = nullptr;
        GetSystemAvInfoFn get_system_av_info = nullptr;
        SetControllerPortDeviceFn set_controller_port_device = nullptr;
        ResetFn reset = nullptr;
        RunFn run = nullptr;
        LoadGameFn load_game = nullptr;
        UnloadGameFn unload_game = nullptr;
        retro_get_proc_address_t get_proc_address = nullptr;
    };

    struct FrameState {
        unsigned width = 0;
        unsigned height = 0;
        size_t pitch = 0;
        PixelFormat format = PixelFormat::Unknown;
        std::vector<uint8_t> raw;
        std::vector<uint32_t> argb32;
        bool has_frame = false;
    };

    static Ps1LibretroAdapter* active_instance_;

    static bool environment_trampoline(unsigned cmd, void* data);
    static void video_refresh_trampoline(const void* data, unsigned width, unsigned height, size_t pitch);
    static void audio_sample_trampoline(int16_t left, int16_t right);
    static size_t audio_sample_batch_trampoline(const int16_t* data, size_t frames);
    static void input_poll_trampoline(void);
    static int16_t input_state_trampoline(unsigned port, unsigned device, unsigned index, unsigned id);

    bool handle_environment(unsigned cmd, void* data);
    void handle_video_refresh(const void* data, unsigned width, unsigned height, size_t pitch);
    int16_t handle_input_state(unsigned port, unsigned device, unsigned index, unsigned id) const;
    bool load_core();
    bool resolve_core_symbols();
    void unload_core();
    void update_geometry();
    void clear_frame();
    void convert_to_argb32();
    uint32_t convert_pixel_0rgb1555(uint16_t pixel) const;
    uint32_t convert_pixel_rgb565(uint16_t pixel) const;

    static void* load_symbol(void* handle, const char* symbol);

    std::filesystem::path rom_path_;
    std::filesystem::path core_path_;
    std::filesystem::path system_directory_;
    std::filesystem::path save_directory_;
    std::string rom_path_string_;
    std::string system_directory_string_;
    std::string save_directory_string_;
    std::map<std::string, std::string> metadata_;
    CoreApi core_;
    FrameState frame_;
    std::array<bool, 16> keyboard_{};
    size_t width_ = 320;
    size_t height_ = 240;
    bool core_loaded_ = false;
    bool game_loaded_ = false;
    bool paused_ = false;
};
