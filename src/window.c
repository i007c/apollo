
#define LOG_NAME "window"

#include "window.h"

#include <SDL2/SDL.h>
#include <SDL2/SDL_vulkan.h>

#include "logger.h"
#include "vec.h"
#include "vulkan.h"

status_r window_init(void) {
    status_t status;

    unwrap(SDL_Init(SDL_INIT_VIDEO | SDL_INIT_TIMER | SDL_INIT_GAMECONTROLLER));

    // From 2.0.18: Enable native IME.
#ifdef SDL_HINT_IME_SHOW_UI
    SDL_SetHint(SDL_HINT_IME_SHOW_UI, "1");
#endif

    SDL_Window *window = SDL_CreateWindow(
        "Apollo", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, 1280, 720, SDL_WINDOW_VULKAN | SDL_WINDOW_RESIZABLE | SDL_WINDOW_ALLOW_HIGHDPI
    );
    if (window == NULL) {
        log_error("Error: SDL_CreateWindow(): %s", SDL_GetError());
        return ERR_SDL_WIN_CRATE;
    }

    Vec ext_list;
    if (!SDL_Vulkan_GetInstanceExtensions(window, &ext_list.total, NULL))
        return ERR_SDL_VK_EXT;

    ext_list.size = sizeof(const char *);
    vec_new(&ext_list);
    SDL_Vulkan_GetInstanceExtensions(window, &ext_list.count, (const char **)ext_list.items);
    unwrap(vulkan_init(&ext_list));

    return OK;
}
