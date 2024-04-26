#ifndef __FORCE_H__
#define __FORCE_H__

#include "scene.h"
#include "force.h"

/**
 * A function which adds some forces or impulses to bodies,
 * e.g. from collisions, gravity, or spring forces.
 * Takes in an auxiliary value that can store parameters or state.
 */
typedef void (*force_creator_t)(void *aux);

/**
 * A force implementation
 */
typedef struct force force_t;

force_t *force_init(force_creator_t fc, void *aux, free_func_t freer);

force_t *force_init_with_bodies(force_creator_t fc, void *aux, free_func_t freer, list_t *bodies);

/**
 * Calls the specific force function from the forces file that we want to use.
 */
void force_create(force_t *force);

/**
 * Freer for the force_t struct.
 */
void force_free(force_t *force);

/**
 * Marks a body for removal--future calls to body_is_removed() will return true.
 * Does not free the body.
 * If the body is already marked for removal, does nothing.
 *
 * @param body the body to mark for removal
 */
void force_remove(force_t *force);

/**
 * Returns whether a body has been marked for removal.
 * This function returns false until body_remove() is called on the body,
 * and returns true afterwards.
 *
 * @param body the body to check
 * @return whether body_remove() has been called on the body
 */
bool force_is_removed(force_t *force);

list_t *force_get_bodies(force_t *force);

    
#endif // #ifndef __FORCE_H__