/*
    interface.cpp

    This source wraps SDL2 and any other needed low-level APIs for window,
    input, graphics, and audio access. Global variables store state, so
    access from multiple threads must be externally synchronized.

*/

#include <iostream>
#include <sstream>
#include <algorithm>
#include <string>
#include <vector>
#include <map>
#include <memory>
#include <thread>
#include <mutex>
#include <atomic>
#include <optional>
#include <array>
#include <cmath>

#include <SDL.h>
#include <SDL_ttf.h>

#include "config.h"
#include "interface.h"
#include "audioproc.h"

SDL_Window *window = nullptr;
SDL_Surface *screen_surface = nullptr;
bool quit_requested = false;
bool needs_redraw = true;

int window_width = WINDOW_WIDTH;
int window_height = WINDOW_HEIGHT;

SDL_AudioDeviceID audio_device = 0;
std::string audio_device_name;
SDL_AudioSpec audio_spec = {0};
std::atomic<bool> is_capturing_audio(false);

TTF_Font * status_font = nullptr;

int frame_count = 0;

std::array<int8_t, WINDOW_WIDTH> display_waveform;

std::vector<std::string> preferred_audio_devices {
    "USB Advanced Audio Device",
    "MacBook Air Microphone"
};

std::optional<std::string> find_capture_device() {
    int num_devices = SDL_GetNumAudioDevices(SDL_TRUE);
    if (num_devices < 1) {
        return std::nullopt;
    }

    std::map<std::string, int> device_map;
    for (int i = 0; i < num_devices; i++) {
        std::string device_name = SDL_GetAudioDeviceName(i, SDL_TRUE);
        device_map[device_name] = i;
    }

    for (const auto & device_name : preferred_audio_devices) {
        auto it = device_map.find(device_name);
        if (it != device_map.end()) {
            return device_name;
        }
    }

    // If nothing, select first device.
    return device_map.begin()->first;
}

void audioCallback(void *userdata, Uint8 *stream, int len) {
    
    // Copy audio data into audioproc's sample queue.
    audio_samples_acquired((int16_t *)stream, len / sizeof(int16_t));

    // Copy audio data, downsampled, to display_waveform.
    int16_t * audio_data = (int16_t *)stream;
    int num_samples = len / sizeof(int16_t);

    for (int i = 0; i < WINDOW_WIDTH; i++) {
        int j = i * num_samples / WINDOW_WIDTH;
        j = std::min(j, num_samples - 1);
        float f = audio_data[j] / 32768.0f;

        display_waveform[i] = f * 20;
    }

    needs_redraw = true;
}

void interface_audio_init() {
    if (is_capturing_audio) {
        return;
    }

    SDL_AudioSpec desired;
    SDL_zero(desired);
    desired.freq = SAMPLE_RATE;
    desired.format = AUDIO_S16SYS;
    desired.channels = CHANNELS;
    desired.samples = BUFFER_SIZE;
    desired.callback = audioCallback;

    std::optional<std::string> device_name = find_capture_device();
    if (!device_name) {
        std::stringstream ss;
        ss << "No audio capture device found.";
        throw std::runtime_error(ss.str());
    }
    std::cout << "Using audio capture device: " << *device_name << std::endl;
    audio_device_name = *device_name;

    audio_device = SDL_OpenAudioDevice(
        device_name->c_str(),
        SDL_TRUE,
        &desired,
        &audio_spec,
        SDL_AUDIO_ALLOW_FREQUENCY_CHANGE | SDL_AUDIO_ALLOW_SAMPLES_CHANGE
    );


    if (audio_device == 0) {
        std::stringstream ss;
        ss << "Failed to open audio: " << SDL_GetError();
        throw std::runtime_error(ss.str());
    }

    std::cout << "Audio format: " << SDL_AUDIO_BITSIZE(audio_spec.format) << " bit" << std::endl;
    std::cout << "Audio frequency: " << audio_spec.freq << " Hz" << std::endl;
    std::cout << "Audio channels: " << (int)audio_spec.channels << std::endl;
    std::cout << "Audio samples: " << audio_spec.samples << std::endl;

    is_capturing_audio = true;
    SDL_PauseAudioDevice(audio_device, 0);
}

void interface_audio_teardown() {
    if (!is_capturing_audio) {
        return;
    }

    SDL_CloseAudioDevice(audio_device);
    is_capturing_audio = false;
    audio_device = 0;
}

