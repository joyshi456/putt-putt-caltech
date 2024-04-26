#include <math.h>
#include <stdlib.h>
#include <assert.h>
#include "star.h"
#include "polygon.h"
#include "list.h"

const size_t STAR_SIZE = 50;
const int RGB_MAX = 255;
const double OUTER_INNER_RATIO_S = 2.5;

typedef struct star{
    vector_t velocity;
    size_t size;
    size_t num_points;
    rgb_color_t color;
    list_t* points;
} star_t;

const vector_t INIT_VEL = {
    .x = 600,
    .y = 0
};

star_t *star_init(size_t num_points, vector_t start_point){
    star_t* star = malloc(sizeof(star_t));
    assert(star != NULL);
    star->size = STAR_SIZE;
    star->color = star_color();
    star->points = star_create(num_points, start_point.x, start_point.y, STAR_SIZE);
    star->num_points = num_points;
    star->velocity = INIT_VEL;
    return star;
}

rgb_color_t star_color(){
    double red = (double) (rand() % RGB_MAX) / RGB_MAX;
    double green = (double) (rand() % RGB_MAX) / RGB_MAX;
    double blue = (double) (rand() % RGB_MAX) / RGB_MAX;
    rgb_color_t color = {.r = red, .g = green, .b = blue};
    return color;
}

void star_free_vec_list(list_t *list){
    for(size_t i = 0; i < list_size(list); i++){
        free(list_get(list, i));
    }
    free(list);
}

void star_free(star_t *star){
    star_free_vec_list(star->points);
    free(star);
}

void star_free_star_list(list_t *stars) {
    for(size_t i = 0; i < list_size(stars); i++){
        star_free(list_get(stars, i));
    }
    free(stars);
}

list_t *star_create(size_t points, double x, double y, double size) {
    list_t *star = list_init(2 * points, (free_func_t) star_free_vec_list);
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
        inner_point->x = x + ((size / OUTER_INNER_RATIO_S) * sin((2 * M_PI * i / points) + (2 * M_PI / (2 *points))));
        inner_point->y = y - ((size / OUTER_INNER_RATIO_S) * cos((2 * M_PI * i / points) + (2 * M_PI / (2 *points))));
        list_add(star, inner_point);
    }
    return star;
}

vector_t star_get_velocity(star_t *star){
    return star->velocity;
}

void star_set_velocity(star_t *star, vector_t new_velocity){
    star->velocity = new_velocity;
}

list_t *star_get_points(star_t *star){
    return star->points;
}

void star_set_points(star_t *star, list_t* new_points){
    star->points = new_points;
}

rgb_color_t star_get_color(star_t *star){
    return star->color;
}