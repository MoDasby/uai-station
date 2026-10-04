#include <ps1_libretro.hpp>

#include <algorithm>
#include <cstring>
#include <dlfcn.h>
#include <iostream>
#include <stdexcept>

Ps1LibretroAdapter* Ps1LibretroAdapter::active_instance_ = nullptr;

namespace {
template <typename T>
T load_symbol_or_throw(void* handle, const char* symbol) {
    void* address = dlsym(handle, symbol);
    if (!address) {
        throw std::runtime_error(std::string("missing libretro symbol: ") + symbol);
    }

    return reinterpret_cast<T>(address);
}
}

Ps1LibretroAdapter::Ps1LibretroAdapter(const std::map<std::string, std::string>* metadata)
    : IEmulator(metadata) {
    if (metadata) {
        metadata_.insert(metadata->begin(), metadata->end());
    }

    system_directory_ = std::filesystem::current_path() / "assets" / "bios";
    save_directory_ = std::filesystem::current_path() / "saves";
    core_path_ = std::filesystem::current_path() / "cores" / "mednafen_psx_libretro.so";
    system_directory_string_ = system_directory_.string();
    save_directory_string_ = save_directory_.string();
}

Ps1LibretroAdapter::~Ps1LibretroAdapter() {
    unload_core();
}

size_t Ps1LibretroAdapter::width() {
    return width_;
}

size_t Ps1LibretroAdapter::height() {
    return height_;
}

bool Ps1LibretroAdapter::load_rom(const std::filesystem::path& path) {
    if (!load_core()) {
        return false;
    }

    rom_path_ = std::filesystem::absolute(path).lexically_normal();
    rom_path_string_ = rom_path_.string();

    retro_game_info game_info{};
    game_info.path = rom_path_string_.c_str();
    game_info.data = nullptr;
    game_info.size = 0;
    game_info.meta = nullptr;

    if (!core_.load_game || !core_.load_game(&game_info)) {
        std::cerr << "[ps1] failed to load content: " << rom_path_string_ << std::endl;
        unload_core();
        return false;
    }

    game_loaded_ = true;
    paused_ = false;
    update_geometry();
    clear_frame();
    return true;
}

void Ps1LibretroAdapter::updateKeyboard(uint16_t key, bool state) {
    if (key < keyboard_.size()) {
        keyboard_[key] = state;
    }
}

void Ps1LibretroAdapter::reset() {
    if (core_.reset && game_loaded_) {
        core_.reset();
    }

    paused_ = false;
}

void Ps1LibretroAdapter::pause() {
    paused_ = true;
}

void Ps1LibretroAdapter::resume() {
    paused_ = false;
}

void Ps1LibretroAdapter::update(double) {
    if (paused_) {
        return;
    }

    if (core_.run && game_loaded_) {
        core_.run();
    }
}

void Ps1LibretroAdapter::render_frame(std::span<uint32_t> buffer) {
    if (buffer.empty()) {
        return;
    }

    if (!frame_.has_frame || frame_.argb32.empty()) {
        std::fill(buffer.begin(), buffer.end(), 0xFF000000);
        return;
    }

    const size_t copy_count = std::min(buffer.size(), frame_.argb32.size());
    std::copy_n(frame_.argb32.begin(), copy_count, buffer.begin());

    if (copy_count < buffer.size()) {
        std::fill(buffer.begin() + static_cast<std::ptrdiff_t>(copy_count), buffer.end(), 0xFF000000);
    }
}

bool Ps1LibretroAdapter::environment_trampoline(unsigned cmd, void* data) {
    return active_instance_ && active_instance_->handle_environment(cmd, data);
}

void Ps1LibretroAdapter::video_refresh_trampoline(const void* data, unsigned width, unsigned height, size_t pitch) {
    if (active_instance_) {
        active_instance_->handle_video_refresh(data, width, height, pitch);
    }
}

void Ps1LibretroAdapter::audio_sample_trampoline(int16_t, int16_t) {
}

size_t Ps1LibretroAdapter::audio_sample_batch_trampoline(const int16_t*, size_t frames) {
    return frames;
}

void Ps1LibretroAdapter::input_poll_trampoline(void) {
}

int16_t Ps1LibretroAdapter::input_state_trampoline(unsigned port, unsigned device, unsigned index, unsigned id) {
    if (!active_instance_) {
        return 0;
    }

    return active_instance_->handle_input_state(port, device, index, id);
}

