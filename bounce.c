#include <math.h>
#include <stdlib.h>
#include "sdl_wrapper.h"
#include "polygon.h"
#include "list.h"
#include "star.h"

const vector_t MAX_CANVAS_SIZE = {.x=1000, .y=500};
//sets the ratio of the distance to the outside point (from center) to the
//distance to the inside point (from center)
const double OUTER_INNER_RATIO = 2.5;
const int POINTS = 5;
const double SIZE = 50;
const double SPEED = 400;
const double ROTATE = .01;
//the color purple
const rgb_color_t RGB = {.r=.87, .g=0.69, .b=0.95};
const double ANGLE = M_PI/4;

list_t *make_star(size_t points, double x, double y, double size) {
    list_t *star = list_init(2*points, (free_func_t) star_free_vec_list);

    //calculates the coordinates of the tip and divot of the star for
    //each point the star has
    for (size_t i = 0; i < points; i++) {
        vector_t *outer_point = malloc(sizeof(vector_t));
        outer_point->x = x + (size * sin(2 * M_PI * i / points));
        outer_point->y = y - (size * cos(2 * M_PI * i / points));
        list_add(star, outer_point);

        vector_t *inner_point = malloc(sizeof(vector_t));
        inner_point->x = x + ((size / OUTER_INNER_RATIO) * sin((2 * M_PI * i / points) + (2 * M_PI / (2 *points))));
        inner_point->y = y - ((size / OUTER_INNER_RATIO) * cos((2 * M_PI * i / points) + (2 * M_PI / (2 *points))));
        list_add(star, inner_point);
    }
    return star;
}

void move (list_t *star, vector_t movement, vector_t center, double dt) {
    polygon_translate(star, vec_multiply(dt, movement));
    polygon_rotate(star, ROTATE, center);
}

vector_t is_collision(list_t *star, vector_t movement, double dt) {
    bool change_x = 0;
    bool change_y = 0;
    //for each outter point checks to see if it crosses the boundary
    //if it does it appropriately changes the direction of movement
    for(size_t i = 0; i < list_size(star); i+=2){
        vector_t pos = vec_add(*(vector_t*)list_get(star, i), vec_multiply(dt, movement));
        if((pos.x < 0 || pos.x > MAX_CANVAS_SIZE.x) && !change_x){
            movement.x *= -1;
            change_x = 1;
        }
        if((pos.y < 0 || pos.y > MAX_CANVAS_SIZE.y) && !change_y){
            movement.y *= -1;
            change_y = 1;
        }
    }
    return movement;
}

int main(int argc, char *argv[]) {
    //initializes the board
    vector_t vec_max = {.x=MAX_CANVAS_SIZE.x, .y=MAX_CANVAS_SIZE.y};
    sdl_init(VEC_ZERO, vec_max);

    vector_t speed = {.x=SPEED * cos(ANGLE), .y=SPEED * sin(ANGLE)};
    list_t *star = make_star(POINTS, MAX_CANVAS_SIZE.x / 2, MAX_CANVAS_SIZE.y / 2, SIZE);
    vector_t center = polygon_centroid(star);

    //creates the bouncing star visual and runs till window is closed
    while (!sdl_is_done(star)) {
        double dt = time_since_last_tick();
        speed = is_collision(star, speed, dt);
        move(star, speed, center, dt);
        center = polygon_centroid(star);

        sdl_clear();
        sdl_draw_polygon(star, RGB, NULL, false, NULL);
        sdl_show();
    }
}