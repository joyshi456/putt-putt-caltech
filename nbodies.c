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
const int NUM_POINTS = 4;
const int NUM_STARS = 50;
const double OUTER_INNER_RATIO_STAR = 2.5;
const int COLOR_MAX = 255;
const int SIZE_RANGE = 30;
const int MIN_SIZE = 20;
const double G = 500;
const int MASS_SCALE = 10;

rgb_color_t random_color(){
    double red = (double) (rand() % COLOR_MAX) / COLOR_MAX;
    double green = (double) (rand() % COLOR_MAX) / COLOR_MAX;
    double blue = (double) (rand() % COLOR_MAX) / COLOR_MAX;
    rgb_color_t color = {.r = red, .g = green, .b = blue};
    return color;
}

void add_star(scene_t *scene, size_t points) {
    double x = (double) (rand() % ((int)MAX_CANVAS_SIZE.x));
    double y = (double) (rand() % ((int)MAX_CANVAS_SIZE.y));
    double size = (double) (rand() % SIZE_RANGE) + MIN_SIZE;
    list_t *star = list_init(2 * points, (free_func_t) body_free_vec_list);
    //calculates the coordinates of the tip and divot of the star for
    //each point the star has
    for (size_t i = 0; i < points; i++) {
        vector_t *outer_point = malloc(sizeof(vector_t));
        assert(outer_point != NULL);
        outer_point->x = x + (size * sin(2 * M_PI * i / points));
        outer_point->y = y - (size * cos(2 * M_PI * i / points));
        list_add(star, outer_point);

        vector_t *inner_point = malloc(sizeof(vector_t));
        assert(inner_point != NULL);
        inner_point->x = x + ((size / OUTER_INNER_RATIO_STAR) * sin((2 * M_PI * i / points) + (2 * M_PI / (2 *points))));
        inner_point->y = y - ((size / OUTER_INNER_RATIO_STAR) * cos((2 * M_PI * i / points) + (2 * M_PI / (2 *points))));
        list_add(star, inner_point);
    }
    //mass is proportional to the size of the star
    double mass = size * MASS_SCALE;
    body_t *star_body = body_init(star, mass, random_color());
    scene_add_body(scene, star_body);
}

int main(int argc, char *argv[]) {
    sdl_init(VEC_ZERO, MAX_CANVAS_SIZE);
    scene_t *scene = scene_init();
    
    //initializes the stars
    for(size_t i = 0; i < NUM_STARS; i++){
        add_star(scene, NUM_POINTS);
    }

    //adds a gravity force between all of the stars
    for(size_t i = 0; i < scene_bodies(scene); i++){
        for(size_t j = i + 1; j < scene_bodies(scene); j++) {
            create_newtonian_gravity(scene, G, scene_get_body(scene, i), scene_get_body(scene, j));
        }
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