bool Ps1LibretroAdapter::handle_environment(unsigned cmd, void* data) {
    switch (cmd) {
    case RETRO_ENVIRONMENT_GET_SYSTEM_DIRECTORY: {
        auto** value = static_cast<const char**>(data);
        *value = system_directory_string_.c_str();
        return true;
    }

    case RETRO_ENVIRONMENT_GET_SAVE_DIRECTORY: {
        auto** value = static_cast<const char**>(data);
        *value = save_directory_string_.c_str();
        return true;
    }

    case RETRO_ENVIRONMENT_SET_PIXEL_FORMAT: {
        auto* format = static_cast<const enum retro_pixel_format*>(data);
        if (!format) {
            return false;
        }

        switch (*format) {
        case RETRO_PIXEL_FORMAT_XRGB8888:
            frame_.format = PixelFormat::Xrgb8888;
            return true;
        case RETRO_PIXEL_FORMAT_RGB565:
            frame_.format = PixelFormat::Rgb565;
            return true;
        case RETRO_PIXEL_FORMAT_0RGB1555:
            frame_.format = PixelFormat::ZeroRgb1555;
            return true;
        default:
            return false;
        }
    }

    case RETRO_ENVIRONMENT_SET_INPUT_DESCRIPTORS:
    case RETRO_ENVIRONMENT_SET_VARIABLES:
    case RETRO_ENVIRONMENT_SET_CONTROLLER_INFO:
    case RETRO_ENVIRONMENT_SET_GEOMETRY:
    case RETRO_ENVIRONMENT_SET_SUPPORT_NO_GAME:
        return true;

    default:
        return false;
    }
}

void Ps1LibretroAdapter::handle_video_refresh(const void* data, unsigned width, unsigned height, size_t pitch) {
    frame_.width = width;
    frame_.height = height;
    frame_.pitch = pitch;
    frame_.has_frame = true;

    if (width > 0 && height > 0) {
        width_ = width;
        height_ = height;
    }

    if (!data || width == 0 || height == 0) {
        return;
    }

    const size_t expected_bytes = pitch * static_cast<size_t>(height);
    frame_.raw.resize(expected_bytes);
    std::memcpy(frame_.raw.data(), data, expected_bytes);

    convert_to_argb32();
}

int16_t Ps1LibretroAdapter::handle_input_state(unsigned port, unsigned device, unsigned, unsigned id) const {
    if (port != 0 || device != RETRO_DEVICE_JOYPAD || id >= keyboard_.size()) {
        return 0;
    }

    return keyboard_[id] ? 1 : 0;
}

bool Ps1LibretroAdapter::load_core() {
    if (core_loaded_) {
        return true;
    }

    active_instance_ = this;

    core_.handle = dlopen(core_path_.c_str(), RTLD_NOW | RTLD_LOCAL);
    if (!core_.handle) {
        std::cerr << "[ps1] failed to load core: " << dlerror() << std::endl;
        active_instance_ = nullptr;
        return false;
    }

    if (!resolve_core_symbols()) {
        unload_core();
        return false;
    }

    core_.set_environment(environment_trampoline);
    core_.set_video_refresh(video_refresh_trampoline);
    core_.set_audio_sample(audio_sample_trampoline);
    core_.set_audio_sample_batch(audio_sample_batch_trampoline);
    core_.set_input_poll(input_poll_trampoline);
    core_.set_input_state(input_state_trampoline);

    if (core_.init) {
        core_.init();
    }

    core_loaded_ = true;
    return true;
}

bool Ps1LibretroAdapter::resolve_core_symbols() {
    try {
        core_.set_environment = load_symbol_or_throw<Ps1LibretroAdapter::SetEnvironmentFn>(core_.handle, "retro_set_environment");
        core_.set_video_refresh = load_symbol_or_throw<Ps1LibretroAdapter::SetVideoRefreshFn>(core_.handle, "retro_set_video_refresh");
        core_.set_audio_sample = load_symbol_or_throw<Ps1LibretroAdapter::SetAudioSampleFn>(core_.handle, "retro_set_audio_sample");
        core_.set_audio_sample_batch = load_symbol_or_throw<Ps1LibretroAdapter::SetAudioSampleBatchFn>(core_.handle, "retro_set_audio_sample_batch");
        core_.set_input_poll = load_symbol_or_throw<Ps1LibretroAdapter::SetInputPollFn>(core_.handle, "retro_set_input_poll");
        core_.set_input_state = load_symbol_or_throw<Ps1LibretroAdapter::SetInputStateFn>(core_.handle, "retro_set_input_state");
        core_.init = load_symbol_or_throw<Ps1LibretroAdapter::InitFn>(core_.handle, "retro_init");
        core_.deinit = load_symbol_or_throw<Ps1LibretroAdapter::DeinitFn>(core_.handle, "retro_deinit");
        core_.api_version = load_symbol_or_throw<Ps1LibretroAdapter::ApiVersionFn>(core_.handle, "retro_api_version");
        core_.get_system_info = load_symbol_or_throw<Ps1LibretroAdapter::GetSystemInfoFn>(core_.handle, "retro_get_system_info");
        core_.get_system_av_info = load_symbol_or_throw<Ps1LibretroAdapter::GetSystemAvInfoFn>(core_.handle, "retro_get_system_av_info");
        core_.set_controller_port_device = load_symbol_or_throw<Ps1LibretroAdapter::SetControllerPortDeviceFn>(core_.handle, "retro_set_controller_port_device");
        core_.reset = load_symbol_or_throw<Ps1LibretroAdapter::ResetFn>(core_.handle, "retro_reset");
        core_.run = load_symbol_or_throw<Ps1LibretroAdapter::RunFn>(core_.handle, "retro_run");
        core_.load_game = load_symbol_or_throw<Ps1LibretroAdapter::LoadGameFn>(core_.handle, "retro_load_game");
        core_.unload_game = load_symbol_or_throw<Ps1LibretroAdapter::UnloadGameFn>(core_.handle, "retro_unload_game");

        if (void* proc = dlsym(core_.handle, "retro_get_proc_address")) {
            core_.get_proc_address = reinterpret_cast<retro_get_proc_address_t>(proc);
        }
    } catch (const std::exception& error) {
        std::cerr << "[ps1] " << error.what() << std::endl;
        return false;
    }

    if (core_.api_version && core_.api_version() != RETRO_API_VERSION) {
        std::cerr << "[ps1] unsupported libretro api version" << std::endl;
        return false;
    }

    return true;
}

