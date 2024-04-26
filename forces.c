#include <math.h>
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include "forces.h"
#include "scene.h"
#include "collision.h"
#include "vector.h"

const double MIN_DIST = 3;

typedef struct aux{
    double G;
    double k;
    double gamma;
    double elasticity;
    body_t *body1;
    body_t *body2;
    vector_t force;
    collision_handler_t handler;
    free_func_t handler_freer;
    void* aux;
    bool recent_col;
    double friction;
    double angle;
    vector_t slope_direction;
    double min_vel_magnitude;
}aux_t;

aux_t *aux_init(){
    aux_t *aux = malloc(sizeof(aux_t));
    aux->handler_freer = NULL;
    aux->min_vel_magnitude = 0;
    assert(aux);
    return aux;
}

void aux_free(aux_t *aux) {
    if(aux->handler_freer != NULL) aux->handler_freer(aux->aux);
    free(aux);
}

// force on body2 from body 1
void newtonian_gravity(aux_t *gravity){
    vector_t displacement = vec_subtract(body_get_centroid(gravity->body1), body_get_centroid(gravity->body2));
    vector_t unit_disp = vec_multiply(1/sqrt(vec_dot(displacement, displacement)), displacement);
    double distance = sqrt(vec_dot(displacement, displacement));
    if(distance > MIN_DIST){
        vector_t grav_force = vec_multiply(gravity->G * body_get_mass(gravity->body1) * body_get_mass(gravity->body2)/vec_dot(displacement, displacement), unit_disp);
        body_add_force(gravity->body2, grav_force);
        body_add_force(gravity->body1, vec_multiply(-1, grav_force));
    }
}

void create_newtonian_gravity(scene_t *scene, double G, body_t *body1, body_t *body2) {
    //add force to bodies
    aux_t *gravity = aux_init();
    gravity->body1 = body1;
    gravity->body2 = body2;
    gravity->G = G;
    list_t *bodies = list_init(2, (free_func_t) free);
    list_add(bodies, body1);
    list_add(bodies, body2);
    scene_add_bodies_force_creator(scene, (force_creator_t) newtonian_gravity, gravity, bodies, (free_func_t) aux_free);
}

void spring(aux_t *spring){
    vector_t displacement = vec_subtract(body_get_centroid(spring->body2), body_get_centroid(spring->body1));
    vector_t spring_force = vec_multiply(spring->k, displacement);
    body_add_force(spring->body1, spring_force);
}

void create_spring(scene_t *scene, double k, body_t *body1, body_t *body2) {
    aux_t *s = aux_init();
    s->body1 = body1;
    s->body2 = body2;
    s->k = k;
    list_t *bodies = list_init(2, (free_func_t) free);
    list_add(bodies, body1);
    list_add(bodies, body2);
    scene_add_bodies_force_creator(scene, (force_creator_t) spring, s, bodies, (free_func_t) aux_free);
}

void drag(aux_t *drag){
    vector_t cons_force = vec_multiply(-1 * drag->gamma, body_get_velocity(drag->body1));
    body_add_force(drag->body1, cons_force);
}

void create_drag(scene_t *scene, double gamma, body_t *body) {
    aux_t *d = aux_init();
    d->body1 = body;
    d->gamma = gamma;
    list_t *bodies = list_init(2, (free_func_t) free);
    list_add(bodies, body);
    scene_add_bodies_force_creator(scene, (force_creator_t) drag, d, bodies, (free_func_t) aux_free);
}

void collision(aux_t *aux){
    body_t *body1 = aux->body1;
    body_t *body2 = aux->body2;
    list_t *l1 = body_get_shape(body1);
    list_t *l2 = body_get_shape(body2);
    collision_info_t collision_axis = find_collision(l1, l2);
    if(collision_axis.collided && !aux->recent_col){
        aux->handler(body1, body2, collision_axis.axis, aux->aux);
        aux->recent_col = true;
    }
    else if(!collision_axis.collided){
        aux->recent_col = false;
    }
    list_free(l1);
    list_free(l2);
}

void create_collision(scene_t *scene, body_t *body1, body_t *body2, collision_handler_t handler, void *aux, free_func_t freer){
    aux_t *collide = aux_init();
    collide->body1 = body1;
    collide->body2 = body2;
    collide->aux = aux;
    collide->handler = handler;
    collide->handler_freer = freer;
    collide->recent_col = false;
    list_t *bodies = list_init(2, (free_func_t) free);
    list_add(bodies, body1);
    list_add(bodies, body2);
    scene_add_bodies_force_creator(scene, (force_creator_t) collision, collide, bodies, (free_func_t) aux_free);
}

void destructive_collision(body_t *body1, body_t *body2, vector_t axis, aux_t *collide){
    body_remove(body1);
}

void create_destructive_collision(scene_t *scene, body_t *body1, body_t *body2){
    create_collision(scene, body1, body2, (collision_handler_t) destructive_collision, NULL, NULL);
}

