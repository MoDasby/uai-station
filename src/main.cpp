#include <algorithm>
#include <array>
#include <cctype>
#include <vector>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <iterator>
#include <limits>
#include <SDL2/SDL.h>
#include <emulator_manager.hpp>
#include <game.hpp>
#include <list>
namespace
{
    enum class AppMode
    {
        Menu,
        Running
    };

    struct Glyph
    {
        std::array<uint8_t, 7> rows{};
    };

    Glyph glyph_for(char input)
    {
        const char c = static_cast<char>(std::toupper(static_cast<unsigned char>(input)));

        switch (c)
        {
        case 'A': return {{{0b01110, 0b10001, 0b10001, 0b11111, 0b10001, 0b10001, 0b10001}}};
        case 'B': return {{{0b11110, 0b10001, 0b10001, 0b11110, 0b10001, 0b10001, 0b11110}}};
        case 'C': return {{{0b01111, 0b10000, 0b10000, 0b10000, 0b10000, 0b10000, 0b01111}}};
        case 'D': return {{{0b11110, 0b10001, 0b10001, 0b10001, 0b10001, 0b10001, 0b11110}}};
        case 'E': return {{{0b11111, 0b10000, 0b10000, 0b11110, 0b10000, 0b10000, 0b11111}}};
        case 'F': return {{{0b11111, 0b10000, 0b10000, 0b11110, 0b10000, 0b10000, 0b10000}}};
        case 'G': return {{{0b01111, 0b10000, 0b10000, 0b10011, 0b10001, 0b10001, 0b01111}}};
        case 'H': return {{{0b10001, 0b10001, 0b10001, 0b11111, 0b10001, 0b10001, 0b10001}}};
        case 'I': return {{{0b11111, 0b00100, 0b00100, 0b00100, 0b00100, 0b00100, 0b11111}}};
        case 'J': return {{{0b00111, 0b00010, 0b00010, 0b00010, 0b10010, 0b10010, 0b01100}}};
        case 'K': return {{{0b10001, 0b10010, 0b10100, 0b11000, 0b10100, 0b10010, 0b10001}}};
        case 'L': return {{{0b10000, 0b10000, 0b10000, 0b10000, 0b10000, 0b10000, 0b11111}}};
        case 'M': return {{{0b10001, 0b11011, 0b10101, 0b10101, 0b10001, 0b10001, 0b10001}}};
        case 'N': return {{{0b10001, 0b11001, 0b10101, 0b10011, 0b10001, 0b10001, 0b10001}}};
        case 'O': return {{{0b01110, 0b10001, 0b10001, 0b10001, 0b10001, 0b10001, 0b01110}}};
        case 'P': return {{{0b11110, 0b10001, 0b10001, 0b11110, 0b10000, 0b10000, 0b10000}}};
        case 'Q': return {{{0b01110, 0b10001, 0b10001, 0b10001, 0b10101, 0b10010, 0b01101}}};
        case 'R': return {{{0b11110, 0b10001, 0b10001, 0b11110, 0b10100, 0b10010, 0b10001}}};
        case 'S': return {{{0b01111, 0b10000, 0b10000, 0b01110, 0b00001, 0b00001, 0b11110}}};
        case 'T': return {{{0b11111, 0b00100, 0b00100, 0b00100, 0b00100, 0b00100, 0b00100}}};
        case 'U': return {{{0b10001, 0b10001, 0b10001, 0b10001, 0b10001, 0b10001, 0b01110}}};
        case 'V': return {{{0b10001, 0b10001, 0b10001, 0b10001, 0b10001, 0b01010, 0b00100}}};
        case 'W': return {{{0b10001, 0b10001, 0b10001, 0b10101, 0b10101, 0b11011, 0b10001}}};
        case 'X': return {{{0b10001, 0b01010, 0b00100, 0b00100, 0b00100, 0b01010, 0b10001}}};
        case 'Y': return {{{0b10001, 0b01010, 0b00100, 0b00100, 0b00100, 0b00100, 0b00100}}};
        case 'Z': return {{{0b11111, 0b00010, 0b00100, 0b00100, 0b01000, 0b10000, 0b11111}}};
        case '0': return {{{0b01110, 0b10011, 0b10101, 0b10101, 0b10101, 0b11001, 0b01110}}};
        case '1': return {{{0b00100, 0b01100, 0b00100, 0b00100, 0b00100, 0b00100, 0b01110}}};
        case '2': return {{{0b01110, 0b10001, 0b00001, 0b00010, 0b00100, 0b01000, 0b11111}}};
        case '3': return {{{0b11110, 0b00001, 0b00001, 0b01110, 0b00001, 0b00001, 0b11110}}};
        case '4': return {{{0b00010, 0b00110, 0b01010, 0b10010, 0b11111, 0b00010, 0b00010}}};
        case '5': return {{{0b11111, 0b10000, 0b10000, 0b11110, 0b00001, 0b00001, 0b11110}}};
        case '6': return {{{0b01110, 0b10000, 0b10000, 0b11110, 0b10001, 0b10001, 0b01110}}};
        case '7': return {{{0b11111, 0b00001, 0b00010, 0b00100, 0b01000, 0b01000, 0b01000}}};
        case '8': return {{{0b01110, 0b10001, 0b10001, 0b01110, 0b10001, 0b10001, 0b01110}}};
        case '9': return {{{0b01110, 0b10001, 0b10001, 0b01111, 0b00001, 0b00001, 0b01110}}};
        case ':': return {{{0b00000, 0b00100, 0b00100, 0b00000, 0b00100, 0b00100, 0b00000}}};
        case '.': return {{{0b00000, 0b00000, 0b00000, 0b00000, 0b00000, 0b00100, 0b00100}}};
        case '-': return {{{0b00000, 0b00000, 0b00000, 0b11111, 0b00000, 0b00000, 0b00000}}};
        case '!': return {{{0b00100, 0b00100, 0b00100, 0b00100, 0b00100, 0b00000, 0b00100}}};
        case ' ': return {{{0, 0, 0, 0, 0, 0, 0}}};
        default: return {{{0b11111, 0b10001, 0b00010, 0b00100, 0b01000, 0b00000, 0b01000}}};
        }
    }

