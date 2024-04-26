#ifndef __GOLF_COURSE_H__
#define __GOLF_COURSE_H__
#include <math.h>
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include "forces.h"
#include "scene.h"
#include "collision.h"
#include "vector.h"

typedef struct golf_course golf_course_t;

golf_course_t *golf_course_init(body_t *course, body_t* hole, vector_t start_pos_ball_1, vector_t start_pos_ball_2, rgb_color_t wall_color);

void golf_course_free(golf_course_t *golf_course);

void golf_course_bodies_free(list_t *bodies);

void golf_course_add_walls(golf_course_t *golf_course);

void golf_course_add_extra(golf_course_t *golf_course, body_t *extra);

body_t *golf_course_get_course(golf_course_t *golf_course);

list_t *golf_course_get_walls(golf_course_t *golf_course);

list_t *golf_course_get_extras(golf_course_t *golf_course);

body_t *golf_course_get_hole(golf_course_t *golf_course);

vector_t golf_course_get_ball1_pos(golf_course_t *golf_course);

vector_t golf_course_get_ball2_pos(golf_course_t *golf_course);

#endif // #ifndef __GOLF_COURSE_H__