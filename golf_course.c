#include <stdlib.h>
#include <stdio.h>
#include <assert.h>
#include "body.h"
#include "color.h"
#include "list.h"
#include "vector.h"
#include "polygon.h"

const double WALL_THICKNESS = 5;

typedef struct golf_course{
    body_t* course;
    list_t* walls;
    rgb_color_t wall_color;
    list_t* extras;
    body_t* hole;
    vector_t start_pos_ball_1;
    vector_t start_pos_ball_2;
} golf_course_t;

void golf_course_bodies_free(list_t *bodies){
    for(size_t i = 0; i < list_size(bodies); i++){
        body_free(list_get(bodies, i));
    }
    free(bodies);
}

golf_course_t *golf_course_init(body_t *course, body_t* hole, vector_t start_pos_ball_1, vector_t start_pos_ball_2, rgb_color_t wall_color){
    golf_course_t* golf_course = malloc(sizeof(golf_course_t));
    assert(golf_course);
    golf_course->course = course;
    golf_course->walls = list_init(10, (free_func_t)golf_course_bodies_free);
    golf_course->extras = list_init(10, (free_func_t)golf_course_bodies_free);
    golf_course->hole = hole;
    golf_course->start_pos_ball_1 = start_pos_ball_1;
    golf_course->start_pos_ball_2 = start_pos_ball_2;
    golf_course->wall_color = wall_color;
    return golf_course;
}

void golf_course_free(golf_course_t *golf_course){
    free(golf_course);
}

body_t *golf_course_add_wall(vector_t point1, vector_t point2, char *type, rgb_color_t wall_color, double thickness){
    vector_t center = vec_multiply(0.5, vec_add(point1, point2));
    vector_t p1_sub = vec_subtract(point1, center);
    double EPS = 2;
    point1 = vec_add(point1, vec_multiply(EPS / sqrt(vec_dot(p1_sub, p1_sub)), p1_sub));
    vector_t p2_sub = vec_subtract(point2, center);
    point2 = vec_add(point2, vec_multiply(EPS / sqrt(vec_dot(p2_sub, p2_sub)), p2_sub));
    // + EPISLON
    list_t *wall_points = list_init(10, (free_func_t) body_free_vec_list);
    vector_t diff = vec_subtract(point1, point2);
    vector_t unit_diff = vec_multiply(1/sqrt(vec_dot(diff, diff)), diff);
    vector_t shift = vec_multiply(thickness/2, vec_rotate(unit_diff, M_PI/2));
    vector_t *p1 = malloc(sizeof(vector_t));
    p1->x = vec_add(point1, shift).x;
    p1->y = vec_add(point1, shift).y;
    list_add(wall_points, p1);
    vector_t *p2 = malloc(sizeof(vector_t));
    p2->x = vec_add(point1, vec_negate(shift)).x;
    p2->y = vec_add(point1, vec_negate(shift)).y;
    list_add(wall_points, p2);
    vector_t *p3 = malloc(sizeof(vector_t));
    p3->x = vec_add(point2, vec_negate(shift)).x;
    p3->y = vec_add(point2, vec_negate(shift)).y;
    list_add(wall_points, p3);
    vector_t *p4 = malloc(sizeof(vector_t));
    p4->x = vec_add(point2, shift).x;
    p4->y = vec_add(point2, shift).y;
    list_add(wall_points, p4);
    body_t *wall = body_init_with_info(wall_points, INFINITY, wall_color, type, free);
    return wall;
}

void golf_course_add_walls(golf_course_t *golf_course){
    list_t *course_points = body_get_shape(golf_course->course);
    for(size_t i = 0; i < list_size(course_points); i++){
        vector_t *point1 = list_get(course_points, i % list_size(course_points));
        vector_t *point2 = list_get(course_points, (i+1) % list_size(course_points));
        body_t *wall = golf_course_add_wall(*point1, *point2, "wall_normal", golf_course->wall_color, WALL_THICKNESS);
        list_add(golf_course->walls, wall);
    }
}

void golf_course_add_extra(golf_course_t *golf_course, body_t *extra){
    list_add(golf_course->extras, extra);
}

body_t *golf_course_get_course(golf_course_t *golf_course){
    return golf_course->course;
}

list_t *golf_course_get_walls(golf_course_t *golf_course){
    return golf_course->walls;
}

list_t *golf_course_get_extras(golf_course_t *golf_course){
    return golf_course->extras;
}

body_t *golf_course_get_hole(golf_course_t *golf_course){
    return golf_course->hole;
}

vector_t golf_course_get_ball1_pos(golf_course_t *golf_course){
    return golf_course->start_pos_ball_1;
}

vector_t golf_course_get_ball2_pos(golf_course_t *golf_course){
    return golf_course->start_pos_ball_2;
}