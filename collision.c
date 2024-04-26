#include <stdlib.h>
#include <assert.h>
#include <math.h>
#include "body.h"
#include "color.h"
#include "list.h"
#include "vector.h"
#include "polygon.h"
#include "collision.h"

typedef struct{
    collision_info_t collision_info;
    double overlap;
}collision_helper_t;

double get_vector_proj(vector_t vec1, vector_t vec2){
    vector_t unit_vec2 = vec_multiply(1/sqrt(vec_dot(vec2, vec2)), vec2);
    return vec_dot(vec1, unit_vec2);
}

vector_t get_polygon_proj(list_t *shape, vector_t vec){
    double min = INT16_MAX;
    double max = INT16_MIN;
    for(size_t i = 0; i < list_size(shape); i++){
        double proj = get_vector_proj(*(vector_t *)list_get(shape, i), vec);
        if(proj < min) min = proj;
        if(proj > max) max = proj;
    }
    vector_t poly_proj = {.x = min, .y = max};
    return poly_proj;
}

collision_helper_t collision_helper(list_t *shape1, list_t *shape2) {
    double min_overlap = INT16_MAX;
    vector_t min_axis;

    for(size_t i = 0; i < list_size(shape1); i++) {
        vector_t *point1 = (vector_t*) list_get(shape1, i);
        vector_t *point2 = (vector_t*) list_get(shape1, (i + 1) % list_size(shape1));
        vector_t edge = vec_subtract(*point1, *point2);
        vector_t axis = vec_rotate(edge, M_PI / 2);
        vector_t proj1 = get_polygon_proj(shape1, axis);
        vector_t proj2 = get_polygon_proj(shape2, axis);
        axis = vec_multiply(1/sqrt(vec_dot(axis, axis)), axis);
        if ((proj1.x < proj2.x && proj1.y < proj2.x) || (proj2.x < proj1.x && proj2.y < proj1.x)) {
            collision_info_t not_collide = {.collided = false};
            collision_helper_t not_collide_helper = {.collision_info = not_collide, .overlap = 0};
            return not_collide_helper;
        }
        double overlap = fmin(proj1.y, proj2.y) - fmax(proj1.x, proj2.x);
        if(overlap < min_overlap){
            min_overlap = overlap;
            min_axis = axis;
        }
    }
    collision_info_t collide = {.collided = true, .axis = min_axis};
    collision_helper_t collide_helper = {.collision_info = collide, .overlap = min_overlap};
    return collide_helper;
}

collision_info_t find_collision(list_t *shape1, list_t *shape2){
    collision_helper_t shape1_collide = collision_helper(shape1, shape2);
    collision_helper_t shape2_collide = collision_helper(shape2, shape1); 
    if(!shape1_collide.collision_info.collided || !shape2_collide.collision_info.collided) return (collision_info_t){.collided = false};
    if(shape1_collide.overlap < shape2_collide.overlap){
        return shape1_collide.collision_info;
    }
    return shape2_collide.collision_info;
}