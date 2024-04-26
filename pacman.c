#include <math.h>
#include <stdlib.h>
#include <stdio.h>
#include <assert.h>
#include "sdl_wrapper.h"
#include "scene.h"
#include "polygon.h"
#include "body.h"
#include "list.h"
#include "vector.h"

const vector_t MAX_CANVAS_SIZE = {.x=1000, .y=500};
const double PELLET_SIZE = 7;
const double PACMAN_SIZE = 50;
const double PELLET_TIME = 0.5;
//the color yellow
const rgb_color_t COLOR = {.r=1, .g=1, .b=0};
const int CIRCLE_POINTS = 20;
const double MASS = 10;
const double ACCELERATION = 1000;
const int PACMAN_MOUTH = 4;
const double DEFAULT_TIME = 0.1;
const double TIME_CAP = 0.001;
const double FIX_ROTATE = M_PI/20 - M_PI/2;
const int INITIAL_PELLET_NUM = 10;

//rotates and changes the velocity of the pacman according to the arrow key pressed
void move_pacman(int new_dir, double held_time, scene_t *scene){
    body_t *pacman = scene_get_body(scene, 0);
    int curr_dir = body_get_orientation(pacman);
    vector_t move = {.x = 0, .y = 0};

    if(new_dir == RIGHT_ARROW) {
        move.x = 1;
    }
    else if(new_dir == LEFT_ARROW) {
        move.x = -1;
    }
    else if(new_dir == DOWN_ARROW) {
        move.y = -1;
    }
    else {
        move.y = 1;
    }

    double rotate = -(M_PI/2) * (new_dir - curr_dir);
    body_set_rotation(pacman, rotate);
    if(held_time < TIME_CAP){
        body_set_velocity(pacman, vec_multiply(DEFAULT_TIME*ACCELERATION, move));
    }
    else body_set_velocity(pacman, vec_multiply(held_time*ACCELERATION, move));
    body_set_orientation(pacman, new_dir);
}

//deals with the key events based on the arrow key pressed and the time held
void on_key(char key, key_event_type_t type, double held_time, void *scene) {
    if (type == KEY_PRESSED) {
        switch(key) {
            case RIGHT_ARROW:
                move_pacman(RIGHT_ARROW, held_time, scene);
                break;
            case LEFT_ARROW:
                move_pacman(LEFT_ARROW, held_time, scene);
                break;
            case DOWN_ARROW:
                move_pacman(DOWN_ARROW, held_time, scene);
                break;
            case UP_ARROW:
                move_pacman(UP_ARROW, held_time, scene);
                break;
        }
    }
}

list_t *draw_circle(vector_t center, size_t size){
    list_t *circle = list_init(CIRCLE_POINTS, (free_func_t) body_free_vec_list);
    for (size_t i = 0; i < CIRCLE_POINTS; i++) {
        vector_t *outer_point = malloc(sizeof(vector_t));
        assert(outer_point);
        outer_point->x = center.x + (size * sin(2 * M_PI * i / CIRCLE_POINTS));
        outer_point->y = center.y - (size * cos(2 * M_PI * i / CIRCLE_POINTS));
        list_add(circle, outer_point);
    }
    return circle;
}

//adds a pellet at a random location on the scene
void add_pellet(scene_t *scene){
    double pos_x = ((double) (rand() % ((int) MAX_CANVAS_SIZE.x)));
    double pos_y = ((double) (rand() % ((int) MAX_CANVAS_SIZE.y)));
    vector_t center = {.x=pos_x, .y=pos_y};
    list_t *circle = draw_circle(center, PELLET_SIZE);
    body_t *new_pellet = body_init(circle, MASS, COLOR);
    scene_add_body(scene, new_pellet);
}

//checks if pacman runs into pellets
void eat_pellet(scene_t *scene){
    body_t *pacman = scene_get_body(scene, 0);
    vector_t pacman_center = body_get_centroid(pacman);
    for(size_t i = 1; i < scene_bodies(scene); i++){
        body_t *pellet = scene_get_body(scene, i);
        vector_t pellet_center = body_get_centroid(pellet);
        bool x_bound = pellet_center.x < pacman_center.x + PACMAN_SIZE && pellet_center.x > pacman_center.x - PACMAN_SIZE;
        bool y_bound = pellet_center.y < pacman_center.y + PACMAN_SIZE && pellet_center.y > pacman_center.y - PACMAN_SIZE;
        if(x_bound && y_bound){
            scene_remove_body(scene, i);
            break;
        }
    }
}