void physics_collision(body_t *body1, body_t *body2, vector_t axis, aux_t *collide) {
    double mass1 = body_get_mass(body1);
    double mass2 = body_get_mass(body2);
    
    vector_t vel1 = body_get_velocity(body1);
    vector_t vel2 = body_get_velocity(body2);

    double mag = vec_dot(axis, axis);
    vector_t u_a = vec_multiply(1/mag, vec_multiply(vec_dot(vel1, axis), axis));
    vector_t u_b = vec_multiply(1/mag, vec_multiply(vec_dot(vel2, axis), axis));
    double reduced_mass;
    if (mass1 == INFINITY) {
        reduced_mass = mass2;
    }
    else if (mass2 == INFINITY) {
        reduced_mass = mass1;
    }
    else {
        reduced_mass = (mass1 * mass2) / (mass1 + mass2);
    }
    vector_t J_n = vec_multiply(reduced_mass*(1 + collide->elasticity), vec_subtract(u_b, u_a));
    body_add_impulse(body1, J_n);
    body_add_impulse(body2, vec_negate(J_n));
}

void create_physics_collision(scene_t *scene, double elasticity, body_t *body1, body_t *body2) {
    aux_t *col = aux_init();
    col->body1 = body1;
    col->body2 = body2;
    col->elasticity = elasticity;
    create_collision(scene, body1, body2, (collision_handler_t) physics_collision, col, (free_func_t) aux_free);
}

void friction_and_slope_force(aux_t *fric_slope) {
    list_t *l1 = body_get_shape(fric_slope->body1);
    list_t *l2 = body_get_shape(fric_slope->body2);
    collision_info_t collision_axis = find_collision(l1, l2);
    if(collision_axis.collided && vec_dot(body_get_velocity(fric_slope->body1), body_get_velocity(fric_slope->body1)) > 0.001){
        vector_t g_force = vec_multiply(fric_slope->G * sin(fric_slope->angle), fric_slope->slope_direction);
        vector_t unit_velocity_direc = vec_multiply(1/vec_dot(body_get_velocity(fric_slope->body1), body_get_velocity(fric_slope->body1)), body_get_velocity(fric_slope->body1));
        vector_t fric_force = vec_multiply(fric_slope->G * cos(fric_slope->angle) * fric_slope->friction, vec_negate(unit_velocity_direc));
        vector_t cons_force = vec_multiply(body_get_mass(fric_slope->body1), vec_add(g_force, fric_force));
        body_add_force(fric_slope->body1, cons_force);
    }
    list_free(l1);
    list_free(l2);
}

void create_frictional_and_slope_force(scene_t *scene, double u_k, double theta, vector_t slope_direc, double g, body_t *body1, body_t *body2) {
    aux_t *d = aux_init();
    d->body1 = body1;
    d->body2 = body2;
    d->friction = u_k;
    d->angle = theta;
    d->G = g;
    d->slope_direction = slope_direc;
    list_t *bodies = list_init(2, (free_func_t) free);
    list_add(bodies, body1);
    scene_add_bodies_force_creator(scene, (force_creator_t) friction_and_slope_force, d, bodies, (free_func_t) aux_free);
}

void force_collision(aux_t *aux){
    body_t *body1 = aux->body1;
    body_t *body2 = aux->body2;
    list_t *l1 = body_get_shape(body1);
    list_t *l2 = body_get_shape(body2);
    collision_info_t collision_axis = find_collision(l1, l2);
    if(collision_axis.collided){
        body_add_force(body1, aux->force);
    }
    list_free(l1);
    list_free(l2);
}

void create_force_collision(scene_t *scene, vector_t force, body_t *body1, body_t *body2) {
    aux_t *col = aux_init();
    col->body1 = body1;
    col->body2 = body2;
    col->force = force;
    list_t *bodies = list_init(2, (free_func_t) free);
    list_add(bodies, body1);
    list_add(bodies, body2);
    scene_add_bodies_force_creator(scene, (force_creator_t) force_collision, col, bodies, (free_func_t) aux_free);
}

void friction(aux_t *aux){
    body_t *body1 = aux->body1;
    body_t *body2 = aux->body2;
    list_t *l1 = body_get_shape(body1);
    list_t *l2 = body_get_shape(body2);
    collision_info_t collision_axis = find_collision(l1, l2);
    if(collision_axis.collided && vec_dot(body_get_velocity(body1), body_get_velocity(body1)) > 0.001){
        vector_t direction = vec_multiply(1/sqrt(vec_dot(body_get_velocity(body1), body_get_velocity(body1))), body_get_velocity(body1));
        body_add_force(body1, vec_multiply(-1 * aux->friction * body_get_mass(body1), direction));
    }
    list_free(l1);
    list_free(l2);
}

void create_friction(scene_t *scene, double frict, body_t *body1, body_t *body2) {
    aux_t *col = aux_init();
    col->body1 = body1;
    col->body2 = body2;
    col->friction = frict;
    list_t *bodies = list_init(2, (free_func_t) free);
    list_add(bodies, body1);
    list_add(bodies, body2);
    scene_add_bodies_force_creator(scene, (force_creator_t) friction, col, bodies, (free_func_t) aux_free);
}
