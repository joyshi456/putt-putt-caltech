#include <assert.h>
#include <math.h>
#include <stdlib.h>
#include <time.h>
#include <SDL2/SDL.h>
#include <SDL2/SDL2_gfxPrimitives.h>
#include <SDL2/SDL_image.h>
#include "sdl_wrapper.h"

const char WINDOW_TITLE[] = "CS 3";
const int WINDOW_WIDTH = 1000;
const int WINDOW_HEIGHT = 500;
const double MS_PER_S = 1e3;

/**
 * The coordinate at the center of the screen.
 */
vector_t center;
/**
 * The coordinate difference from the center to the top right corner.
 */
vector_t max_diff;
/**
 * The SDL window where the scene is rendered.
 */
SDL_Window *window;
/**
 * The renderer used to draw the scene.
 */
SDL_Renderer *renderer;
list_t *texture_text;
list_t *rect_text;
list_t *texture_img;
list_t *rect_img;
/**
 * The keypress handler, or NULL if none has been configured.
 */
key_handler_t key_handler = NULL;

mouse_click_handler_t mouse_click_handler = NULL;
mouse_click_handler_t mouse_click_handler_no_release = NULL;
mouse_click_handler_t mouse_button_up_handler = NULL;
mouse_scroll_handler_t mouse_scroll_handler = NULL;
/**
 * SDL's timestamp when a key was last pressed or released.
 * Used to mesasure how long a key has been held.
 */
uint32_t key_start_timestamp;
/**
 * The value of clock() when time_since_last_tick() was last called.
 * Initially 0.
 */
clock_t last_clock = 0;

vector_t pos_click;

bool mouse_is_clicked = false;

bool prev_clicked = false;

/** Computes the center of the window in pixel coordinates */
vector_t get_window_center(void) {
    int *width = malloc(sizeof(*width)),
        *height = malloc(sizeof(*height));
    assert(width != NULL);
    assert(height != NULL);
    SDL_GetWindowSize(window, width, height);
    vector_t dimensions = {.x = *width, .y = *height};
    free(width);
    free(height);
    return vec_multiply(0.5, dimensions);
}

/**
 * Computes the scaling factor between scene coordinates and pixel coordinates.
 * The scene is scaled by the same factor in the x and y dimensions,
 * chosen to maximize the size of the scene while keeping it in the window.
 */
double get_scene_scale(vector_t window_center) {
    // Scale scene so it fits entirely in the window
    double x_scale = window_center.x / max_diff.x,
           y_scale = window_center.y / max_diff.y;
    return x_scale < y_scale ? x_scale : y_scale;
}

/** Maps a scene coordinate to a window coordinate */
vector_t get_window_position(vector_t scene_pos, vector_t window_center) {
    // Scale scene coordinates by the scaling factor
    // and map the center of the scene to the center of the window
    vector_t scene_center_offset = vec_subtract(scene_pos, center);
    double scale = get_scene_scale(window_center);
    vector_t pixel_center_offset = vec_multiply(scale, scene_center_offset);
    vector_t pixel = {
        .x = round(window_center.x + pixel_center_offset.x),
        // Flip y axis since positive y is down on the screen
        .y = round(window_center.y - pixel_center_offset.y)
    };
    return pixel;
}

/**
 * Converts an SDL key code to a char.
 * 7-bit ASCII characters are just returned
 * and arrow keys are given special character codes.
 */
char get_keycode(SDL_Keycode key) {
    switch (key) {
        case SDLK_LEFT:  return LEFT_ARROW;
        case SDLK_UP:    return UP_ARROW;
        case SDLK_RIGHT: return RIGHT_ARROW;
        case SDLK_DOWN:  return DOWN_ARROW;
        case SDLK_SPACE: return SPACE;
        case SDLK_n: return N;
        default:
            // Only process 7-bit ASCII characters
            return key == (SDL_Keycode) (char) key ? key : '\0';
    }
}

void sdl_free_rect(list_t *list){
    for(size_t i = 0; i < list_size(list); i++){
        free(list_remove(list, i));
        i--;
    }
    free(list);
}

