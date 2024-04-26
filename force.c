#include <stdlib.h>
#include <stdio.h>
#include <assert.h>
#include "force.h"

typedef struct force{
    force_creator_t force_creator;
    void *aux;
    free_func_t freer;
    bool removed;
    list_t *bodies;
}force_t;

force_t *force_init(force_creator_t fc, void *aux, free_func_t freer){
    force_t* force = malloc(sizeof(force_t));
    assert(force);
    force->force_creator = fc;
    force->aux = aux;
    force->freer = freer;
    force->removed = false;
    force->bodies = list_init(0, free);
    return force;
}

force_t *force_init_with_bodies(force_creator_t fc, void *aux, free_func_t freer, list_t *bodies){
    force_t* force = malloc(sizeof(force_t));
    assert(force);
    force->force_creator = fc;
    force->aux = aux;
    force->freer = freer;
    force->removed = false;
    force->bodies = bodies;
    return force;
}

void force_free(force_t *force){
    if(force->freer != NULL) force->freer(force->aux);
    free(force->bodies);
    free(force);
}

void force_create(force_t *force){
    force->force_creator(force->aux);
}

void force_remove(force_t *force){
    if(!force_is_removed(force)) force->removed = true;
}

bool force_is_removed(force_t *force){
    return(force->removed);
}

list_t *force_get_bodies(force_t *force){
    return force->bodies;
}