    void draw_text(SDL_Renderer* renderer, int x, int y, const std::string& text, SDL_Color color, int scale = 3, int spacing = 1)
    {
        SDL_SetRenderDrawColor(renderer, color.r, color.g, color.b, color.a);

        int cursor_x = x;
        for (char ch : text)
        {
            const Glyph glyph = glyph_for(ch);

            for (int row = 0; row < 7; ++row)
            {
                for (int col = 0; col < 5; ++col)
                {
                    if (glyph.rows[row] & (1u << (4 - col)))
                    {
                        SDL_Rect pixel_rect{
                            cursor_x + col * scale,
                            y + row * scale,
                            scale,
                            scale
                        };
                        SDL_RenderFillRect(renderer, &pixel_rect);
                    }
                }
            }

            cursor_x += 5 * scale + spacing;
        }
    }

    uint16_t map_chip8_key(SDL_Keycode keycode)
    {
        switch (keycode)
        {
        case SDLK_1:
            return 0x1;
        case SDLK_2:
            return 0x2;
        case SDLK_3:
            return 0x3;
        case SDLK_4:
            return 0xC;
        case SDLK_q:
            return 0x4;
        case SDLK_w:
            return 0x5;
        case SDLK_e:
            return 0x6;
        case SDLK_r:
            return 0xD;
        case SDLK_a:
            return 0x7;
        case SDLK_s:
            return 0x8;
        case SDLK_d:
            return 0x9;
        case SDLK_f:
            return 0xE;
        case SDLK_z:
            return 0xA;
        case SDLK_x:
            return 0x0;
        case SDLK_c:
            return 0xB;
        case SDLK_v:
            return 0xF;
        default:
            return 0xFF;
        }
    }

    uint16_t map_ps1_key(SDL_Keycode keycode)
    {
        switch (keycode)
        {
        case SDLK_x:
            return 0;
        case SDLK_z:
            return 8;
        case SDLK_RETURN:
            return 3;
        case SDLK_BACKSPACE:
            return 2;
        case SDLK_UP:
            return 4;
        case SDLK_DOWN:
            return 5;
        case SDLK_LEFT:
            return 6;
        case SDLK_RIGHT:
            return 7;
        case SDLK_q:
            return 10;
        case SDLK_w:
            return 11;
        case SDLK_a:
            return 12;
        case SDLK_s:
            return 13;
        default:
            return 0xFF;
        }
    }
}

