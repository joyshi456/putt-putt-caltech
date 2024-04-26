#include <math.h>
#include "vector.h"

const vector_t VEC_ZERO = {.x=0, .y=0};
extern const vector_t VEC_ZERO;
const int INVERSE_CONST = -1;

vector_t vec_add(vector_t v1, vector_t v2) {
    vector_t new_vec = {.x = v1.x + v2.x, .y = v1.y + v2.y};
    return new_vec;
}

vector_t vec_subtract(vector_t v1, vector_t v2) {
    vector_t new_vec = {.x = v1.x - v2.x, .y = v1.y - v2.y};
    return new_vec;    
}

vector_t vec_negate(vector_t v) {
    vector_t new_vec = {.x = v.x * INVERSE_CONST, .y = v.y * INVERSE_CONST};
    return new_vec;
}

vector_t vec_multiply(double scalar, vector_t v) {
    vector_t new_vec = {.x = v.x * scalar, .y = v.y * scalar};
    return new_vec;
}

double vec_dot(vector_t v1, vector_t v2) {
    return (v1.x * v2.x) + (v1.y * v2.y);
}

double vec_cross(vector_t v1, vector_t v2) {
    return (v1.x*v2.y) - (v1.y*v2.x);
}

vector_t vec_rotate(vector_t v, double angle) {
    double new_x = (v.x * cos(angle)) - (v.y * sin(angle));
    double new_y = (v.x * sin(angle)) + (v.y * cos(angle));
    vector_t new_vec = {.x = new_x, .y = new_y};
    return new_vec;
}

double vec_dist(vector_t v1, vector_t v2){
    return sqrt((v1.x - v2.x) * (v1.x - v2.x) + (v1.y - v2.y) * (v1.y - v2.y));
}