void Ps1LibretroAdapter::unload_core() {
    if (game_loaded_ && core_.unload_game) {
        core_.unload_game();
        game_loaded_ = false;
    }

    if (core_loaded_ && core_.deinit) {
        core_.deinit();
    }

    if (core_.handle) {
        dlclose(core_.handle);
        core_.handle = nullptr;
    }

    core_loaded_ = false;
    active_instance_ = nullptr;
    clear_frame();
}

void Ps1LibretroAdapter::update_geometry() {
    if (!core_.get_system_av_info) {
        return;
    }

    retro_system_av_info info{};
    core_.get_system_av_info(&info);

    if (info.geometry.base_width > 0) {
        width_ = info.geometry.base_width;
    }

    if (info.geometry.base_height > 0) {
        height_ = info.geometry.base_height;
    }

    if (width_ == 0) {
        width_ = 320;
    }

    if (height_ == 0) {
        height_ = 240;
    }

    frame_.argb32.resize(width_ * height_);
}

void Ps1LibretroAdapter::clear_frame() {
    frame_.width = 0;
    frame_.height = 0;
    frame_.pitch = 0;
    frame_.raw.clear();
    frame_.argb32.clear();
    frame_.has_frame = false;
}

void Ps1LibretroAdapter::convert_to_argb32() {
    if (frame_.width == 0 || frame_.height == 0) {
        return;
    }

    frame_.argb32.resize(static_cast<size_t>(frame_.width) * static_cast<size_t>(frame_.height));

    switch (frame_.format) {
    case PixelFormat::Xrgb8888: {
        for (unsigned y = 0; y < frame_.height; ++y) {
            const auto* src = reinterpret_cast<const uint32_t*>(frame_.raw.data() + y * frame_.pitch);
            auto* dst = frame_.argb32.data() + static_cast<size_t>(y) * frame_.width;

            for (unsigned x = 0; x < frame_.width; ++x) {
                dst[x] = 0xFF000000u | (src[x] & 0x00FFFFFFu);
            }
        }
        break;
    }

    case PixelFormat::Rgb565: {
        for (unsigned y = 0; y < frame_.height; ++y) {
            const auto* src = reinterpret_cast<const uint16_t*>(frame_.raw.data() + y * frame_.pitch);
            auto* dst = frame_.argb32.data() + static_cast<size_t>(y) * frame_.width;

            for (unsigned x = 0; x < frame_.width; ++x) {
                dst[x] = convert_pixel_rgb565(src[x]);
            }
        }
        break;
    }

    case PixelFormat::ZeroRgb1555: {
        for (unsigned y = 0; y < frame_.height; ++y) {
            const auto* src = reinterpret_cast<const uint16_t*>(frame_.raw.data() + y * frame_.pitch);
            auto* dst = frame_.argb32.data() + static_cast<size_t>(y) * frame_.width;

            for (unsigned x = 0; x < frame_.width; ++x) {
                dst[x] = convert_pixel_0rgb1555(src[x]);
            }
        }
        break;
    }

    case PixelFormat::Unknown:
    default:
        std::fill(frame_.argb32.begin(), frame_.argb32.end(), 0xFF000000u);
        break;
    }
}

uint32_t Ps1LibretroAdapter::convert_pixel_0rgb1555(uint16_t pixel) const {
    const uint32_t r = static_cast<uint32_t>((pixel >> 10) & 0x1F) * 255u / 31u;
    const uint32_t g = static_cast<uint32_t>((pixel >> 5) & 0x1F) * 255u / 31u;
    const uint32_t b = static_cast<uint32_t>(pixel & 0x1F) * 255u / 31u;
    return 0xFF000000u | (r << 16) | (g << 8) | b;
}

uint32_t Ps1LibretroAdapter::convert_pixel_rgb565(uint16_t pixel) const {
    const uint32_t r = static_cast<uint32_t>((pixel >> 11) & 0x1F) * 255u / 31u;
    const uint32_t g = static_cast<uint32_t>((pixel >> 5) & 0x3F) * 255u / 63u;
    const uint32_t b = static_cast<uint32_t>(pixel & 0x1F) * 255u / 31u;
    return 0xFF000000u | (r << 16) | (g << 8) | b;
}