class Main
{
public:
    static void run()
    {
        EmulatorManager manager;
        std::vector<Game> games;
        for (const Game& game : manager.list_games())
        {
            games.push_back(game);
        }

        if (games.empty())
        {
            std::cerr << "[main] no games available" << std::endl;
            return;
        }

        if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_TIMER) != 0)
        {
            std::cerr << "[main] SDL_Init failed: " << SDL_GetError() << std::endl;
            return;
        }

        constexpr int windowWidth = 1200;
        constexpr int windowHeight = 720;

        SDL_Window *window = SDL_CreateWindow(
            "UAI Station - Menu",
            SDL_WINDOWPOS_CENTERED,
            SDL_WINDOWPOS_CENTERED,
            windowWidth,
            windowHeight,
            SDL_WINDOW_SHOWN);

        if (!window)
        {
            std::cerr << "[main] SDL_CreateWindow failed: " << SDL_GetError() << std::endl;
            SDL_Quit();
            return;
        }

        SDL_Renderer *renderer = SDL_CreateRenderer(
            window,
            -1,
            SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);

        if (!renderer)
        {
            std::cerr << "[main] SDL_CreateRenderer failed: " << SDL_GetError() << std::endl;
            SDL_DestroyWindow(window);
            SDL_Quit();
            return;
        }

        constexpr double dt = 1.0 / 60.0;
        bool running = true;
        AppMode mode = AppMode::Menu;
        size_t selected_game = 0;
        EmulatorState state{EmulatorStatus::IDLE, nullptr, 0, 0, {}};
        SDL_Texture* texture = nullptr;

        auto destroy_texture = [&]() {
            if (texture)
            {
                SDL_DestroyTexture(texture);
                texture = nullptr;
            }
        };

        auto ensure_texture = [&](size_t width, size_t height) -> bool {
            if (width == 0 || height == 0)
            {
                return false;
            }

            if (texture)
            {
                Uint32 texture_format = 0;
                int texture_access = 0;
                int texture_width = 0;
                int texture_height = 0;

                if (SDL_QueryTexture(texture, &texture_format, &texture_access, &texture_width, &texture_height) == 0 &&
                    texture_width == static_cast<int>(width) &&
                    texture_height == static_cast<int>(height))
                {
                    return true;
                }

                destroy_texture();
            }

            texture = SDL_CreateTexture(
                renderer,
                SDL_PIXELFORMAT_ARGB8888,
                SDL_TEXTUREACCESS_STREAMING,
                static_cast<int>(width),
                static_cast<int>(height));

            if (!texture)
            {
                std::cerr << "[main] SDL_CreateTexture failed: " << SDL_GetError() << std::endl;
                return false;
            }

            return true;
        };

        auto start_game = [&](size_t index) {
            destroy_texture();

            manager.play_game(games[index]);
            state = manager.get_state();

            if (state.status == EmulatorStatus::IDLE || !state.current_frame.data())
            {
                mode = AppMode::Menu;
                SDL_SetWindowTitle(window, "UAI Station - Menu");
                return;
            }

            const size_t width = state.width;
            const size_t height = state.height;
            if (!ensure_texture(width, height))
            {
                manager.stop_game();
                state = manager.get_state();
                mode = AppMode::Menu;
                SDL_SetWindowTitle(window, "UAI Station - Menu");
                return;
            }

            mode = AppMode::Running;
            SDL_SetWindowTitle(window, state.current_game ? state.current_game->name.c_str() : "UAI Station");
        };

        auto draw_menu = [&]() {
            SDL_SetRenderDrawColor(renderer, 12, 14, 20, 255);
            SDL_RenderClear(renderer);

            SDL_Rect backdrop{64, 48, windowWidth - 128, windowHeight - 96};
            SDL_SetRenderDrawColor(renderer, 24, 28, 40, 255);
            SDL_RenderFillRect(renderer, &backdrop);

            SDL_SetRenderDrawColor(renderer, 64, 76, 112, 255);
            SDL_RenderDrawRect(renderer, &backdrop);

            draw_text(renderer, 92, 82, "UAI STATION", SDL_Color{240, 244, 255, 255}, 5, 2);
            draw_text(renderer, 92, 132, "SELECT A GAME", SDL_Color{168, 180, 210, 255}, 3, 1);
            draw_text(renderer, 92, 170, "UP/DOWN TO MOVE   ENTER TO PLAY   ESC TO QUIT", SDL_Color{120, 132, 156, 255}, 2, 1);

            const int item_x = 92;
            const int item_y = 228;
            const int item_width = windowWidth - 184;
            const int item_height = 84;
            const int item_gap = 18;

            for (size_t index = 0; index < games.size(); ++index)
            {
                const int y = item_y + static_cast<int>(index) * (item_height + item_gap);
                SDL_Rect item{item_x, y, item_width, item_height};

                if (index == selected_game)
                {
                    SDL_SetRenderDrawColor(renderer, 46, 74, 120, 255);
                }
                else
                {
                    SDL_SetRenderDrawColor(renderer, 34, 38, 52, 255);
                }

                SDL_RenderFillRect(renderer, &item);

                SDL_SetRenderDrawColor(renderer, 94, 106, 132, 255);
                SDL_RenderDrawRect(renderer, &item);

                draw_text(renderer, item_x + 20, y + 18, games[index].name, SDL_Color{248, 250, 255, 255}, 3, 1);
                draw_text(renderer, item_x + 20, y + 48, games[index].description, SDL_Color{160, 170, 192, 255}, 2, 1);
            }

            SDL_RenderPresent(renderer);
        };

        auto draw_game = [&]() {
            if (!texture || !state.current_frame.data())
            {
                return;
            }

            SDL_UpdateTexture(
                texture,
                nullptr,
                state.current_frame.data(),
                static_cast<int>(state.width * sizeof(uint32_t)));

            int win_width = 0;
            int win_height = 0;
            SDL_GetWindowSize(window, &win_width, &win_height);

            SDL_Rect dst_rect{};
            if (state.width > 0 && state.height > 0 && win_width > 0 && win_height > 0)
            {
                const double scale_x = static_cast<double>(win_width) / static_cast<double>(state.width);
                const double scale_y = static_cast<double>(win_height) / static_cast<double>(state.height);
                const double scale_factor = std::min(scale_x, scale_y);
                const int render_width = static_cast<int>(state.width * scale_factor);
                const int render_height = static_cast<int>(state.height * scale_factor);
                dst_rect.x = (win_width - render_width) / 2;
                dst_rect.y = (win_height - render_height) / 2;
                dst_rect.w = render_width;
                dst_rect.h = render_height;
            }
            else
            {
                dst_rect = SDL_Rect{0, 0, win_width, win_height};
            }

            SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
            SDL_RenderClear(renderer);
            SDL_RenderCopy(renderer, texture, nullptr, &dst_rect);
            SDL_RenderPresent(renderer);
        };

        while (running)
        {
            SDL_Event event;

            while (SDL_PollEvent(&event))
            {
                if (event.type == SDL_QUIT)
                {
                    running = false;
                }

                if (event.type == SDL_KEYDOWN || event.type == SDL_KEYUP)
                {
                    const bool pressed = (event.type == SDL_KEYDOWN);
                    if (mode == AppMode::Menu && pressed)
                    {
                        switch (event.key.keysym.sym)
                        {
                        case SDLK_ESCAPE:
                            running = false;
                            break;
                        case SDLK_UP:
                            selected_game = (selected_game == 0) ? games.size() - 1 : selected_game - 1;
                            break;
                        case SDLK_DOWN:
                            selected_game = (selected_game + 1) % games.size();
                            break;
                        case SDLK_RETURN:
                        case SDLK_KP_ENTER:
                            start_game(selected_game);
                            break;
                        default:
                            break;
                        }
                    }
                    else if (mode == AppMode::Running)
                    {
                        if (pressed && event.key.keysym.sym == SDLK_ESCAPE)
                        {
                            manager.stop_game();
                            state = manager.get_state();
                            destroy_texture();
                            mode = AppMode::Menu;
                            SDL_SetWindowTitle(window, "UAI Station - Menu");
                        }
                        else
                        {
                            const Game* current_game = state.current_game;
                            if (current_game)
                            {
                                const uint16_t key = (current_game->id == "ps1")
                                                         ? map_ps1_key(event.key.keysym.sym)
                                                         : map_chip8_key(event.key.keysym.sym);

                                if (key != 0xFF)
                                {
                                    manager.updateKeyboard(key, pressed);
                                }
                            }
                        }
                    }
                }
            }

            if (mode == AppMode::Running)
            {
                manager.update(dt);
                state = manager.get_state();

                if (!state.current_frame.data())
                {
                    draw_menu();
                    continue;
                }

                if (!ensure_texture(state.width, state.height))
                {
                    manager.stop_game();
                    state = manager.get_state();
                    mode = AppMode::Menu;
                    SDL_SetWindowTitle(window, "UAI Station - Menu");
                    draw_menu();
                    continue;
                }

                draw_game();
            }
            else
            {
                draw_menu();
            }
        }

        destroy_texture();
        SDL_DestroyRenderer(renderer);
        SDL_DestroyWindow(window);
        SDL_Quit();
    }
};

int main()
{
    Main::run();
    return 0;
}