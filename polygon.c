#include <math.h>
#include <stdlib.h> 
#include "list.h"
#include "vector.h"

double polygon_area(list_t *polygon){
    double size = 0;
    size_t polygon_size = list_size(polygon);
    for(size_t i = 0; i<polygon_size; i++){
        size += vec_cross((*(vector_t*)list_get(polygon, i % polygon_size)), (*(vector_t*)(list_get(polygon, (i+1) % polygon_size))));
    }
    return fabs(size)/2;
}

vector_t polygon_centroid(list_t *polygon){
    double centroid_x = 0;
    double centroid_y = 0;
    size_t polygon_size = list_size(polygon);
    for(size_t i = 0; i < polygon_size; i++){
        //% makes the values loop around
        centroid_x += ((((vector_t*)list_get(polygon, i % polygon_size))->x + ((vector_t*)list_get(polygon, (i+1) % polygon_size))->x) 
                        * vec_cross((*(vector_t*)list_get(polygon, i % polygon_size)), (*(vector_t*)list_get(polygon, (i+1) % polygon_size))));
        centroid_y += ((((vector_t*)list_get(polygon, i % polygon_size))->y + ((vector_t*)list_get(polygon, (i+1) % polygon_size))->y) 
                        * vec_cross((*(vector_t*)list_get(polygon, i % polygon_size)), (*(vector_t*)list_get(polygon, (i+1) % polygon_size))));
    }
    centroid_x /= (6 * polygon_area(polygon));
    centroid_y /= (6 * polygon_area(polygon));
    vector_t centroid = {.x = centroid_x, .y = centroid_y};
    return centroid;
}

void polygon_translate(list_t *polygon, vector_t translation){
    for(size_t i = 0; i<list_size(polygon); i++){
        vector_t *new_vec = malloc(sizeof(vector_t));
        *new_vec = vec_add(*(vector_t*)(list_get(polygon, i)), translation);
        free(list_get(polygon, i));
        list_set(polygon, i, (void *) new_vec);
    }
}

void polygon_rotate(list_t *polygon, double angle, vector_t point){
    polygon_translate(polygon, vec_negate(point));
    for(size_t i = 0; i<list_size(polygon); i++){
        vector_t *new_vec = malloc(sizeof(vector_t));
        *new_vec = vec_rotate(*(vector_t*)(list_get(polygon, i)), angle);
        free(list_get(polygon, i));
        list_set(polygon, i, new_vec);
    }
    polygon_translate(polygon, point);
}