void interface_setup() {

    // Initialize SDL2
    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO) != 0) {
        std::stringstream ss;
        ss << "Unable to initialize SDL: " << SDL_GetError();
        throw std::runtime_error(ss.str());
    }

    // Initialize SDL2_ttf
    if (TTF_Init() != 0) {
        std::stringstream ss;
        ss << "Unable to initialize SDL_ttf: " << TTF_GetError();
        throw std::runtime_error(ss.str());
    }

    status_font = TTF_OpenFont("Roboto-Regular.ttf", 10);
    if (!status_font) {
        std::stringstream ss;
        ss << "Unable to load Roboto-Regular.ttf: " << TTF_GetError();
        throw std::runtime_error(ss.str());
    }


    // Initialize audio.
    interface_audio_init();


    // Create window
    window = SDL_CreateWindow(
        "Lastest Stop",
        SDL_WINDOWPOS_CENTERED,
        SDL_WINDOWPOS_CENTERED,
        WINDOW_WIDTH,
        WINDOW_HEIGHT,
        SDL_WINDOW_SHOWN
    );

    if (!window) {
        std::stringstream ss;
        ss << "Could not create window: " << SDL_GetError();
        throw std::runtime_error(ss.str());
    }

    // Get window surface
    screen_surface = SDL_GetWindowSurface(window);
    if (!screen_surface) {
        std::stringstream ss;
        ss << "Could not get window surface: " << SDL_GetError();
        throw std::runtime_error(ss.str());
    }


    // We're done!
}

void interface_teardown() {
    interface_audio_teardown();

    TTF_CloseFont(status_font);
    SDL_DestroyWindow(window);

    TTF_Quit();
    SDL_Quit();
}

void interface_process_events() {
    SDL_Event event;

    while (SDL_PollEvent(&event)) {
        switch (event.type) {

            case SDL_QUIT:
                quit_requested = true;
                break;

            case SDL_KEYDOWN:
                // Bail if it's a key repeat
                if (event.key.repeat) {
                    break;
                }
                
                if (event.key.keysym.sym == SDLK_r) {
                    interface_audio_teardown();
                    interface_audio_init(); 
                    needs_redraw = true;
                }

                if (event.key.keysym.sym == SDLK_SPACE) {
                    audio_begin_capture();
                }
                break;

            case SDL_KEYUP:
                if (event.key.keysym.sym == SDLK_SPACE) {
                    audio_end_capture();
                }
                break;

            case SDL_AUDIODEVICEADDED:

                if (event.adevice.iscapture && !is_capturing_audio) {
                    interface_audio_init();
                    needs_redraw = true;
                }
                break;

            case SDL_AUDIODEVICEREMOVED:
                if (event.adevice.iscapture && is_capturing_audio) {
                    interface_audio_teardown();
                    needs_redraw = true;
                }
                break;

            default:
                break;
        }
    }
}

void interface_draw_string(const std::string & str, int x, int y, uint8_t r, uint8_t g, uint8_t b) {
    SDL_Color textColor = { r, g ,b, 255 };
    SDL_Surface * textSurf = TTF_RenderText_Blended( status_font, str.c_str(), textColor );
    if (!textSurf) {
        std::stringstream ss;
        ss << "Could not render text: " << TTF_GetError();
        throw std::runtime_error(ss.str());
    }
    
    SDL_Rect dst = {x, y, textSurf->w, textSurf->h};
    SDL_BlitSurface(textSurf, nullptr, screen_surface, &dst);

    SDL_FreeSurface(textSurf);
}

void interface_render_waveform() {
    // Lock pixels for surface and manually draw waveform.
    SDL_LockSurface(screen_surface);
    for (int x = 0; x < WINDOW_WIDTH; x++) {

        int y = 28 + display_waveform[x];
        y = std::clamp(y, 0, WINDOW_HEIGHT - 1);

        uint32_t * pixel = (uint32_t *)screen_surface->pixels + y * screen_surface->pitch / 4 + x;

        *pixel = SDL_MapRGB(
            screen_surface->format,
            48, 
            0, 
            32
        );

        while (y != 28) {
            if (y < 28) {
                y++;
                pixel += screen_surface->pitch / 4;
            } else {
                y--;
                pixel -= screen_surface->pitch / 4;
            }

            
            *pixel = SDL_MapRGB(
                screen_surface->format,
                144, 
                96, 
                120
            );
        }

    }
    SDL_UnlockSurface(screen_surface);
}

void interface_render() {
    if (!needs_redraw) {
        return;
    }

    frame_count++;
    
    std::stringstream ss;
    if (is_capturing_audio) {
        SDL_FillRect(screen_surface, nullptr, SDL_MapRGB(screen_surface->format, 235, 220, 226));
    
        ss << frame_count << " " << audio_device_name;
        
        interface_draw_string(ss.str(), 5, 5, 96, 32, 64);
        interface_render_waveform();
    } else {
        SDL_FillRect(screen_surface, nullptr, SDL_MapRGB(screen_surface->format, 195, 200, 205));
    
        ss << frame_count << " " << "Disconnected.";

        interface_draw_string(ss.str(), 5, 5, 60, 72, 90);
    }

    SDL_UpdateWindowSurface(window);
    needs_redraw = false;
}

bool interface_quit_requested() {
    return quit_requested;
}