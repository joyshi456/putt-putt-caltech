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
const double BULLET_SIZE = 7;
const double INVADER_SIZE = 30;
const double PLAYER_SIZE = 20;
const vector_t PLAYER_START_POS = {.x=250, .y=20};
const double BULLET_TIME = 0.5;
const vector_t START_VEL = {.x=100, .y=0};
const vector_t PLAYER_BULLET_VEL = {.x=0, .y=700};
//the color gray
const rgb_color_t INVADER_COLOR = {.r=0.5, .g=0.5, .b=0.5};
//the color purple
const rgb_color_t PLAYER_COLOR = {.r=.87, .g=0.69, .b=0.95};
const int CIRCLE_POINTS = 20;
const double MASS = 10;
const int INVADER_POINT = 10;
const double FIX_ROTATE = M_PI/20 + M_PI/2;
const double BUFFER = 10;
const double NUM_ROWS = 3;
const double TRANSLATION = 10;

body_t *add_invader(vector_t center, size_t size){
    list_t *invader = list_init(CIRCLE_POINTS - INVADER_POINT, (free_func_t) body_free_vec_list);
    for (size_t i = 0; i < (CIRCLE_POINTS - INVADER_POINT); i++) {
        vector_t *outer_point = malloc(sizeof(vector_t));
        assert(outer_point);
        outer_point->x = center.x + (size * sin(2 * M_PI * i / CIRCLE_POINTS));
        outer_point->y = center.y - (size * cos(2 * M_PI * i / CIRCLE_POINTS));
        list_add(invader, outer_point);
    }
    vector_t *outer_point = malloc(sizeof(vector_t));
    assert(outer_point);
    outer_point->x = center.x;
    outer_point->y = center.y;
    list_add(invader, outer_point);
    
    polygon_rotate(invader, FIX_ROTATE, center);
    body_t *invader_body = body_init_with_info(invader, MASS, INVADER_COLOR, "invader", free);
    return invader_body;
}

body_t *add_player(vector_t center, size_t size){
    list_t *player = list_init(CIRCLE_POINTS - INVADER_POINT, (free_func_t) body_free_vec_list);
    for (size_t i = 0; i < (CIRCLE_POINTS - INVADER_POINT); i++) {
        vector_t *outer_point = malloc(sizeof(vector_t));
        assert(outer_point);
        outer_point->x = center.x + (size * sin(2 * M_PI * i / CIRCLE_POINTS));
        outer_point->y = center.y - (size * cos(2 * M_PI * i / CIRCLE_POINTS));
        list_add(player, outer_point);
    }
    polygon_rotate(player, FIX_ROTATE, center);
    body_t *player_body = body_init_with_info(player, MASS, PLAYER_COLOR, "player", free);
    return player_body;
}

body_t *add_bullet(vector_t center, size_t size, bool is_player){
    list_t *bullet_points = list_init(CIRCLE_POINTS - INVADER_POINT, (free_func_t) body_free_vec_list);
    //creates a square
    vector_t *point1 = malloc(sizeof(vector_t));
    point1->x = center.x + size/2;
    point1->y = center.y + size/2;
    list_add(bullet_points, point1);

    vector_t *point2 = malloc(sizeof(vector_t));
    point2->x = center.x - size/2;
    point2->y = center.y + size/2;
    list_add(bullet_points, point2);

    vector_t *point3 = malloc(sizeof(vector_t));
    point3->x = center.x - size/2;
    point3->y = center.y - size/2;
    list_add(bullet_points, point3);

    vector_t *point4 = malloc(sizeof(vector_t));
    point4->x = center.x + size/2;
    point4->y = center.y - size/2;
    list_add(bullet_points, point4);

    body_t *bullet;
    if(is_player){
        bullet = body_init_with_info(bullet_points, MASS, PLAYER_COLOR, "player_bullet", free);
    }
    else{
        bullet = body_init_with_info(bullet_points, MASS, INVADER_COLOR, "invader_bullet", free);
    }
    return bullet;
}