void sdl_free_texture(list_t *list){
    for(size_t i = 0; i < list_size(list); i++){
        SDL_DestroyTexture(list_remove(list, i));
        i--;
    }
    free(list);
}

void sdl_init(vector_t min, vector_t max) {
    // Check parameters
    assert(min.x < max.x);
    assert(min.y < max.y);

    center = vec_multiply(0.5, vec_add(min, max));
    max_diff = vec_subtract(max, center);
    SDL_Init(SDL_INIT_EVERYTHING);
    window = SDL_CreateWindow(
        WINDOW_TITLE,
        SDL_WINDOWPOS_CENTERED,
        SDL_WINDOWPOS_CENTERED,
        WINDOW_WIDTH,
        WINDOW_HEIGHT,
        SDL_WINDOW_RESIZABLE
    );
    renderer = SDL_CreateRenderer(window, -1, 0);

    rect_text = list_init(10, (free_func_t)sdl_free_rect);
    texture_text = list_init(10, (free_func_t)sdl_free_texture);

    rect_img = list_init(10, (free_func_t)sdl_free_rect);
    texture_img = list_init(10, (free_func_t)sdl_free_texture);
}

bool sdl_is_done(void *scene) {
    SDL_Event *event = malloc(sizeof(*event));
    assert(event != NULL);
    int *x = malloc(sizeof(int));
    int *y = malloc(sizeof(int));
    while (SDL_PollEvent(event)) {
        switch (event->type) {
            case SDL_QUIT:
                free(event);
                return true;
            case SDL_KEYDOWN:
            case SDL_KEYUP:
                // Skip the keypress if no handler is configured
                // or an unrecognized key was pressed
                if (key_handler == NULL) break;
                char key = get_keycode(event->key.keysym.sym);
                if (key == '\0') break;

                uint32_t timestamp = event->key.timestamp;
                if (!event->key.repeat) {
                    key_start_timestamp = timestamp;
                }
                key_event_type_t type =
                    event->type == SDL_KEYDOWN ? KEY_PRESSED : KEY_RELEASED;
                double held_time = (timestamp - key_start_timestamp) / MS_PER_S;
                key_handler(key, type, held_time, scene);
                break;
            case SDL_MOUSEBUTTONDOWN:
                if (mouse_click_handler == NULL) break;
                SDL_GetMouseState(x, y);
                mouse_is_clicked = true;
                if(!prev_clicked){
                    pos_click = (vector_t){.x = *x, .y = *y};
                    prev_clicked = true;
                }
                break;
            case SDL_MOUSEMOTION:
                if(mouse_is_clicked){
                    if (mouse_click_handler_no_release == NULL) break;
                    SDL_GetMouseState(x, y);
                    vector_t new_pos = {.x = *x, .y = *y};
                    mouse_click_handler_no_release(pos_click, new_pos, scene);
                }
                break;
            case SDL_MOUSEBUTTONUP:
                if (mouse_click_handler == NULL) break;
                SDL_GetMouseState(x, y);
                vector_t new_pos = {.x = *x, .y = *y};
                mouse_click_handler(pos_click, new_pos, scene);
                prev_clicked = false;
                mouse_is_clicked = false;
                if (mouse_button_up_handler == NULL) break;
                mouse_button_up_handler(pos_click, new_pos, scene);
                break;
            case SDL_MOUSEWHEEL:
                if (mouse_scroll_handler == NULL) break;
                mouse_scroll_handler(event->wheel.x, event->wheel.y, scene);
                break;

        }
    }
    free(x);
    free(y);
    free(event);
    return false;
}

void sdl_clear(void) {
    SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255);
    SDL_RenderClear(renderer);
}

