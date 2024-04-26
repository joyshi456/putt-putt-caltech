#include <math.h>
#include <stdlib.h>
#include <stdio.h>
#include "sdl_wrapper.h"
#include "star.h"
#include "polygon.h"
#include "list.h"

const vector_t MAX_CANVAS_SIZE = {.x=1000, .y=500};
const double SIZE = 50;
const double STAR_TIME = 0.3;
const vector_t GRAVITY = {.x = 0, .y = -10000};
const double ROTATE = .01;
const int RANDOM_CONTROL[] = {40, 150, 200};
const int POINT_INCREASE = 5;
const int NEGATION = -1;

void move_stars(star_t *star, double dt) {
    polygon_translate(star_get_points(star), vec_multiply(dt, star_get_velocity(star)));
    polygon_rotate(star_get_points(star), ROTATE, polygon_centroid(star_get_points(star)));
}

int off_screen(star_t *star){
    //loops through all of the points of the star and returns whether 
    //all of them are off the screen
    for(size_t i = 0; i < list_size(star_get_points(star)); i++){
        if (((vector_t*)list_get(star_get_points(star), i))->x < MAX_CANVAS_SIZE.x){
            return 0;
        }
    }
    return 1;
}

int star_bounce(star_t *star, double dt) {
    int change_y = 0;
    double elasticity = ((double) (rand() % RANDOM_CONTROL[0]) + RANDOM_CONTROL[1]) / RANDOM_CONTROL[2];
    //checks to see if the star hits the ground and changes the velocity accordingly
    for(size_t i = 0; i < list_size(star_get_points(star)); i += 2){
        vector_t pos = vec_add(*(vector_t*)list_get(star_get_points(star), i), vec_multiply(dt, star_get_velocity(star)));
        if((pos.y < 0) && !change_y) {
            vector_t delta = {
                .x = 0,
                .y = NEGATION * (1 + elasticity) * star_get_velocity(star).y
            };
            star_set_velocity(star, vec_add(star_get_velocity(star), delta));
            change_y = 1;
        }
    }
    star_set_velocity(star, vec_add(star_get_velocity(star), (vec_multiply(dt, GRAVITY))));
    return change_y;
}

int main(int argc, char *argv[]) {
    sdl_init(VEC_ZERO, MAX_CANVAS_SIZE);
    list_t *stars = list_init(10, (free_func_t) star_free_star_list);
    vector_t start_center = {.x = SIZE, .y = MAX_CANVAS_SIZE.y - SIZE};

    int star_number_beg = 0;
    int star_number_end = 0;
    double dt_from_last_star = 0;

    //keeps having stars come in till the window is closed
    while (!sdl_is_done(stars)) {
        double dt = time_since_last_tick();
        dt_from_last_star += dt;

        //adds a new star to the screen once enough time has elapsed
        if (dt_from_last_star > STAR_TIME) {
            star_t *new_star = star_init(star_number_end + POINT_INCREASE, start_center);
            list_add(stars, new_star);
            star_number_end++;
            dt_from_last_star = 0;
        }

        sdl_clear();

        //iterates over the stars in the range and draws them on the screen
        for(int i = star_number_beg; i < star_number_end; i++) {
            star_t *cur_star = list_get(stars, i);
            //if a star is offscreen we ignore it and remove it from the range
            if (off_screen(cur_star)) {
                star_number_beg++;
                continue;
            }
            if (!star_bounce(cur_star, dt)) {
                move_stars(cur_star, dt);
            }
            sdl_draw_polygon(star_get_points(cur_star), star_get_color(cur_star), NULL, false, NULL);
        }
        sdl_show();
    }
    list_free(stars);
    return 0;
}