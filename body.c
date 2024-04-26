#include <stdlib.h>
#include <stdio.h>
#include <assert.h>
#include "body.h"
#include "color.h"
#include "list.h"
#include "vector.h"
#include "polygon.h"
#include "sdl_wrapper.h"

typedef struct body{
    list_t* shape;
    rgb_color_t color;
    rgb_color_t color2;
    double mass;
    vector_t velocity;
    vector_t force;
    vector_t impulse;
    vector_t centroid;
    int orientation;
    void* info;
    free_func_t info_freer;
    bool removed;
    bool has_texture;
    SDL_Surface *texture;
    bool hide;
    bool second_color;
} body_t;

body_t *body_init(list_t *shape, double mass, rgb_color_t color){
    body_t* body = malloc(sizeof(body_t));
    assert(body);
    body->shape = shape;
    body->color = color;
    body->mass = mass;
    body->velocity = VEC_ZERO;
    body->force = VEC_ZERO;
    body->impulse = VEC_ZERO;
    body->centroid = polygon_centroid(shape);
    body->orientation = 0;
    body->has_texture = false;
    body->removed = false;
    body->texture = NULL;
    body->hide = false;
    body->second_color = false;
    return body;
}

body_t *body_init_with_info(list_t *shape, double mass, rgb_color_t color, void *info, free_func_t info_freer){
    body_t* body = malloc(sizeof(body_t));
    assert(body);
    body->shape = shape;
    body->color = color;
    body->mass = mass;
    body->velocity = VEC_ZERO;
    body->force = VEC_ZERO;
    body->impulse = VEC_ZERO;
    body->centroid = polygon_centroid(shape);
    body->orientation = 0;
    body->removed = false;
    body->info = info;
    body->info_freer = info_freer;
    body->hide = false;
    return body;
}

void body_free_vec_list(list_t *list){
    size_t size_of_list = list_size(list);
    for(size_t i = 0; i < size_of_list; i++){
        free(list_remove(list, 0));
    }
    free(list);
}

void body_free(body_t *body){
    list_free(body->shape);
    free(body);
}

void body_hide(body_t *body){
    body->hide = true;
}

bool body_is_hidden(body_t *body){
    return body->hide;
}

list_t *body_get_shape(body_t *body){
    list_t *shape = body->shape;
    list_t* new_shape = list_init(list_size(shape), (free_func_t) body_free_vec_list);
    for(size_t i = 0; i<list_size(shape); i++){
        vector_t* vec = list_get(shape, i);
        vector_t* new_vec = malloc(sizeof(vector_t));
        new_vec->x = vec->x;
        new_vec->y = vec->y;
        list_add(new_shape, new_vec);
    }
    return new_shape;
}

vector_t body_get_centroid(body_t *body){
    return body->centroid;
}

vector_t body_get_velocity(body_t *body){
    return body->velocity;
}

vector_t body_get_force(body_t *body){
    return body->force;
}

vector_t body_get_impulse(body_t *body) {
    return body->impulse;
}

rgb_color_t body_get_color(body_t *body) {
    return body->color;
}

void body_set_color2(body_t *body, rgb_color_t color) {
    body->second_color = true;
    body->color2 = color;
}

void body_swap_color(body_t *body){
    if(body->second_color){
        rgb_color_t temp = body->color2;
        body->color2 = body->color;
        body->color = temp;
    }
}

void body_set_texture(body_t *body, SDL_Surface *texture){
    body->has_texture = true;
    body->texture = texture;
}

SDL_Surface *body_get_texture(body_t *body){
    return body->texture;
}

bool body_has_texture(body_t *body){
    return body->has_texture;
}

double body_get_mass(body_t *body) {
    return body->mass;
}

int body_get_orientation(body_t *body){
    return body->orientation;
}

void *body_get_info(body_t *body){
    return body->info;
}

void body_set_centroid(body_t *body, vector_t x){
    polygon_translate(body->shape, vec_subtract(x, body->centroid));
    body->centroid = x;
}

void body_set_velocity(body_t *body, vector_t v) {
    body->velocity = v;
}

void body_set_rotation(body_t *body, double angle) {
    polygon_rotate(body->shape, angle, body->centroid);
}

void body_set_orientation(body_t *body, int orientation) {
    body->orientation = orientation;
}

void body_tick(body_t *body, double dt) {
    vector_t new_vel = vec_add(body->velocity, vec_multiply((dt/body->mass), body->force));
    new_vel = vec_add(vec_multiply(1/body->mass, body->impulse), new_vel);
    body_set_centroid(body, vec_add(vec_multiply(dt/2, vec_add(body->velocity, new_vel)), body_get_centroid(body)));
    body->velocity = new_vel;
    body->force = VEC_ZERO;
    body->impulse = VEC_ZERO;
}

void body_add_force(body_t *body, vector_t force){
    body->force = vec_add(body->force, force);
}

void body_add_impulse(body_t *body, vector_t impulse){
     body->impulse = vec_add(body->impulse, impulse);
}

void body_remove(body_t *body){
    if(!body_is_removed(body)) body->removed = true;
}

bool body_is_removed(body_t *body){
    return(body->removed);
}

SDL_Rect *body_get_rect(body_t *body){
    list_t *shape = body->shape;
    double minx = INT16_MAX;
    double maxx = INT16_MIN;
    double miny = INT16_MAX;
    double maxy = INT16_MIN;
    for(size_t i = 0; i<list_size(shape); i++){
        vector_t *point = list_get(shape, i);
        if(point->x < minx) minx = point->x;
        if(point->x > maxx) maxx = point->x;
        if(point->y < miny) miny = point->y;
        if(point->y > maxy) maxy = point->y;
    }
    SDL_Rect *rect = malloc(sizeof(*rect));
    rect->x = minx;
    rect->y = miny;
    rect->w = maxx - minx;
    rect->h = maxy - miny;
    return rect;
}