void sdl_draw_polygon(list_t *points, rgb_color_t color, SDL_Surface *texture, bool has_texture, SDL_Rect *rect) {
    // Check parameters
    size_t n = list_size(points);
    assert(n >= 3);
    assert(0 <= color.r && color.r <= 1);
    assert(0 <= color.g && color.g <= 1);
    assert(0 <= color.b && color.b <= 1);

    vector_t window_center = get_window_center();

    // Convert each vertex to a point on screen
    int16_t *x_points = malloc(sizeof(*x_points) * n),
            *y_points = malloc(sizeof(*y_points) * n);
    assert(x_points != NULL);
    assert(y_points != NULL);
    for (size_t i = 0; i < n; i++) {
        vector_t *vertex = list_get(points, i);
        vector_t pixel = get_window_position(*vertex, window_center);
        x_points[i] = pixel.x;
        y_points[i] = pixel.y;
    }
    filledPolygonRGBA(
            renderer,
            x_points, y_points, n,
            color.r * 255, color.g * 255, color.b * 255, 255
        );
    // Draw polygon with the given color
    if(has_texture){
        SDL_Texture *img_texture = SDL_CreateTextureFromSurface(renderer, texture);
        list_add(rect_img, rect);
        list_add(texture_img, img_texture);
        SDL_RenderCopy(renderer, img_texture, NULL, rect);
        //texturedPolygon(renderer, x_points, y_points, n, texture, 0, 0);
    }
    free(x_points);
    free(y_points);
}

void sdl_draw_text(SDL_Surface *surface, double x, double y, double w, double h){
    SDL_Texture *texture = SDL_CreateTextureFromSurface(renderer, surface);
    SDL_FreeSurface(surface);
    SDL_Rect *rect = malloc(sizeof(*rect));
    rect->x = x;
    rect->y = y;
    rect->w = w;
    rect->h = h;
    list_add(rect_text, rect);
    list_add(texture_text, texture);
}

void sdl_show(void) {
    // Draw boundary lines
    vector_t window_center = get_window_center();
    vector_t max = vec_add(center, max_diff),
             min = vec_subtract(center, max_diff);
    vector_t max_pixel = get_window_position(max, window_center),
             min_pixel = get_window_position(min, window_center);
    SDL_Rect *boundary = malloc(sizeof(*boundary));
    boundary->x = min_pixel.x;
    boundary->y = max_pixel.y;
    boundary->w = max_pixel.x - min_pixel.x;
    boundary->h = min_pixel.y - max_pixel.y;
    SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
    SDL_RenderDrawRect(renderer, boundary);
    for(size_t i = 0; i < list_size(texture_text); i++){
        SDL_RenderCopy(renderer, list_get(texture_text, i), NULL, list_get(rect_text, i));
    }
    for(size_t i = 0; i < list_size(texture_img); i++){
        SDL_RenderCopy(renderer, list_get(texture_img, i), NULL, list_get(rect_img, i));
    }
    list_free(texture_text);
    list_free(rect_text);
    rect_text = list_init(10, (free_func_t)sdl_free_rect);
    texture_text = list_init(10, (free_func_t)sdl_free_texture);

    list_free(texture_img);
    list_free(rect_img);
    rect_img = list_init(10, (free_func_t)sdl_free_rect);
    texture_img = list_init(10, (free_func_t)sdl_free_texture);
    free(boundary);

    SDL_RenderPresent(renderer);
}

void sdl_render_scene(scene_t *scene) {
    sdl_clear();
    size_t body_count = scene_bodies(scene);
    for (size_t i = 0; i < body_count; i++) {
        body_t *body = scene_get_body(scene, i);
        list_t *shape = body_get_shape(body);
        if(!body_is_hidden(body)) sdl_draw_polygon(shape, body_get_color(body), body_get_texture(body), body_has_texture(body), body_get_rect(body));
        list_free(shape);
    }
    sdl_show();
}

void sdl_on_key(key_handler_t handler) {
    key_handler = handler;
}

void sdl_on_click(mouse_click_handler_t handler) {
    mouse_click_handler = handler;
}

void sdl_on_click_no_release(mouse_click_handler_t handler) {
    mouse_click_handler_no_release = handler;
}

void sdl_on_scroll(mouse_scroll_handler_t handler){
    mouse_scroll_handler = handler;
}

void sdl_mouse_button_up(mouse_click_handler_t handler){
    mouse_button_up_handler = handler;
}

double time_since_last_tick(void) {
    clock_t now = clock();
    double difference = last_clock
        ? (double) (now - last_clock) / CLOCKS_PER_SEC
        : 0.0; // return 0 the first time this is called
    last_clock = now;
    return difference;
}