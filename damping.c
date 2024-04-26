#include <math.h>
#include <stdlib.h>
#include <stdio.h>
#include <assert.h>
#include "sdl_wrapper.h"
#include "polygon.h"
#include "scene.h"
#include "forces.h"
#include "body.h"
#include "list.h"
#include "vector.h"

const vector_t MAX_CANVAS_SIZE = {.x=1000, .y=500};
const int COLOR_MAX = 255;
const int CIRCLE_POINTS = 20;
const int CIRCLE_SIZE = 10;
const double MASS = 10;
const double G = 0;
const double K = 100;
const double GAMMA = 5;
const double INITIAL_SPEED = 100;
const rgb_color_t WHITE = {.r = 1, .g = 1, .b = 1};

rgb_color_t random_color(){
    double red = (double) (rand() % COLOR_MAX) / COLOR_MAX;
    double green = (double) (rand() % COLOR_MAX) / COLOR_MAX;
    double blue = (double) (rand() % COLOR_MAX) / COLOR_MAX;
    rgb_color_t color = {.r = red, .g = green, .b = blue};
    return color;
}

list_t *draw_circle(vector_t center, size_t size){
    list_t *circle = list_init(CIRCLE_POINTS, (free_func_t) body_free_vec_list);
    for (size_t i = 0; i < CIRCLE_POINTS; i++) {
        vector_t *outer_point = malloc(sizeof(vector_t));
        assert(outer_point);
        outer_point->x = center.x + (size * sin(2 * M_PI * i / CIRCLE_POINTS));
        outer_point->y = center.y - (size * cos(2 * M_PI * i / CIRCLE_POINTS));
        list_add(circle, outer_point);
    }
    return circle;
}

//adds a circle
body_t *add_circle(scene_t *scene, vector_t center, double size, rgb_color_t color){
    list_t *circle = draw_circle(center, size);
    body_t *new_circle = body_init(circle, MASS, color);
    scene_add_body(scene, new_circle);
    return new_circle;
}

int main(int argc, char *argv[]) {
    sdl_init(VEC_ZERO, MAX_CANVAS_SIZE);
    scene_t *scene = scene_init();

    //initializes the anchors for the spring
    vector_t pos = {.x=CIRCLE_SIZE, .y=MAX_CANVAS_SIZE.y/2};
    int num_circles = MAX_CANVAS_SIZE.x/(CIRCLE_SIZE*2);
    for(size_t i = 0; i < num_circles; i++){
        add_circle(scene, pos, 1, WHITE);
        pos.x += (CIRCLE_SIZE*2);
    }

    //initializes the cicles that act as the spring
    pos.x = CIRCLE_SIZE;
    for(size_t i = 0; i < num_circles; i++){
        add_circle(scene, pos, CIRCLE_SIZE, random_color());
        pos.x += (CIRCLE_SIZE*2);
    }
    
    //creates the spring (between circle and anchor) and drag forces on the circles
    vector_t velocity = {.x = 0, .y = INITIAL_SPEED};
    for(size_t i = 0; i < num_circles; i++){
        body_t *body = scene_get_body(scene, i+num_circles);
        body_t *anchor = scene_get_body(scene, i);
        body_set_velocity(body, vec_multiply(sqrt(i)*sin((double)i/5), velocity));
        create_spring(scene, K, body, anchor);
        create_drag(scene, GAMMA, body);
    }
    
    //runs untill the window closes
    while (!sdl_is_done(scene)) {
        double dt = time_since_last_tick();
        scene_tick(scene, dt);
        sdl_render_scene(scene);
    }
    scene_free(scene);
    return 0;
}