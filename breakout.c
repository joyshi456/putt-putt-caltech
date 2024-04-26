#include <math.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <assert.h>
#include "sdl_wrapper.h"
#include "polygon.h"
#include "scene.h"
#include "forces.h"
#include "body.h"
#include "list.h"
#include "vector.h"

const vector_t MAX_CANVAS_SIZE = {.x=800, .y=800};
const double WALL_SIZE = 5;
const double BRICK_SIZE = 20;
const double PLAYER_SIZE = 30;
const double BALL_PADDLE_RATIO = 0.3;
const vector_t PADDLE_START_POS = {.x=250, .y=20};
const vector_t BALL_START_POS = {.x=290, .y=60};
const vector_t START_VEL = {.x=500, .y=500};
//the color white
const rgb_color_t WALL_COLOR = {.r=1, .g=1, .b=1};
//the color purple
const rgb_color_t PLAYER_COLOR = {.r=.87, .g=0.69, .b=0.95};
//the color blue
const rgb_color_t PELLET_COLOR = {.r=0, .g=0.69, .b=0.95};
const int POINTS = 4;
const double MASS = 10;
const double BUFFER = 10;
const double NUM_ROWS = 3;
const double TRANSLATION = 10;
const int CIRCLE_POINTS = 20;
const double ELASTICITY = 1;
const double PELLET_TIME = 1;
const double FORCE_MULTIPLIER = 1.2;
const double RGB_INC_HIGH = 0.75;
const double RGB_INC_LOW = 0.50;
const double WALL_MULTIPLIER = 4;
const double BRICK_MULTIPLIER = 2;

void add_wall(scene_t *scene, double min_x, double max_x, double min_y, double max_y, char *type){
    list_t *wall_points = list_init(POINTS, (free_func_t) body_free_vec_list);
    vector_t *point1 = malloc(sizeof(vector_t));
    point1->x = min_x;
    point1->y = min_y;
    list_add(wall_points, point1);
    vector_t *point2 = malloc(sizeof(vector_t));
    point2->x = max_x;
    point2->y = min_y;
    list_add(wall_points, point2);
    vector_t *point3 = malloc(sizeof(vector_t));
    point3->x = max_x;
    point3->y = max_y;
    list_add(wall_points, point3);
    vector_t *point4 = malloc(sizeof(vector_t));
    point4->x = min_x;
    point4->y = max_y;
    list_add(wall_points, point4);
    body_t *wall = body_init_with_info(wall_points, INFINITY, WALL_COLOR, type, free);
    scene_add_body(scene, wall);
}

