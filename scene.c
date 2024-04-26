#include <stdbool.h>
#include <stdlib.h>
#include <stdio.h>
#include <assert.h>
#include "forces.h"
#include "body.h"
#include "list.h"
#include "scene.h"
#include "force.h"

const int DEFAULT_NUM_BODIES = 20;

typedef struct scene{
    list_t* bodies;
    list_t* forces;
    void* state;
} scene_t;

scene_t *scene_init(void){
    scene_t *scene = malloc(sizeof(scene_t));
    assert(scene);
    scene->bodies = list_init(DEFAULT_NUM_BODIES, (free_func_t) scene_bodies_free);
    scene->forces = list_init(DEFAULT_NUM_BODIES, (free_func_t) scene_forces_free);
    scene->state = NULL;
    return scene;
}

void scene_forces_free(scene_t *scene){
    for(size_t i = 0; i < list_size(scene->forces); i++){
        force_free(list_get(scene->forces, i));
    }
    free(scene->forces);
}

void scene_bodies_free(scene_t *scene){
    for(size_t i = 0; i < list_size(scene->bodies); i++){
        body_free(list_get(scene->bodies, i));
    }
    free(scene->bodies);
}

void scene_free(scene_t *scene){
    scene_bodies_free(scene);
    scene_forces_free(scene);
    free(scene);
}

size_t scene_bodies(scene_t *scene){
    return list_size(scene->bodies);
}

body_t *scene_get_body(scene_t *scene, size_t index){
    return list_get(scene->bodies, index);
}

void scene_add_body(scene_t *scene, body_t *body){
    return list_add(scene->bodies, body);
}

void scene_remove_body(scene_t *scene, size_t index) {
    assert(index < list_size(scene->bodies));
    body_remove(list_get(scene->bodies, index));
}

void scene_add_force_creator(scene_t *scene, force_creator_t forcer, void *aux, free_func_t freer){
    list_add(scene->forces, force_init(forcer, aux, freer));
}

void scene_add_bodies_force_creator(scene_t *scene, force_creator_t forcer, void *aux, list_t *bodies, free_func_t freer){
    list_add(scene->forces, force_init_with_bodies(forcer, aux, freer, bodies));
}

void *scene_get_state(scene_t *scene){
    return scene->state;
}

void scene_set_state(scene_t *scene, void *state){
    free(scene->state);
    scene->state = state;
}

void scene_tick(scene_t *scene, double dt){
    //creates force if force is not removed else removes force from list
    for(size_t i = 0; i < list_size(scene->forces); i++){
        force_t *force = list_get(scene->forces, i);
        force_create(force);
    }
    //moves body if body is not removed else removes body from list
    for(size_t i = 0; i < scene_bodies(scene); i++){
        body_tick(list_get(scene->bodies, i), dt);
    }
    //flags forces with remove if any of their corresponding bodies are removed
    for(size_t i = 0; i < list_size(scene->forces); i++){
        list_t *bodies = force_get_bodies(list_get(scene->forces, i));
        for(size_t j = 0; j < list_size(bodies); j++){
            if(body_is_removed(list_get(bodies, j))){
                force_remove(list_get(scene->forces, i));
                break;
            }
        }
    }
    for(size_t i = 0; i < list_size(scene->forces); i++){
        force_t *force = list_get(scene->forces, i);
        if(force_is_removed(force)){
            force_free(list_remove(scene->forces, i));
            i--;
        }
    }
    for(size_t i = 0; i<scene_bodies(scene); i++){
        if(body_is_removed(list_get(scene->bodies, i))){
            body_free(list_remove(scene->bodies, i));
            i--;
        }
    }
}