body_t *add_pacman(vector_t center, size_t size){
    list_t *pacman = list_init(CIRCLE_POINTS - PACMAN_MOUTH, (free_func_t) body_free_vec_list);
    for (size_t i = 0; i < (CIRCLE_POINTS - PACMAN_MOUTH)/2; i++) {
        vector_t *outer_point = malloc(sizeof(vector_t));
        assert(outer_point);
        outer_point->x = center.x + (size * sin(2 * M_PI * i / CIRCLE_POINTS));
        outer_point->y = center.y - (size * cos(2 * M_PI * i / CIRCLE_POINTS));
        list_add(pacman, outer_point);
    }
    vector_t *outer_point = malloc(sizeof(vector_t));
    assert(outer_point);
    outer_point->x = center.x;
    outer_point->y = center.y;
    list_add(pacman, outer_point);
    
    for (size_t i = (CIRCLE_POINTS + PACMAN_MOUTH)/2; i < CIRCLE_POINTS; i++) {
        vector_t *outer_point = malloc(sizeof(vector_t));
        assert(outer_point);
        outer_point->x = center.x + (size * sin(2 * M_PI * i / CIRCLE_POINTS));
        outer_point->y = center.y - (size * cos(2 * M_PI * i / CIRCLE_POINTS));
        list_add(pacman, outer_point);
    }
    polygon_rotate(pacman, FIX_ROTATE, center);
    body_t *pacman_body = body_init(pacman, MASS, COLOR);
    return pacman_body;
}

//helper method check if the pacman is out of bounds
vector_t out_bounds(list_t* pacman_points){
    //keeps track of of the four sides (x_left, x_right, y_bottom, y_top) respectively
    bool check_x_under = 1;
    bool check_x_above = 1;
    bool check_y_under = 1;
    bool check_y_above = 1;

    for (size_t i = 0; i < list_size(pacman_points); i++) {
        vector_t *cur_point = list_get(pacman_points, i);
        if(check_x_under && (cur_point->x > 0)){
            check_x_under = 0;
        }
        if(check_x_above && (cur_point->x < MAX_CANVAS_SIZE.x)){
            check_x_above = 0;
        }
        if(check_y_under && (cur_point->y > 0)){
            check_y_under = 0;
        }
        if(check_y_above && (cur_point->y < MAX_CANVAS_SIZE.y)){
            check_y_above = 0;
        }
    }

    vector_t diff = {.x = 0, .y = 0};
    if(check_x_under){
        diff.x = MAX_CANVAS_SIZE.x;
    }
    else if(check_x_above){
        diff.x = -MAX_CANVAS_SIZE.x;
    }
    else if(check_y_under){
        diff.y = MAX_CANVAS_SIZE.y;
    }
    else if(check_y_above){
        diff.y = -MAX_CANVAS_SIZE.y;
    }
    return diff;
}

//changes the pacman's position if its fully offscreen
void offscreen(scene_t *scene){
    body_t *pacman = scene_get_body(scene, 0);
    list_t *pacman_points = body_get_shape(pacman);
    body_set_centroid(pacman, vec_add(body_get_centroid(pacman), out_bounds(pacman_points)));
}

int main(int argc, char *argv[]) {
    sdl_init(VEC_ZERO, MAX_CANVAS_SIZE);
    scene_t *scene = scene_init();
    sdl_on_key(on_key);

    vector_t start_center = {.x = MAX_CANVAS_SIZE.x/2, .y = MAX_CANVAS_SIZE.y/2};
    body_t *pacman = add_pacman(start_center, PACMAN_SIZE);
    body_set_orientation(pacman, RIGHT_ARROW);
    scene_add_body(scene, pacman);

    double dt_from_last_pellet = 0;
    
    for(size_t i = 0; i < INITIAL_PELLET_NUM; i++){
        add_pellet(scene);
    }

    //runs untill the window closes
    while (!sdl_is_done(scene)) {
        double dt = time_since_last_tick();
        dt_from_last_pellet += dt;

        //adds a new pellet to the screen once enough time has elapsed
        if (dt_from_last_pellet > PELLET_TIME) {
            add_pellet(scene);
            dt_from_last_pellet = 0;
        }

        eat_pellet(scene);
        offscreen(scene);

        scene_tick(scene, dt);

        sdl_render_scene(scene);
    }
    return 0;
}