void add_walls(scene_t *scene){
    //left bounce wall
    add_wall(scene, 0, WALL_SIZE, 0, MAX_CANVAS_SIZE.y, "bounce_wall");
    //right bounce wall
    add_wall(scene, MAX_CANVAS_SIZE.x - WALL_SIZE, MAX_CANVAS_SIZE.x, 0, MAX_CANVAS_SIZE.y, "bounce_wall");
    //top bounce wall
    add_wall(scene, 0, MAX_CANVAS_SIZE.x, MAX_CANVAS_SIZE.x - WALL_SIZE, MAX_CANVAS_SIZE.y, "bounce_wall");
    //bottom reset wall
    add_wall(scene, 0, MAX_CANVAS_SIZE.x, 0, WALL_SIZE, "lose_wall");
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

void add_player(scene_t *scene, vector_t center, size_t size){
    list_t *player_points = list_init(POINTS, (free_func_t) body_free_vec_list);
    //creates a square
    vector_t *point1 = malloc(sizeof(vector_t));
    point1->x = center.x + size;
    point1->y = center.y + size/2;
    list_add(player_points, point1);

    vector_t *point2 = malloc(sizeof(vector_t));
    point2->x = center.x - size;
    point2->y = center.y + size/2;
    list_add(player_points, point2);

    vector_t *point3 = malloc(sizeof(vector_t));
    point3->x = center.x - size;
    point3->y = center.y - size/2;
    list_add(player_points, point3);

    vector_t *point4 = malloc(sizeof(vector_t));
    point4->x = center.x + size;
    point4->y = center.y - size/2;
    list_add(player_points, point4);

    body_t *player = body_init_with_info(player_points, INFINITY, PLAYER_COLOR, "player_paddle", free);
    scene_add_body(scene, player);

    vector_t circle_center = BALL_START_POS;
    list_t *circle = draw_circle(circle_center, size * BALL_PADDLE_RATIO);
    body_t *ball = body_init_with_info(circle, 10, PLAYER_COLOR, "player_ball", free);
    body_set_velocity(ball, START_VEL);
    scene_add_body(scene, ball);
}

body_t *add_brick(vector_t center, size_t size, rgb_color_t color) {
    list_t *brick = list_init(POINTS, (free_func_t) body_free_vec_list);
    
    vector_t *point1 = malloc(sizeof(vector_t));
    point1->x = center.x + size;
    point1->y = center.y + size/2;
    list_add(brick, point1);

    vector_t *point2 = malloc(sizeof(vector_t));
    point2->x = center.x - size;
    point2->y = center.y + size/2;
    list_add(brick, point2);

    vector_t *point3 = malloc(sizeof(vector_t));
    point3->x = center.x - size;
    point3->y = center.y - size/2;
    list_add(brick, point3);

    vector_t *point4 = malloc(sizeof(vector_t));
    point4->x = center.x + size;
    point4->y = center.y - size/2;
    list_add(brick, point4);

    body_t *brick_body = body_init_with_info(brick, INFINITY, color, "brick", free);
    return brick_body;
}

void add_row_of_bricks(scene_t *scene) {
    int num_col = (MAX_CANVAS_SIZE.x/(BRICK_SIZE * BRICK_MULTIPLIER + BUFFER)) - 1;
    for(size_t i = 0; i< num_col; i++){
        for(size_t j = 0; j < NUM_ROWS; j++){
            vector_t center = {.x = (i + 1) * (BRICK_SIZE * BRICK_MULTIPLIER + BUFFER), .y = MAX_CANVAS_SIZE.y - (j + 1) * (BRICK_SIZE + BUFFER)};
            rgb_color_t color;
            if(i < num_col/2){
                color = (rgb_color_t) {.r = 1 - (double)i/(num_col), .g = (double)i/(num_col * RGB_INC_LOW), .b = 0};
            }
            else if(i < 3 * num_col/4){
                color = (rgb_color_t) {.r=0, .g= (double)1 - ((double)i-(num_col / 2))/((3 * num_col/4 - num_col/2) - 1), .b= (double)(i-(num_col / 2))/(3 * num_col/4 - num_col/2 - 1)};  
            }
            else{
                color = (rgb_color_t) {.r=(double)(i-(3 * num_col/4))/(num_col - 3 * num_col/4), .g= 0, .b= (double)1 - ((double)i-(3 * num_col/4))/((num_col - 3 * num_col/4))}; 
            }

            body_t *brick = add_brick(center, BRICK_SIZE, color);
            scene_add_body(scene, brick);
        }
    }
}

void special_force(body_t *ball, body_t *target, vector_t axis, scene_t *scene){
    body_set_velocity(ball, vec_multiply(FORCE_MULTIPLIER, body_get_velocity(ball)));
}

//This is the special feature that we have implemented as per the specs of the project.
void add_pellet(scene_t *scene){
    double pos_x = BRICK_MULTIPLIER * WALL_SIZE + ((double) (rand() % (int) (MAX_CANVAS_SIZE.x - (WALL_MULTIPLIER * WALL_SIZE))));
    double pos_y = WALL_SIZE + PLAYER_SIZE + ((double) (rand() % (int) (MAX_CANVAS_SIZE.y - (WALL_SIZE * WALL_MULTIPLIER) - PLAYER_SIZE - (NUM_ROWS * (BRICK_SIZE * BRICK_MULTIPLIER + BUFFER)))));
    vector_t center = {.x=pos_x, .y=pos_y};
    list_t *circle = draw_circle(center, PLAYER_SIZE);
    body_t *new_pellet = body_init_with_info(circle, MASS, PELLET_COLOR, "pellet", free);
    scene_add_body(scene, new_pellet);
    body_t *player;
    for(size_t i = 0; i < scene_bodies(scene); i++){
        player = scene_get_body(scene, i);
        if(strcmp(body_get_info(player), "player_ball") == 0){
            body_t *pellet = scene_get_body(scene, scene_bodies(scene) - 1);
            create_collision(scene, player, pellet, (collision_handler_t) special_force, scene, NULL);
            create_destructive_collision(scene, pellet, player);
        }
        if(strcmp(body_get_info(player), "player_paddle") == 0){
            body_t *pellet = scene_get_body(scene, scene_bodies(scene) - 1);
            create_destructive_collision(scene, pellet, player);
        }
    }
}

void add_forces_bricks(scene_t *scene){
    body_t *player;
    for(size_t i = 0; i < scene_bodies(scene); i++){
        player = scene_get_body(scene, i);
        if(strcmp(body_get_info(player), "player_ball") == 0){
            for (size_t i = 0; i < scene_bodies(scene); i++) {
                body_t *body = scene_get_body(scene, i);
                if(strcmp(body_get_info(body), "brick") == 0){
                    create_physics_collision(scene, ELASTICITY, body, player);
                    create_destructive_collision(scene, body, player);
                }
            }
        }
    }
}

void reset(body_t *ball, body_t *target, vector_t axis, scene_t *scene){
    for(int i = 0; i < scene_bodies(scene); i++){
        body_t *body = scene_get_body(scene, i);
        if(strcmp(body_get_info(body), "brick") == 0 || strcmp(body_get_info(body), "pellet") == 0) {
            scene_remove_body(scene, i);
        }
        else if(strcmp(body_get_info(body), "player_ball") == 0) {
            body_set_centroid(body, BALL_START_POS);
            body_set_velocity(body, START_VEL);
        }
        else if(strcmp(body_get_info(body), "player_paddle") == 0) {
            body_set_centroid(body, PADDLE_START_POS);
        }
    }
    add_row_of_bricks(scene);
    add_forces_bricks(scene);
    
}

void add_forces(scene_t *scene){
    body_t *player;
    for(size_t i = 0; i < scene_bodies(scene); i++){
        player = scene_get_body(scene, i);
        if(strcmp(body_get_info(player), "player_ball") == 0){
            for (size_t i = 0; i < scene_bodies(scene); i++) {
                body_t *body = scene_get_body(scene, i);
                if(strcmp(body_get_info(body), "bounce_wall") == 0){
                    create_physics_collision(scene, ELASTICITY, body, player);
                }
                else if(strcmp(body_get_info(body), "player_paddle") == 0){
                    create_physics_collision(scene, ELASTICITY, body, player);
                }
                else if(strcmp(body_get_info(body), "lose_wall") == 0){
                    create_collision(scene, body, player, (collision_handler_t) reset, scene, NULL);
                }
            }
        }
    }
    add_forces_bricks(scene);
}

//moves the player according to the arrow key pressed
void move_player(int new_dir, scene_t *scene){
    for(size_t i = 0; i < scene_bodies(scene); i++){
        body_t *body = scene_get_body(scene, i);
        if(strcmp(body_get_info(body), "player_paddle") == 0){
            vector_t move = VEC_ZERO;
            if(new_dir == RIGHT_ARROW) {
                //only moves if will be onscreen
                if(body_get_centroid(body).x < MAX_CANVAS_SIZE.x - (PLAYER_SIZE * 2)) move.x = TRANSLATION;
            }
            else if(new_dir == LEFT_ARROW) {
                //only moves if will be onscreen
                if(body_get_centroid(body).x > PLAYER_SIZE) move.x = -TRANSLATION;
            }
            body_set_centroid(body, vec_add(body_get_centroid(body), move));
            return;
        }
    }
}

//deals with the key events based on the arrow key pressed
void on_key(char key, key_event_type_t type, double held_time, void *scene) {
    if (type == KEY_PRESSED) {
        switch(key) {
            case RIGHT_ARROW:
                move_player(RIGHT_ARROW, scene);
                break;
            case LEFT_ARROW:
                move_player(LEFT_ARROW, scene);
                break;
        }
    }
}

int main(int argc, char *argv[]) {
    //initialization
    sdl_init(VEC_ZERO, MAX_CANVAS_SIZE);
    scene_t *scene = scene_init();
    sdl_on_key(on_key);
    add_row_of_bricks(scene);
    add_walls(scene);
    add_player(scene, PADDLE_START_POS, PLAYER_SIZE);
    add_forces(scene);

    double dt_from_last_pellet = 0;
    //runs until the window closes
    while (!sdl_is_done(scene)) {
        double dt = time_since_last_tick();
        dt_from_last_pellet += dt;

        //adds a pellet
        if(dt_from_last_pellet >= PELLET_TIME){
            add_pellet(scene);
            dt_from_last_pellet = 0;
        }

        scene_tick(scene, dt);
        sdl_render_scene(scene);
    }
    scene_free(scene);
    return 0;
}