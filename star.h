#ifndef __STAR_H__
#define __STAR_H__

#include <math.h>
#include <stdlib.h>
#include "polygon.h"
#include "list.h"
#include "color.h"

/**
 * A growable array of vectors, stored as pointers to malloc()ed vectors.
 * A list owns all the vectors in it, so it is responsible for free()ing them.
 * This line does two things:
 * - Declares a "struct star" type
 * - Makes "star_t" an alias for "struct star"
 */
typedef struct star star_t;

/**
 * Allocates memory for a new star with space for the given number of points and start point.
 *
 * @param num_points the integer number of points the star has
 * @param start_point the vector_t storing the point of the center of the star
 * @return a pointer to the newly allocated star
 */
star_t *star_init(size_t num_points, vector_t start_point);

/**
 * Computes a random RGB color.
 *
 * @return an rgb struct storing the random RGB values of the star
 */
rgb_color_t star_color();

/**
 * Releases the memory allocated for a vec list.
 * Also frees all vectors in the list.
 *
 * @param stars a pointer to a star list
 */
void star_free_vec_list(list_t *stars);

/**
 * Releases the memory allocated for a star.
 * Also frees all points in the star.
 *
 * @param star a pointer to a star returned from star_init()
 */
void star_free(star_t *star);

/**
 * Releases the memory allocated for a star list.
 * Also frees all stars in the list.
 * Also frees all points in all the stars.
 *
 * @param stars a pointer to a star list
 */
void star_free_star_list(list_t *stars);

/*
 * Makes a star consisting of a list of vectors that represent the coordinates of the points of the star.
 * 
 * @param points the number of points the star has
 * @param x the x-value of the center of the star
 * @param y the y-value of the center of the star
 * @param size the distance between the center of the star and the tip of the star
 * @return a vec_list_t that stores the coordinates of the points of the star
 */
list_t *star_create(size_t points, double x, double y, double size);

/**
 * Gets the velocity from a certain star
 *
 * @param star a pointer to a star returned from star_init()
 * @return the velocity vector of the star
 */
vector_t star_get_velocity(star_t *star);

/**
 * Set the star's velocity to a given new velocity.
 *
 * @param star a pointer to a star returned from star_init()
 * @param new_velocity a vector to set the velocity to
 */
void star_set_velocity(star_t *star, vector_t new_velocity);

/**
 * Gets the vec_list of the star's points from a certain star
 *
 * @param star a pointer to a star returned from star_init()
 * @return the vec_list of the points of the star
 */
list_t *star_get_points(star_t *star);

/**
 * Set the star's points to a given new set of points.
 *
 * @param star a pointer to a star returned from star_init()
 * @param new_points a set of points to set the points to
 */
void star_set_points(star_t *star, list_t* new_points);

/**
 * Gets the color of a certain star
 *
 * @param star a pointer to a star returned from star_init()
 * @return the color of the star
 */
rgb_color_t star_get_color(star_t *star);

#endif