//moves the player according to the arrow key pressed
void move_player(int new_dir, scene_t *scene){
    for(size_t i = 0; i < scene_bodies(scene); i++){
        body_t *body = scene_get_body(scene, i);
        if(strcmp(body_get_info(body), "player") == 0){
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

void player_shoot(scene_t *scene){
    for(size_t i = 0; i < scene_bodies(scene); i++){
        body_t *player= scene_get_body(scene, i);
        if(strcmp(body_get_info(player), "player") == 0){
            //shoots bullet from player
            body_t *bullet = add_bullet(body_get_centroid(player), BULLET_SIZE, true);
            scene_add_body(scene, bullet);
            body_set_velocity(bullet, PLAYER_BULLET_VEL);
            //adds destructive force between player bullet and all of the invaders
            for(size_t i = 0; i < scene_bodies(scene); i++){
                body_t *body = scene_get_body(scene, i);
                if(strcmp(body_get_info(body), "invader") == 0){
                    create_destructive_collision(scene, body, bullet);
                }
            }
            return;
        }
    }
}

int get_num_invader(scene_t *scene){
    int count = 0;
    for(size_t i = 0; i < scene_bodies(scene); i++){
        body_t *body = scene_get_body(scene, i);
        if(strcmp(body_get_info(body), "invader") == 0){
            count++;
        }
    }
    return count;
}

void invader_shoot(scene_t *scene){
    int invader_count = get_num_invader(scene);
    if(invader_count > 0){
        //chooses a random invader
        int idx = rand() % (invader_count);
        int count = 0;
        for(size_t i = 0; i < scene_bodies(scene); i++){
            body_t *body = scene_get_body(scene, i);
            if(strcmp(body_get_info(body), "invader") == 0){
                //shoots bullet from nth invader
                if(count == idx){
                    body_t *player = scene_get_body(scene, 0);
                    body_t *bullet = add_bullet(body_get_centroid(body), BULLET_SIZE, false);
                    scene_add_body(scene, bullet);
                    body_set_velocity(bullet, vec_negate(PLAYER_BULLET_VEL));
                    create_destructive_collision(scene, player, bullet);
                    break;
                }
                count++;
            }
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
            case SPACE:
                player_shoot(scene);
                break;
        }
    }
}

//helper method check if the bullets or invaders is out of bounds
void out_bounds(body_t *body){
    list_t *points = body_get_shape(body);
    if(strcmp(body_get_info(body), "invader") == 0){
        //iterates over points to see if any of them are offscreen
        for (size_t i = 0; i < list_size(points); i++) {
            vector_t *cur_point = list_get(points, i);
            if(cur_point->x < INVADER_SIZE || cur_point->x > MAX_CANVAS_SIZE.x - INVADER_SIZE){
                //shifts invader and changes velocity if offscreen
                vector_t pos = body_get_centroid(body);
                if(cur_point->x < INVADER_SIZE) pos.x = 2 * INVADER_SIZE + BUFFER;
                else pos.x = MAX_CANVAS_SIZE.x - 2 * INVADER_SIZE - BUFFER;
                pos.y -= ((INVADER_SIZE + BUFFER) * NUM_ROWS);
                body_set_centroid(body, pos);
                body_set_velocity(body, vec_negate(body_get_velocity(body)));
                return;
            }
        }
    }
    else if(strcmp(body_get_info(body), "invader_bullet") == 0|| strcmp(body_get_info(body), "player_bullet") == 0) {
        for (size_t i = 0; i < list_size(points); i++) {
            vector_t *cur_point = list_get(points, i);
            if(cur_point->y < 0 || cur_point->y > MAX_CANVAS_SIZE.y){
                body_remove(body);
                return;
            }
        }
    }
}

void initialize_invaders(scene_t *scene){
    for(size_t i = 0; i<(MAX_CANVAS_SIZE.x/(INVADER_SIZE * 2 + BUFFER)) - 2; i++){
        for(size_t j = 0; j < NUM_ROWS; j++){
            vector_t center = {.x = (i + 1) * (INVADER_SIZE * 2 + BUFFER), .y = MAX_CANVAS_SIZE.y - (j + 1) * (INVADER_SIZE + BUFFER)};
            body_t *invader = add_invader(center, INVADER_SIZE);
            body_set_velocity(invader, START_VEL);
            scene_add_body(scene, invader);
        }
    }
}

int main(int argc, char *argv[]) {
    //initialization
    sdl_init(VEC_ZERO, MAX_CANVAS_SIZE);
    scene_t *scene = scene_init();
    sdl_on_key(on_key);
    scene_add_body(scene, add_player(PLAYER_START_POS, PLAYER_SIZE));
    initialize_invaders(scene);
    double dt_from_last_bullet = 0;

    //runs untill the window closes
    while (!sdl_is_done(scene)) {
        double dt = time_since_last_tick();
        dt_from_last_bullet += dt;

        //adds a new bullet to the screen once enough time has elapsed
        if (dt_from_last_bullet > BULLET_TIME) {
            invader_shoot(scene);
            dt_from_last_bullet = 0;
        }
        for(size_t i = 0; i < scene_bodies(scene); i++){
            out_bounds(scene_get_body(scene, i));
        }

        scene_tick(scene, dt);
        sdl_render_scene(scene);
    }
    scene_free(scene);
    return 0;
}