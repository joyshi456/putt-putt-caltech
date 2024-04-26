#include <math.h>
#include <SDL2/SDL_image.h>
#include <SDL2/SDL_ttf.h>
#include <SDL2/SDL_mixer.h>
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
#include "golf_course.h"

const vector_t MAX_CANVAS_SIZE = {.x=1000, .y=500};
const double PELLET_SIZE = 5;
const double HOLE_SIZE = 10;

// basic color scheme
const rgb_color_t COURSE_COLOR = {.r=0.2, .g=0.7, .b=0.2};
const rgb_color_t WALL_COLOR = {.r=0.2, .g=0.5, .b=0.2};
const rgb_color_t BALL1_COLOR = {.r=1, .g=1, .b=0.5};
const rgb_color_t BALL2_COLOR = {.r=0.5, .g=1, .b=1};
const rgb_color_t HOLE_COLOR = {.r=0, .g=0, .b=0};
const rgb_color_t PATCH_COLOR = {.r=((double) 66)/255, .g=((double) 117)/255, .b=((double) 73)/255};
const rgb_color_t SLOPE_COLOR = {.r=((double) 30)/255, .g=((double) 117)/255, .b=((double) 73)/255};
const rgb_color_t BOUNCY_COLOR = {.r=((double) 30)/255, .g=((double) 50)/255, .b=((double) 73)/255};
const rgb_color_t FORCE_COLOR = {.r=0.5, .g=0.8, .b=0.3};
const rgb_color_t COIN_COLOR = {.r=0.5, .g=1, .b=0.5};
const rgb_color_t FREEZE_PELLET_COLOR = {.r=0.25, .g=0.75, .b=0.25};
const rgb_color_t BACKGROUND_COLOR = {.r=((double) 158)/255, .g=((double) 231)/255, .b=((double) 245)/255};
const rgb_color_t LINE_COLOR = {.r=((double) 255)/255, .g=((double) 255)/255, .b=((double) 255)/255};

// alternate color scheme (beach mode)
const rgb_color_t ALT_COURSE_COLOR = {.r=((double) 20)/255, .g=((double) 66)/255, .b=((double) 140)/255};
const rgb_color_t ALT_WALL_COLOR = {.r=((double) 138)/255, .g=((double) 154)/255, .b=((double) 181)/255};
const rgb_color_t ALT_HOLE_COLOR = {.r=1, .g=1, .b=1};
const rgb_color_t ALT_PATCH_COLOR = {.r=((double) 95)/255, .g=((double) 136)/255, .b=((double) 201)/255};
const rgb_color_t ALT_SLOPE_COLOR = {.r=((double) 95)/255, .g=((double) 136)/255, .b=((double) 100)/255};
const rgb_color_t ALT_BOUNCY_COLOR = {.r=((double) 40)/255, .g=((double) 136)/255, .b=((double) 150)/255};
const rgb_color_t ALT_FORCE_COLOR = {.r=0.3, .g=0.8, .b=0.5};
const rgb_color_t ALT_COIN_COLOR = {.r=0.5, .g=0.5, .b=1};
const rgb_color_t ALT_FREEZE_PELLET_COLOR = {.r=0.25, .g=0.25, .b=0.75};
const rgb_color_t ALT_BACKGROUND_COLOR = {.r=((double) 255)/255, .g=((double) 241)/255, .b=((double) 166)/255};
const rgb_color_t ALT_LINE_COLOR = {.r=((double) 0)/255, .g=((double) 0)/255, .b=((double) 0)/255};

const int POINTS = 4;
const double MASS = 0.001;
const double BUFFER = 10;
const double NUM_ROWS = 3;
const double TRANSLATION = 20;
const int CIRCLE_POINTS = 20;
const int STAR_POINTS = 5;
const double SCALE = 1.2;
const double LAUNCH_FACTOR = 5;
const vector_t SHIFT = {.x=-20, .y=-20};
double EPSILON = 0.01;
double MAX_HOLE_VEL = 1000000;
double MAX_VEL = 2000000000;
const int MAX_NUM_WORDS = 100;
const double OUTER_INNER_RATIO = 2.5;

//a datastructure to keep track of the state of the game and house game variables
typedef struct game_state{
    double player1_points;
    double player2_points;
    bool player1_done;
    bool player2_done;
    Mix_Chunk *bounce;
    Mix_Chunk *frict;
    Mix_Chunk *hole;
    Mix_Chunk *coin;
    Mix_Chunk *freeze;
    Mix_Chunk *bouncy_ball;
    bool is_over;
    Mix_Music *current;
    Mix_Music *hole1;
    Mix_Music *hole2;
    Mix_Music *hole3;
    Mix_Music *game_over;
    SDL_Color text_color_day;
    SDL_Color text_color_beach;
    bool night_mode;
    bool freeze_player_1;
    bool freeze_player_2;
    double freeze_time_1;
    double freeze_time_2;
} game_state_t;

game_state_t *game_state_init(){
    game_state_t *game_state = malloc(sizeof(game_state_t));
    assert(game_state);
    game_state->player1_points = 0;
    game_state->player2_points = 0;
    game_state->player1_done = false;
    game_state->player2_done = false;
    game_state->bounce = NULL;
    game_state->frict = NULL;
    game_state->hole = NULL;
    game_state->coin = NULL;
    game_state->bouncy_ball = NULL;
    game_state->freeze = NULL;
    game_state->is_over = false;
    game_state->night_mode = false;
    game_state->freeze_player_1 = false;
    game_state->freeze_player_2 = false;
    game_state->freeze_time_1 = 0;
    game_state->freeze_time_2 = 0;
    return game_state;
}

//A coin is a powerup that enhances or detracts from a players score.
typedef struct coin_data{
    double amount;
    scene_t *scene;
} coin_data_t;

coin_data_t *coin_data_init(double amount, scene_t *scene){
    coin_data_t *coin_data = malloc(sizeof(coin_data_t));
    assert(coin_data);
    coin_data->amount = amount;
    coin_data->scene = scene;
    return coin_data;
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

list_t *draw_star(vector_t center, size_t size) {
    list_t *star = list_init(2*STAR_POINTS, (free_func_t) body_free_vec_list);
    //calculates the coordinates of the tip and divot of the star for
    //each point the star has
    for (size_t i = 0; i < STAR_POINTS; i++) {
        vector_t *outer_point = malloc(sizeof(vector_t));
        outer_point->x = center.x + (size * sin(2 * M_PI * i / STAR_POINTS));
        outer_point->y = center.y - (size * cos(2 * M_PI * i / STAR_POINTS));
        list_add(star, outer_point);

        vector_t *inner_point = malloc(sizeof(vector_t));
        inner_point->x = center.x + ((size / OUTER_INNER_RATIO) * sin((2 * M_PI * i / STAR_POINTS) + (2 * M_PI / (2 *STAR_POINTS))));
        inner_point->y = center.y - ((size / OUTER_INNER_RATIO) * cos((2 * M_PI * i / STAR_POINTS) + (2 * M_PI / (2 *STAR_POINTS))));
        list_add(star, inner_point);
    }
    return star;
}

//this is the freeze force for the freeze powerup
void freeze(body_t *ball, body_t *target, vector_t axis, coin_data_t *data){
    if(strcmp(body_get_info(ball), "golf_ball1") == 0){
        ((game_state_t *) scene_get_state(data->scene))->freeze_time_1 += data->amount;
        ((game_state_t *) scene_get_state(data->scene))->freeze_player_1 = true;
    }
    else if(strcmp(body_get_info(ball), "golf_ball2") == 0){
        ((game_state_t *) scene_get_state(data->scene))->freeze_time_2 += data->amount;
        ((game_state_t *) scene_get_state(data->scene))->freeze_player_2 = true;
    }
}

//The freeze pellet activates for a period of time that allows the player to 
//make as many moves as they want and not count it against their score
body_t *add_freeze_pellet(scene_t *scene, double pos_x, double pos_y, double amount){
    vector_t center = {.x=pos_x, .y=pos_y};
    list_t *circle = draw_circle(center, PELLET_SIZE);
    body_t *freeze_pellet = body_init_with_info(circle, MASS, FREEZE_PELLET_COLOR, "freeze_pellet", free);
    scene_add_body(scene, freeze_pellet);
    body_set_color2(freeze_pellet, ALT_FREEZE_PELLET_COLOR);
    coin_data_t *data = coin_data_init(amount, scene);
    for(size_t i = 0; i < scene_bodies(scene); i++){
        body_t *player = scene_get_body(scene, i);
        if(strcmp(body_get_info(player), "golf_ball1") == 0 || strcmp(body_get_info(player), "golf_ball2") == 0){
            create_collision(scene, player, freeze_pellet, (collision_handler_t) freeze, data, NULL);
            create_destructive_collision(scene, freeze_pellet, player);
        }
    }
    return freeze_pellet;
}

//is the force that adds to or subtracts from the player's score when they collide with a coin
void change_count(body_t *ball, body_t *target, vector_t axis, coin_data_t *data){
    if(strcmp(body_get_info(ball), "golf_ball1") == 0){
        ((game_state_t *) scene_get_state(data->scene))->player1_points += data->amount;
    }
    else if(strcmp(body_get_info(ball), "golf_ball2") == 0){
        ((game_state_t *) scene_get_state(data->scene))->player2_points += data->amount;
    }
}

//adds a coin to a screen, change_count is called when the player collides with the coin
body_t *add_coin(scene_t *scene, double pos_x, double pos_y, double amount){
    vector_t center = {.x=pos_x, .y=pos_y};
    list_t *circle = draw_circle(center, PELLET_SIZE);
    body_t *coin = body_init_with_info(circle, MASS, COIN_COLOR, "coin", free);
    scene_add_body(scene, coin);
    body_set_color2(coin, ALT_COIN_COLOR);
    coin_data_t *data = coin_data_init(amount, scene);
    for(size_t i = 0; i < scene_bodies(scene); i++){
        body_t *player = scene_get_body(scene, i);
        if(strcmp(body_get_info(player), "golf_ball1") == 0 || strcmp(body_get_info(player), "golf_ball2") == 0){
            create_collision(scene, player, coin, (collision_handler_t) change_count, data, NULL);
            create_destructive_collision(scene, coin, player);
        }
    }
    return coin;
}

//Switching between different color schemes or modes
void swap_colors(void *scene){
    for (size_t i = 0; i < scene_bodies(scene); i++) {
        body_swap_color(scene_get_body(scene, i));
    }
}

void on_key(char key, key_event_type_t type, double held_time, void *scene) {
    if (type == KEY_PRESSED) {
        switch(key) {
            case RIGHT_ARROW: 
                //changes color mode
                ((game_state_t *) scene_get_state(scene))->night_mode = !((game_state_t *) scene_get_state(scene))->night_mode;
                swap_colors(scene);
                break;
            case LEFT_ARROW:
                //pauses and plays the music
                if(!Mix_PausedMusic()){
                    Mix_PauseMusic();
                }
                else{
                    Mix_ResumeMusic();
                }
                break;
        }
    }
}

//adds an impulse to the ball when it is hit
void hit_ball(vector_t imp, scene_t *scene, char *type) {
    if(vec_dot(imp,imp) > MAX_VEL){
        imp = vec_multiply((MAX_VEL/vec_dot(imp, imp)), imp);
    }
    for(size_t i = 0; i < scene_bodies(scene); i++){
        body_t *body = scene_get_body(scene, i);
        if(strcmp(body_get_info(body), type) == 0){
            vector_t J_n = vec_multiply(LAUNCH_FACTOR * body_get_mass(body), imp);
            J_n.x = -J_n.x;
            body_add_impulse(body, J_n);
            return;
        }
    }
}

//when a click happens, figures out the player whose turn it is and hits the ball
//only hits ball if both balls aren't moving
void on_click(vector_t start_pos, vector_t end_pos, void *scene){
    vector_t diff = vec_subtract(end_pos, start_pos);
    body_t *ball1;
    body_t *ball2;
    body_t *hole;
    for(size_t i = 0; i < scene_bodies(scene); i++){
        body_t *body = scene_get_body(scene, i);
        if(strcmp(body_get_info(body), "golf_ball1") == 0){
            ball1 = body;
        }
        if(strcmp(body_get_info(body), "golf_ball2") == 0){
            ball2 = body;
        }
        if(strcmp(body_get_info(body), "hole") == 0){
            hole = body;
        }
    }
    if(vec_dot(body_get_velocity(ball1), body_get_velocity(ball1)) < EPSILON && vec_dot(body_get_velocity(ball2), body_get_velocity(ball2)) < EPSILON){
        if(vec_dist(body_get_centroid(ball1), body_get_centroid(hole)) > vec_dist(body_get_centroid(ball2), body_get_centroid(hole)) && (!((game_state_t *) scene_get_state(scene))->player1_done || !((game_state_t *) scene_get_state(scene))->player2_done)){
            hit_ball(diff, scene, "golf_ball1");
            if(!((game_state_t *) scene_get_state(scene))->freeze_player_1){
                ((game_state_t *) scene_get_state(scene))->player1_points += 1;
            }
        }
        else if(!((game_state_t *) scene_get_state(scene))->player2_done || !((game_state_t *) scene_get_state(scene))->player1_done){
            hit_ball(diff, scene, "golf_ball2");
            if(!((game_state_t *) scene_get_state(scene))->freeze_player_2){
                ((game_state_t *) scene_get_state(scene))->player2_points += 1;
            }
        }
    }
}

//allows player to scroll around screen using mouse pad
void on_scroll(int x, int y, void *scene){
    vector_t pos = {.x = -x, .y = y};
    vector_t diff = vec_multiply(5, pos);
    for(size_t i = 0; i < scene_bodies(scene); i++){
        body_t *body = scene_get_body(scene, i);
        if(strcmp(body_get_info(body), "background") != 0){
            body_set_centroid(body, vec_add(body_get_centroid(body), diff));
        }
    }
}

//gives a rectangle body for the path dependent on two points
body_t *add_path(vector_t point1, vector_t point2, char *type, rgb_color_t color, double thickness){
    vector_t center = vec_multiply(0.5, vec_add(point1, point2));
    vector_t p1_sub = vec_subtract(point1, center);
    double EPS = 2;
    point1 = vec_add(point1, vec_multiply(EPS / sqrt(vec_dot(p1_sub, p1_sub)), p1_sub));
    vector_t p2_sub = vec_subtract(point2, center);
    point2 = vec_add(point2, vec_multiply(EPS / sqrt(vec_dot(p2_sub, p2_sub)), p2_sub));
    list_t *wall_points = list_init(10, (free_func_t) body_free_vec_list);
    vector_t diff = vec_subtract(point1, point2);
    vector_t unit_diff = vec_multiply(1/sqrt(vec_dot(diff, diff)), diff);
    vector_t shift = vec_multiply(thickness/2, vec_rotate(unit_diff, M_PI/2));
    vector_t *p1 = malloc(sizeof(vector_t));
    p1->x = vec_add(point1, shift).x;
    p1->y = vec_add(point1, shift).y;
    list_add(wall_points, p1);
    vector_t *p2 = malloc(sizeof(vector_t));
    p2->x = vec_add(point1, vec_negate(shift)).x;
    p2->y = vec_add(point1, vec_negate(shift)).y;
    list_add(wall_points, p2);
    vector_t *p3 = malloc(sizeof(vector_t));
    p3->x = vec_add(point2, vec_negate(shift)).x;
    p3->y = vec_add(point2, vec_negate(shift)).y;
    list_add(wall_points, p3);
    vector_t *p4 = malloc(sizeof(vector_t));
    p4->x = vec_add(point2, shift).x;
    p4->y = vec_add(point2, shift).y;
    list_add(wall_points, p4);
    body_t *wall = body_init_with_info(wall_points, INFINITY, color, type, free);
    return wall;
}

//returns the player whose turn it is
char *get_turn(void *scene){
    body_t *ball1;
    body_t *ball2;
    body_t *hole;
    for(size_t i = 0; i < scene_bodies(scene); i++){
        body_t *body = scene_get_body(scene, i);
        if(strcmp(body_get_info(body), "golf_ball1") == 0){
            ball1 = body;
        }
        if(strcmp(body_get_info(body), "golf_ball2") == 0){
            ball2 = body;
        }
        if(strcmp(body_get_info(body), "hole") == 0){
            hole = body;
        }
    }
    if(vec_dot(body_get_velocity(ball1), body_get_velocity(ball1)) > EPSILON){
        return "player 1";
    }
    else if(vec_dot(body_get_velocity(ball2), body_get_velocity(ball2)) > EPSILON){
        return "player 2";
    }
    else if(vec_dist(body_get_centroid(ball1), body_get_centroid(hole)) > vec_dist(body_get_centroid(ball2), body_get_centroid(hole))){
        return "player 1";
    }
    else{
        return "player 2";
    }
}

//Shows the hypothetical path of the ball when a player clicks and creates a stroke
void display_hit_path(vector_t start_pos, vector_t end_pos, void *scene){
    char* player = "golf_ball1";
    if(strcmp(get_turn(scene), "player 2") == 0){
        player = "golf_ball2";
    }
    body_t *ball = NULL;
    for(size_t i = 0; i < scene_bodies(scene); i++){
        body_t *body = scene_get_body(scene, i);
            if(strcmp(body_get_info(body), player) == 0){
                ball = body;
                start_pos = body_get_centroid(body);
                break;
            }
    }
    //only displays the path if the balls have no speed
    if(ball != NULL && vec_dot(body_get_velocity(ball), body_get_velocity(ball)) < EPSILON && !((game_state_t *) scene_get_state(scene))->is_over){
        vector_t sub = vec_subtract(end_pos, start_pos);
        sub.y = -sub.y;
        vector_t diff = vec_subtract(start_pos, sub);
        body_t *hit_path = add_path(start_pos, diff, "launch_line", LINE_COLOR, 2);
        body_set_color2(hit_path, ALT_LINE_COLOR);
        scene_add_body(scene, hit_path);
    }
}

//removes the hit path in conjunction with SDL when mouse button is released
void on_mouse_button_up(vector_t start_pos, vector_t end_pos, void *scene){
    for(size_t i = 0; i < scene_bodies(scene); i++){
        body_t *body = scene_get_body(scene, i);
            if(strcmp(body_get_info(body), "launch_line") == 0){
                body_remove(body);
            }
    }
}

//removes hit path and allows the hit path to update
void delete_hit_path(scene_t *scene){
    int count = 0;
    for(size_t i = 0; i < scene_bodies(scene); i++){
        body_t *body = scene_get_body(scene, i);
        if(strcmp(body_get_info(body), "launch_line") == 0){
            count++;
        }
    }
    if(count > 1){
        for(size_t i = 0; i < scene_bodies(scene); i++){
            body_t *body = scene_get_body(scene, i);
            if(strcmp(body_get_info(body), "launch_line") == 0){
                body_remove(body);
                return;
            }
        }
    }
}

void automatic_scroll(void *scene){
    body_t *ball1;
    body_t *ball2;
    body_t *hole;
    for(size_t i = 0; i < scene_bodies(scene); i++){
        body_t *body = scene_get_body(scene, i);
        if(strcmp(body_get_info(body), "golf_ball1") == 0){
            ball1 = body;
        }
        if(strcmp(body_get_info(body), "golf_ball2") == 0){
            ball2 = body;
        }
        if(strcmp(body_get_info(body), "hole") == 0){
            hole = body;
        }
    }
    vector_t diff;
    if(strcmp(get_turn(scene), "player 2")){
        diff = vec_subtract(vec_multiply(0.5, MAX_CANVAS_SIZE), body_get_centroid(ball1));
    }
    else{
        diff = vec_subtract(vec_multiply(0.5, MAX_CANVAS_SIZE), body_get_centroid(ball2));
    }
    for(size_t i = 0; i < scene_bodies(scene); i++){
        body_t *body = scene_get_body(scene, i);
        if(strcmp(body_get_info(body), "background") != 0){
            body_set_centroid(body, vec_add(body_get_centroid(body), diff));
        }
    }
}

//plays the corresponding sound dependednt on what the ball hits
void play_sound(body_t *ball, body_t *target, vector_t axis, scene_t *scene){
    if(strcmp(body_get_info(target), "wall_normal") == 0){
        Mix_PlayChannel(-1, ((game_state_t *)scene_get_state(scene))->bounce, 0);
    }
    else if(strcmp(body_get_info(target), "patch") == 0){
        Mix_PlayChannel(-1, ((game_state_t *)scene_get_state(scene))->frict, 0);
    }
    else if(strcmp(body_get_info(target), "hole") == 0){
        Mix_PlayChannel(-1, ((game_state_t *)scene_get_state(scene))->hole, 0);
    }
    else if(strcmp(body_get_info(target), "coin") == 0){
        Mix_PlayChannel(-1, ((game_state_t *)scene_get_state(scene))->coin, 0);
    }
    else if(strcmp(body_get_info(target), "freeze_pellet") == 0){
        Mix_PlayChannel(-1, ((game_state_t *)scene_get_state(scene))->freeze, 0);
    }
    else if(strcmp(body_get_info(target), "bouncy_ball") == 0){
        Mix_PlayChannel(-1, ((game_state_t *)scene_get_state(scene))->bouncy_ball, 0);
    }
}

//sorts the scores in ascending order since smaller scores are better
void double_sort(list_t *doubles) {
    for(size_t i = 0; i < list_size(doubles); i++){
        size_t idx = i;
        for(size_t j = i + 1; j < list_size(doubles); j++){
            if(*((double *)list_get(doubles, idx)) > *((double *)list_get(doubles, j))){
                idx = j;
            }
        }
        double *temp = list_get(doubles, i);
        list_set(doubles, i, list_get(doubles, idx));
        list_set(doubles, idx, temp);
    }
}

//gets the leaderboard scores from the text file
list_t *get_scores(){
    FILE *file = fopen("static/data/leaderboard.txt", "r");
    list_t *scores = list_init(POINTS, (free_func_t)body_free_vec_list);
    char *result = malloc(sizeof(char));
    char *word = malloc((sizeof(char)) * (MAX_NUM_WORDS + 1));
    int idx = 0;
    char *temp;
    //reads each character from the file one at a time
    while (fread(result, sizeof(char), 1, file)) {
        word[idx] = result[0];
        idx++;
        //new line character signifies end of word
        if (result[0] == '\n') {
            word[idx-1] = '\0';
            double *result = malloc(sizeof(double));
            *result = strtod(word, &temp);
            list_add(scores, result);
            idx = 0;
        }
    }
    free(result);
    free(word);
    fclose(file);
    double_sort(scores);
    return scores;
}

//records the player scores to the leaderboard file once both balls go in the last hole
void record_scores(scene_t *scene){
    double score1 = ((game_state_t *) scene_get_state(scene))->player1_points;
    double score2 = ((game_state_t *) scene_get_state(scene))->player2_points;
    FILE *file = fopen("static/data/leaderboard.txt", "a");
    fprintf(file, "%f\n", score1);
    fprintf(file, "%f\n", score2);
    fclose(file);
}

void ball_in_hole3(body_t *ball, body_t *target, vector_t axis, scene_t *scene){
    //ball only "goes in the hole" if its velocity is below the max velocity
    if(vec_dot(body_get_velocity(ball), body_get_velocity(ball)) < MAX_HOLE_VEL){
        body_set_velocity(ball, VEC_ZERO);
        if(strcmp(body_get_info(ball), "golf_ball2") == 0){
            ((game_state_t *) scene_get_state(scene))->player2_done = true;
        }
        else{
            ((game_state_t *) scene_get_state(scene))->player1_done = true;
        }
        body_hide(ball);
    }
    if(((game_state_t *) scene_get_state(scene))->player2_done && ((game_state_t *) scene_get_state(scene))->player1_done){
        list_t *background_points = list_init(POINTS, (free_func_t) body_free_vec_list);
        vector_t *bp1 = malloc(sizeof(vector_t));
        bp1->x = 0;
        bp1->y = 0;
        list_add(background_points, bp1);
        vector_t *bp2 = malloc(sizeof(vector_t));
        bp2->x = 0;
        bp2->y = MAX_CANVAS_SIZE.y;
        list_add(background_points, bp2);
        vector_t *bp3 = malloc(sizeof(vector_t));
        bp3->x = MAX_CANVAS_SIZE.x;
        bp3->y = MAX_CANVAS_SIZE.y;
        list_add(background_points, bp3);
        vector_t *bp4 = malloc(sizeof(vector_t));
        bp4->x = MAX_CANVAS_SIZE.x;
        bp4->y = 0;
        list_add(background_points, bp4);
        body_t *background;
        if(!((game_state_t *) scene_get_state(scene))->night_mode){
            background = body_init_with_info(background_points, INFINITY, BACKGROUND_COLOR, "background", free);
            body_set_color2(background, ALT_BACKGROUND_COLOR);
        }
        else{
            background = body_init_with_info(background_points, INFINITY, ALT_BACKGROUND_COLOR, "background", free);
            body_set_color2(background, BACKGROUND_COLOR);
        }
        scene_add_body(scene, background);
        record_scores(scene);
        ((game_state_t *) scene_get_state(scene))->is_over = true;
    }
}

void init_course_3(scene_t *scene){
    Mix_PlayMusic(((game_state_t *)scene_get_state(scene))->hole3, -1);
    ((game_state_t *) scene_get_state(scene))->night_mode = false;
    list_t *background_points = list_init(POINTS, (free_func_t) body_free_vec_list);
    vector_t *bp1 = malloc(sizeof(vector_t));
    bp1->x = 0;
    bp1->y = 0;
    list_add(background_points, bp1);
    vector_t *bp2 = malloc(sizeof(vector_t));
    bp2->x = 0;
    bp2->y = MAX_CANVAS_SIZE.y;
    list_add(background_points, bp2);
    vector_t *bp3 = malloc(sizeof(vector_t));
    bp3->x = MAX_CANVAS_SIZE.x;
    bp3->y = MAX_CANVAS_SIZE.y;
    list_add(background_points, bp3);
    vector_t *bp4 = malloc(sizeof(vector_t));
    bp4->x = MAX_CANVAS_SIZE.x;
    bp4->y = 0;
    list_add(background_points, bp4);
    body_t *background = body_init_with_info(background_points, INFINITY, BACKGROUND_COLOR, "background", free);
    scene_add_body(scene, background);
    body_set_color2(background, ALT_BACKGROUND_COLOR);

    list_t *course_points = list_init(POINTS, (free_func_t) body_free_vec_list);
    vector_t *p1 = malloc(sizeof(vector_t));
    p1->x = 1 * SCALE;
    p1->y = MAX_CANVAS_SIZE.y * 3/13 * SCALE;
    list_add(course_points, p1);
    vector_t *p2 = malloc(sizeof(vector_t));
    p2->x = MAX_CANVAS_SIZE.x * 3/27 * SCALE;
    p2->y = MAX_CANVAS_SIZE.y * 3/13 * SCALE;
    list_add(course_points, p2);
    vector_t *p3 = malloc(sizeof(vector_t));
    p3->x = MAX_CANVAS_SIZE.x * 3/27 * SCALE;
    p3->y = MAX_CANVAS_SIZE.y * 5/13 * SCALE;
    list_add(course_points, p3);
    vector_t *p4 = malloc(sizeof(vector_t));
    p4->x = MAX_CANVAS_SIZE.x * 2/27 * SCALE;
    p4->y = MAX_CANVAS_SIZE.y * 5/13 * SCALE;
    list_add(course_points, p4);
    vector_t *p5 = malloc(sizeof(vector_t));
    p5->x = MAX_CANVAS_SIZE.x * 2/27 * SCALE;
    p5->y = MAX_CANVAS_SIZE.y * 8/13 * SCALE;
    list_add(course_points, p5);
    vector_t *p6 = malloc(sizeof(vector_t));
    p6->x = MAX_CANVAS_SIZE.x * 5/27 * SCALE;
    p6->y = MAX_CANVAS_SIZE.y * 8/13 * SCALE;
    list_add(course_points, p6);
    vector_t *p7 = malloc(sizeof(vector_t));
    p7->x = MAX_CANVAS_SIZE.x * 5/27 * SCALE;
    p7->y = MAX_CANVAS_SIZE.y * 10/13 * SCALE;
    list_add(course_points, p7);
    vector_t *p8 = malloc(sizeof(vector_t));
    p8->x = MAX_CANVAS_SIZE.x * 7/27 * SCALE;
    p8->y = MAX_CANVAS_SIZE.y * 10/13 * SCALE;
    list_add(course_points, p8);
    vector_t *p9 = malloc(sizeof(vector_t));
    p9->x = MAX_CANVAS_SIZE.x * 7/27 * SCALE;
    p9->y = MAX_CANVAS_SIZE.y * 7/13 * SCALE;
    list_add(course_points, p9);
    vector_t *p10 = malloc(sizeof(vector_t));
    p10->x = MAX_CANVAS_SIZE.x * 5/27 * SCALE;
    p10->y = MAX_CANVAS_SIZE.y * 7/13 * SCALE;
    list_add(course_points, p10);
    vector_t *p11 = malloc(sizeof(vector_t));
    p11->x = MAX_CANVAS_SIZE.x * 5/27 * SCALE;
    p11->y = 1;
    list_add(course_points, p11);
    vector_t *p12 = malloc(sizeof(vector_t));
    p12->x = MAX_CANVAS_SIZE.x * 16/27 * SCALE;
    p12->y = 1;
    list_add(course_points, p12);
    vector_t *p13 = malloc(sizeof(vector_t));
    p13->x = MAX_CANVAS_SIZE.x * 16/27 * SCALE;
    p13->y = MAX_CANVAS_SIZE.y * 5/13 * SCALE;
    list_add(course_points, p13);
    vector_t *p14 = malloc(sizeof(vector_t));
    p14->x = MAX_CANVAS_SIZE.x * 21/27 * SCALE;
    p14->y = MAX_CANVAS_SIZE.y * 5/13 * SCALE;
    list_add(course_points, p14);
    vector_t *p15 = malloc(sizeof(vector_t));
    p15->x = MAX_CANVAS_SIZE.x * 21/27 * SCALE;
    p15->y = MAX_CANVAS_SIZE.y * 7/13 * SCALE;
    list_add(course_points, p15);
    vector_t *p16 = malloc(sizeof(vector_t));
    p16->x = MAX_CANVAS_SIZE.x * SCALE;
    p16->y = MAX_CANVAS_SIZE.y * 7/13 * SCALE;
    list_add(course_points, p16);
    vector_t *p17 = malloc(sizeof(vector_t));
    p17->x = MAX_CANVAS_SIZE.x * SCALE;
    p17->y = MAX_CANVAS_SIZE.y * 9/13 * SCALE;
    list_add(course_points, p17);
    vector_t *p18 = malloc(sizeof(vector_t));
    p18->x = MAX_CANVAS_SIZE.x * 25/27 * SCALE;
    p18->y = MAX_CANVAS_SIZE.y * 9/13 * SCALE;
    list_add(course_points, p18);
    vector_t *p19 = malloc(sizeof(vector_t));
    p19->x = MAX_CANVAS_SIZE.x * 25/27 * SCALE;
    p19->y = MAX_CANVAS_SIZE.y * 10/13 * SCALE;
    list_add(course_points, p19);
    vector_t *p20 = malloc(sizeof(vector_t));
    p20->x = MAX_CANVAS_SIZE.x * 26/27 * SCALE;
    p20->y = MAX_CANVAS_SIZE.y * 10/13 * SCALE;
    list_add(course_points, p20);
    vector_t *p21 = malloc(sizeof(vector_t));
    p21->x = MAX_CANVAS_SIZE.x * 26/27 * SCALE;
    p21->y = MAX_CANVAS_SIZE.y * 11/13 * SCALE;
    list_add(course_points, p21);
    vector_t *p22 = malloc(sizeof(vector_t));
    p22->x = MAX_CANVAS_SIZE.x * 25/27 * SCALE;
    p22->y = MAX_CANVAS_SIZE.y * 11/13 * SCALE;
    list_add(course_points, p22);
    vector_t *p23 = malloc(sizeof(vector_t));
    p23->x = MAX_CANVAS_SIZE.x * 25/27 * SCALE;
    p23->y = MAX_CANVAS_SIZE.y * 12/13 * SCALE;
    list_add(course_points, p23);
    vector_t *p24 = malloc(sizeof(vector_t));
    p24->x = MAX_CANVAS_SIZE.x * 26/27 * SCALE;
    p24->y = MAX_CANVAS_SIZE.y * 12/13 * SCALE;
    list_add(course_points, p24);
    vector_t *p25 = malloc(sizeof(vector_t));
    p25->x = MAX_CANVAS_SIZE.x * 26/27 * SCALE;
    p25->y = MAX_CANVAS_SIZE.y * SCALE;
    list_add(course_points, p25);
    vector_t *p26 = malloc(sizeof(vector_t));
    p26->x = MAX_CANVAS_SIZE.x * 24/27 * SCALE;
    p26->y = MAX_CANVAS_SIZE.y * SCALE;
    list_add(course_points, p26);
    vector_t *p27 = malloc(sizeof(vector_t));
    p27->x = MAX_CANVAS_SIZE.x * 24/27 * SCALE;
    p27->y = MAX_CANVAS_SIZE.y * 9/13 * SCALE;
    list_add(course_points, p27);
    vector_t *p28 = malloc(sizeof(vector_t));
    p28->x = MAX_CANVAS_SIZE.x * 22/27 * SCALE;
    p28->y = MAX_CANVAS_SIZE.y * 9/13 * SCALE;
    list_add(course_points, p28);
    vector_t *p29 = malloc(sizeof(vector_t));
    p29->x = MAX_CANVAS_SIZE.x * 22/27 * SCALE;
    p29->y = MAX_CANVAS_SIZE.y * 10/13 * SCALE;
    list_add(course_points, p29);
    vector_t *p30 = malloc(sizeof(vector_t));
    p30->x = MAX_CANVAS_SIZE.x * 23/27 * SCALE;
    p30->y = MAX_CANVAS_SIZE.y * 10/13 * SCALE;
    list_add(course_points, p30);
    vector_t *p31 = malloc(sizeof(vector_t));
    p31->x = MAX_CANVAS_SIZE.x * 23/27 * SCALE;
    p31->y = MAX_CANVAS_SIZE.y * 11/13 * SCALE;
    list_add(course_points, p31);
    vector_t *p32 = malloc(sizeof(vector_t));
    p32->x = MAX_CANVAS_SIZE.x * 22/27 * SCALE;
    p32->y = MAX_CANVAS_SIZE.y * 11/13 * SCALE;
    list_add(course_points, p32);
    vector_t *p33 = malloc(sizeof(vector_t));
    p33->x = MAX_CANVAS_SIZE.x * 22/27 * SCALE;
    p33->y = MAX_CANVAS_SIZE.y * 12/13 * SCALE;
    list_add(course_points, p33);
    vector_t *p34 = malloc(sizeof(vector_t));
    p34->x = MAX_CANVAS_SIZE.x * 23/27 * SCALE;
    p34->y = MAX_CANVAS_SIZE.y * 12/13 * SCALE;
    list_add(course_points, p34);
    vector_t *p35 = malloc(sizeof(vector_t));
    p35->x = MAX_CANVAS_SIZE.x * 23/27 * SCALE;
    p35->y = MAX_CANVAS_SIZE.y * SCALE;
    list_add(course_points, p35);
    vector_t *p36 = malloc(sizeof(vector_t));
    p36->x = MAX_CANVAS_SIZE.x * 21/27 * SCALE;
    p36->y = MAX_CANVAS_SIZE.y * 1 * SCALE;
    list_add(course_points, p36);
    vector_t *p37 = malloc(sizeof(vector_t));
    p37->x = MAX_CANVAS_SIZE.x * 21/27 * SCALE;
    p37->y = MAX_CANVAS_SIZE.y * 9/13 * SCALE;
    list_add(course_points, p37);
    vector_t *p38 = malloc(sizeof(vector_t));
    p38->x = MAX_CANVAS_SIZE.x * 18/27 * SCALE;
    p38->y = MAX_CANVAS_SIZE.y * 9/13 * SCALE;
    list_add(course_points, p38);
    vector_t *p39 = malloc(sizeof(vector_t));
    p39->x = MAX_CANVAS_SIZE.x * 18/27 * SCALE;
    p39->y = MAX_CANVAS_SIZE.y * 7/13 * SCALE;
    list_add(course_points, p39);
    vector_t *p40 = malloc(sizeof(vector_t));
    p40->x = MAX_CANVAS_SIZE.x * 14/27 * SCALE;
    p40->y = MAX_CANVAS_SIZE.y * 7/13 * SCALE;
    list_add(course_points, p40);
    vector_t *p41 = malloc(sizeof(vector_t));
    p41->x = MAX_CANVAS_SIZE.x * 14/27 * SCALE;
    p41->y = MAX_CANVAS_SIZE.y * 5/13 * SCALE;
    list_add(course_points, p41);
    vector_t *p42 = malloc(sizeof(vector_t));
    p42->x = MAX_CANVAS_SIZE.x * 7/27 * SCALE;
    p42->y = MAX_CANVAS_SIZE.y * 5/13 * SCALE;
    list_add(course_points, p42);
    vector_t *p43 = malloc(sizeof(vector_t));
    p43->x = MAX_CANVAS_SIZE.x * 7/27 * SCALE;
    p43->y = MAX_CANVAS_SIZE.y * 6/13 * SCALE;
    list_add(course_points, p43);
    vector_t *p44 = malloc(sizeof(vector_t));
    p44->x = MAX_CANVAS_SIZE.x * 8/27 * SCALE;
    p44->y = MAX_CANVAS_SIZE.y * 6/13 * SCALE;
    list_add(course_points, p44);
    vector_t *p45 = malloc(sizeof(vector_t));
    p45->x = MAX_CANVAS_SIZE.x * 8/27 * SCALE;
    p45->y = MAX_CANVAS_SIZE.y * 11/13 * SCALE;
    list_add(course_points, p45);
    vector_t *p46 = malloc(sizeof(vector_t));
    p46->x = MAX_CANVAS_SIZE.x * 4/27 * SCALE;
    p46->y = MAX_CANVAS_SIZE.y * 11/13 * SCALE;
    list_add(course_points, p46);
    vector_t *p47 = malloc(sizeof(vector_t));
    p47->x = MAX_CANVAS_SIZE.x * 4/27 * SCALE;
    p47->y = MAX_CANVAS_SIZE.y * 9/13 * SCALE;
    list_add(course_points, p47);
    vector_t *p48 = malloc(sizeof(vector_t));
    p48->x = MAX_CANVAS_SIZE.x * 1/27 * SCALE;
    p48->y = MAX_CANVAS_SIZE.y * 9/13 * SCALE;
    list_add(course_points, p48);
    vector_t *p49 = malloc(sizeof(vector_t));
    p49->x = MAX_CANVAS_SIZE.x * 1/27 * SCALE;
    p49->y = MAX_CANVAS_SIZE.y * 5/13 * SCALE;
    list_add(course_points, p49);
    vector_t *p50 = malloc(sizeof(vector_t));
    p50->x = 1;
    p50->y = MAX_CANVAS_SIZE.y * 5/13 * SCALE;
    list_add(course_points, p50);

    body_t *course = body_init_with_info(course_points, INFINITY, COURSE_COLOR, "course", free);
    scene_add_body(scene, course);
    body_set_color2(course, ALT_COURSE_COLOR);

    list_t *patch1_points = list_init(POINTS, (free_func_t) body_free_vec_list);
    vector_t *patch11 = malloc(sizeof(vector_t));
    patch11->x = MAX_CANVAS_SIZE.x * 6/27 * SCALE;
    patch11->y = MAX_CANVAS_SIZE.y * 1/13 * SCALE;
    list_add(patch1_points, patch11);
    vector_t *patch12 = malloc(sizeof(vector_t));
    patch12->x = MAX_CANVAS_SIZE.x * 6/27 * SCALE;
    patch12->y = MAX_CANVAS_SIZE.y * 2/13 * SCALE;
    list_add(patch1_points, patch12);
    vector_t *patch13 = malloc(sizeof(vector_t));
    patch13->x = MAX_CANVAS_SIZE.x * 8/27 * SCALE;
    patch13->y = MAX_CANVAS_SIZE.y * 2/13 * SCALE;
    list_add(patch1_points, patch13);
    vector_t *patch14 = malloc(sizeof(vector_t));
    patch14->x = MAX_CANVAS_SIZE.x * 8/27 * SCALE;
    patch14->y = MAX_CANVAS_SIZE.y * 1/13 * SCALE;
    list_add(patch1_points, patch14);
    body_t *patch1 = body_init_with_info(patch1_points, INFINITY, PATCH_COLOR, "patch", free);
    scene_add_body(scene, patch1);
    body_set_color2(patch1, ALT_PATCH_COLOR);

    list_t *patch2_points = list_init(POINTS, (free_func_t) body_free_vec_list);
    vector_t *patch21 = malloc(sizeof(vector_t));
    patch21->x = MAX_CANVAS_SIZE.x * 12/27 * SCALE;
    patch21->y = MAX_CANVAS_SIZE.y * 3/13 * SCALE;
    list_add(patch2_points, patch21);
    vector_t *patch22 = malloc(sizeof(vector_t));
    patch22->x = MAX_CANVAS_SIZE.x * 12/27 * SCALE;
    patch22->y = MAX_CANVAS_SIZE.y * 4/13 * SCALE;
    list_add(patch2_points, patch22);
    vector_t *patch23 = malloc(sizeof(vector_t));
    patch23->x = MAX_CANVAS_SIZE.x * 13/27 * SCALE;
    patch23->y = MAX_CANVAS_SIZE.y * 4/13 * SCALE;
    list_add(patch2_points, patch23);
    vector_t *patch24 = malloc(sizeof(vector_t));
    patch24->x = MAX_CANVAS_SIZE.x * 13/27 * SCALE;
    patch24->y = MAX_CANVAS_SIZE.y * 3/13 * SCALE;
    list_add(patch2_points, patch24);
    body_t *patch2 = body_init_with_info(patch2_points, INFINITY, PATCH_COLOR, "patch", free);
    scene_add_body(scene, patch2);
    body_set_color2(patch2, ALT_PATCH_COLOR);

    list_t *patch3_points = list_init(POINTS, (free_func_t) body_free_vec_list);
    vector_t *patch31 = malloc(sizeof(vector_t));
    patch31->x = MAX_CANVAS_SIZE.x * 19/27 * SCALE;
    patch31->y = MAX_CANVAS_SIZE.y * 5/13 * SCALE;
    list_add(patch3_points, patch31);
    vector_t *patch32 = malloc(sizeof(vector_t));
    patch32->x = MAX_CANVAS_SIZE.x * 19/27 * SCALE;
    patch32->y = MAX_CANVAS_SIZE.y * 7/13 * SCALE;
    list_add(patch3_points, patch32);
    vector_t *patch33 = malloc(sizeof(vector_t));
    patch33->x = MAX_CANVAS_SIZE.x * 21/27 * SCALE;
    patch33->y = MAX_CANVAS_SIZE.y * 7/13 * SCALE;
    list_add(patch3_points, patch33);
    vector_t *patch34 = malloc(sizeof(vector_t));
    patch34->x = MAX_CANVAS_SIZE.x * 21/27 * SCALE;
    patch34->y = MAX_CANVAS_SIZE.y * 5/13 * SCALE;
    list_add(patch3_points, patch34);
    body_t *patch3 = body_init_with_info(patch3_points, INFINITY, PATCH_COLOR, "patch", free);
    scene_add_body(scene, patch3);
    body_set_color2(patch3, ALT_PATCH_COLOR);

    double hole_pos_x = MAX_CANVAS_SIZE.x * 25.5/27 * SCALE;
    double hole_pos_y = MAX_CANVAS_SIZE.y * 10.5/13 * SCALE;

    vector_t center = {.x=hole_pos_x, .y=hole_pos_y};
    list_t *circle = draw_circle(center, HOLE_SIZE * SCALE);
    body_t *hole = body_init_with_info(circle, INFINITY, HOLE_COLOR, "hole", free);
    scene_add_body(scene, hole);
    body_set_color2(hole, ALT_HOLE_COLOR);

    list_t *circle_big = draw_circle(center, HOLE_SIZE * 2 * SCALE);
    body_t *hole_real = body_init_with_info(circle_big, INFINITY, HOLE_COLOR, "hole_real", free);
    scene_add_body(scene, hole_real);
    body_set_color2(hole_real, ALT_HOLE_COLOR);

    vector_t start_pos_ball1 = {.x=MAX_CANVAS_SIZE.x * 1/27 * SCALE, .y=MAX_CANVAS_SIZE.y * 3.5/13 * SCALE};
    list_t *circle1 = draw_circle(start_pos_ball1, HOLE_SIZE * SCALE);
    body_t *ball1 = body_init_with_info(circle1, MASS, BALL1_COLOR, "golf_ball1", free);
    create_collision(scene, ball1, hole, (collision_handler_t) ball_in_hole3, scene, NULL);

    vector_t start_pos_ball2 = {.x=MAX_CANVAS_SIZE.x * 2/27 * SCALE, .y=MAX_CANVAS_SIZE.y * 3.5/13 * SCALE};
    list_t *circle2 = draw_circle(start_pos_ball2, HOLE_SIZE * SCALE);
    body_t *ball2 = body_init_with_info(circle2, MASS, BALL2_COLOR, "golf_ball2", free);
    create_collision(scene, ball2, hole, (collision_handler_t) ball_in_hole3, scene, NULL);
    
    body_t *coin1 = add_coin(scene, MAX_CANVAS_SIZE.x * 22.5/27 * SCALE, MAX_CANVAS_SIZE.y * 10.5/13 * SCALE, -0.4);
    create_collision(scene, ball1, coin1, (collision_handler_t) play_sound, scene, NULL);
    create_collision(scene, ball2, coin1, (collision_handler_t) play_sound, scene, NULL);

    body_t *coin2 = add_coin(scene, MAX_CANVAS_SIZE.x * 22.5/27 * SCALE, MAX_CANVAS_SIZE.y * 12.5/13 * SCALE, 0.3);
    create_collision(scene, ball1, coin2, (collision_handler_t) play_sound, scene, NULL);
    create_collision(scene, ball2, coin2, (collision_handler_t) play_sound, scene, NULL);

    body_t *freeze_pellet_1 = add_freeze_pellet(scene, MAX_CANVAS_SIZE.x * 4/7 * SCALE, MAX_CANVAS_SIZE.y * 0.2/5 * SCALE, 20);
    create_collision(scene, ball1, freeze_pellet_1, (collision_handler_t) play_sound, scene, NULL);
    create_collision(scene, ball2, freeze_pellet_1, (collision_handler_t) play_sound, scene, NULL);
    
    golf_course_t *golf_course = golf_course_init(course, hole, start_pos_ball1, start_pos_ball2, WALL_COLOR);
    golf_course_add_walls(golf_course);
    for(size_t i = 0; i < list_size(golf_course_get_walls(golf_course)); i++){
        body_t *wall = list_get(golf_course_get_walls(golf_course), i);
        scene_add_body(scene, wall);
        body_set_color2(wall, ALT_WALL_COLOR);
        create_physics_collision(scene, 0.3, ball1, wall);
        create_physics_collision(scene, 0.3, ball2, wall);
        create_collision(scene, ball1, wall, (collision_handler_t) play_sound, scene, NULL);
        create_collision(scene, ball2, wall, (collision_handler_t) play_sound, scene, NULL);
    }
    create_friction(scene, 100, ball1, course);
    create_friction(scene, 100, ball2, course);
    create_friction(scene, 300, ball1, patch1);
    create_friction(scene, 300, ball2, patch1);
    create_friction(scene, 250, ball1, patch2);
    create_friction(scene, 250, ball2, patch2);
    create_friction(scene, 180, ball1, patch3);
    create_friction(scene, 180, ball2, patch3);
    scene_add_body(scene, ball1);
    scene_add_body(scene, ball2);

    create_collision(scene, ball1, hole, (collision_handler_t) play_sound, scene, NULL);
    create_collision(scene, ball2, hole, (collision_handler_t) play_sound, scene, NULL);

    create_collision(scene, ball1, patch1, (collision_handler_t) play_sound, scene, NULL);
    create_collision(scene, ball2, patch1, (collision_handler_t) play_sound, scene, NULL);
    create_collision(scene, ball1, patch2, (collision_handler_t) play_sound, scene, NULL);
    create_collision(scene, ball2, patch2, (collision_handler_t) play_sound, scene, NULL);
    create_collision(scene, ball1, patch3, (collision_handler_t) play_sound, scene, NULL);
    create_collision(scene, ball2, patch3, (collision_handler_t) play_sound, scene, NULL);
}

void ball_in_hole2(body_t *ball, body_t *target, vector_t axis, scene_t *scene){
    //ball only "goes in the hole" if its velocity is below the max velocity
    if(vec_dot(body_get_velocity(ball), body_get_velocity(ball)) < MAX_HOLE_VEL){
        body_set_velocity(ball, VEC_ZERO);
        if(strcmp(body_get_info(ball), "golf_ball2") == 0){
            ((game_state_t *) scene_get_state(scene))->player2_done = true;
        }
        else{
            ((game_state_t *) scene_get_state(scene))->player1_done = true;
        }
        body_hide(ball);
    }
    if(((game_state_t *) scene_get_state(scene))->player2_done && ((game_state_t *) scene_get_state(scene))->player1_done){
        for(size_t i = 0; i < scene_bodies(scene); i++){
            if(!body_is_removed(scene_get_body(scene, i))) body_remove(scene_get_body(scene, i));
        }
        ((game_state_t *) scene_get_state(scene))->player1_done = false;
        ((game_state_t *) scene_get_state(scene))->player2_done = false;
        init_course_3(scene);
    }
}

void init_course_2(scene_t *scene){
    Mix_PlayMusic(((game_state_t *)scene_get_state(scene))->hole2, -1);
    ((game_state_t *) scene_get_state(scene))->night_mode = false;
    list_t *background_points = list_init(POINTS, (free_func_t) body_free_vec_list);
    vector_t *bp1 = malloc(sizeof(vector_t));
    bp1->x = 0;
    bp1->y = 0;
    list_add(background_points, bp1);
    vector_t *bp2 = malloc(sizeof(vector_t));
    bp2->x = 0;
    bp2->y = MAX_CANVAS_SIZE.y;
    list_add(background_points, bp2);
    vector_t *bp3 = malloc(sizeof(vector_t));
    bp3->x = MAX_CANVAS_SIZE.x;
    bp3->y = MAX_CANVAS_SIZE.y;
    list_add(background_points, bp3);
    vector_t *bp4 = malloc(sizeof(vector_t));
    bp4->x = MAX_CANVAS_SIZE.x;
    bp4->y = 0;
    list_add(background_points, bp4);
    body_t *background = body_init_with_info(background_points, INFINITY, BACKGROUND_COLOR, "background", free);
    scene_add_body(scene, background);
    body_set_color2(background, ALT_BACKGROUND_COLOR);

    list_t *course_points = list_init(POINTS, (free_func_t) body_free_vec_list);
    vector_t *p1 = malloc(sizeof(vector_t));
    p1->x = 1 * SCALE;
    p1->y = 1 * SCALE;
    list_add(course_points, p1);
    vector_t *p2 = malloc(sizeof(vector_t));
    p2->x = 1 * SCALE;
    p2->y = MAX_CANVAS_SIZE.y * 2/5 * SCALE;
    list_add(course_points, p2);
    vector_t *p3 = malloc(sizeof(vector_t));
    p3->x = MAX_CANVAS_SIZE.x * 2/7 * SCALE;
    p3->y = MAX_CANVAS_SIZE.y * 2/5 * SCALE;
    list_add(course_points, p3);
    vector_t *p4 = malloc(sizeof(vector_t));
    p4->x = MAX_CANVAS_SIZE.x * 2/7 * SCALE;
    p4->y = MAX_CANVAS_SIZE.y* 1 * SCALE;
    list_add(course_points, p4);
    vector_t *p5 = malloc(sizeof(vector_t));
    p5->x = MAX_CANVAS_SIZE.x * 5/7 * SCALE;
    p5->y = MAX_CANVAS_SIZE.y * 1 * SCALE;
    list_add(course_points, p5);
    vector_t *p6 = malloc(sizeof(vector_t));
    p6->x = MAX_CANVAS_SIZE.x * 5/7 * SCALE;
    p6->y = MAX_CANVAS_SIZE.y * 3/6 * SCALE;
    list_add(course_points, p6);
    vector_t *p7 = malloc(sizeof(vector_t));
    p7->x = MAX_CANVAS_SIZE.x * 6/7 * SCALE;
    p7->y = MAX_CANVAS_SIZE.y * 3/6 * SCALE;
    list_add(course_points, p7);
    vector_t *p8 = malloc(sizeof(vector_t));
    p8->x = MAX_CANVAS_SIZE.x * 6/7 * SCALE;
    p8->y = MAX_CANVAS_SIZE.y * 1 * SCALE;
    list_add(course_points, p8);
    vector_t *p9 = malloc(sizeof(vector_t));
    p9->x = MAX_CANVAS_SIZE.x * 1 * SCALE;
    p9->y = MAX_CANVAS_SIZE.y * 1 * SCALE;
    list_add(course_points, p9);
    vector_t *p10 = malloc(sizeof(vector_t));
    p10->x = MAX_CANVAS_SIZE.x * 1 * SCALE;
    p10->y = 1;
    list_add(course_points, p10);
    vector_t *p11 = malloc(sizeof(vector_t));
    p11->x = MAX_CANVAS_SIZE.x * 4/7 * SCALE;
    p11->y = 1;
    list_add(course_points, p11);
    vector_t *p12 = malloc(sizeof(vector_t));
    p12->x = MAX_CANVAS_SIZE.x * 4/7 * SCALE;
    p12->y = MAX_CANVAS_SIZE.y * 2/6 * SCALE;
    list_add(course_points, p12);
    vector_t *p13 = malloc(sizeof(vector_t));
    p13->x = MAX_CANVAS_SIZE.x * 3/7 * SCALE;
    p13->y = MAX_CANVAS_SIZE.y * 2/6 * SCALE;
    list_add(course_points, p13);
    vector_t *p14 = malloc(sizeof(vector_t));
    p14->x = MAX_CANVAS_SIZE.x * 3/7 * SCALE;
    p14->y = MAX_CANVAS_SIZE.y * 1/6 * SCALE;
    list_add(course_points, p14);
    vector_t *p15 = malloc(sizeof(vector_t));
    p15->x = MAX_CANVAS_SIZE.x * 1/7 * SCALE;
    p15->y = MAX_CANVAS_SIZE.y * 1/6 * SCALE;
    list_add(course_points, p15);
    vector_t *p16 = malloc(sizeof(vector_t));
    p16->x = MAX_CANVAS_SIZE.x * 1/7 * SCALE;
    p16->y = 1;
    list_add(course_points, p16);
    body_t *course = body_init_with_info(course_points, INFINITY, COURSE_COLOR, "course", free);
    scene_add_body(scene, course);
    body_set_color2(course, ALT_COURSE_COLOR);
    
    list_t *slope_points = list_init(POINTS, (free_func_t) body_free_vec_list);
    vector_t *slope11 = malloc(sizeof(vector_t));
    slope11->x = MAX_CANVAS_SIZE.x * 4/7 * SCALE;
    slope11->y = MAX_CANVAS_SIZE.y * 0 * SCALE;
    list_add(slope_points, slope11);
    vector_t *slope12 = malloc(sizeof(vector_t));
    slope12->x = MAX_CANVAS_SIZE.x * 1 * SCALE;
    slope12->y = MAX_CANVAS_SIZE.y * 0 * SCALE;
    list_add(slope_points, slope12);
    vector_t *slope13 = malloc(sizeof(vector_t));
    slope13->x = MAX_CANVAS_SIZE.x * 1 * SCALE;
    slope13->y = MAX_CANVAS_SIZE.y * 2/6 * SCALE;
    list_add(slope_points, slope13);
    vector_t *slope14 = malloc(sizeof(vector_t));
    slope14->x = MAX_CANVAS_SIZE.x * 4/7 * SCALE;
    slope14->y = MAX_CANVAS_SIZE.y * 2/6 * SCALE;
    list_add(slope_points, slope14);
    body_t *slope1 = body_init_with_info(slope_points, INFINITY, SLOPE_COLOR, "slope", free);
    scene_add_body(scene, slope1);
    body_set_color2(slope1, ALT_SLOPE_COLOR);

    double hole_pos_x = MAX_CANVAS_SIZE.x * 13/14 * SCALE;
    double hole_pos_y = MAX_CANVAS_SIZE.y * 11/12 * SCALE;
    vector_t center = {.x=hole_pos_x, .y=hole_pos_y};
    list_t *circle = draw_circle(center, HOLE_SIZE * SCALE);
    body_t *hole = body_init_with_info(circle, INFINITY, HOLE_COLOR, "hole", free);
    scene_add_body(scene, hole);
    body_set_color2(hole, ALT_HOLE_COLOR);

    list_t *circle_big = draw_circle(center, HOLE_SIZE * 2 * SCALE);
    body_t *hole_real = body_init_with_info(circle_big, INFINITY, HOLE_COLOR, "hole_real", free);
    scene_add_body(scene, hole_real);
    body_set_color2(hole_real, ALT_HOLE_COLOR);

    vector_t start_pos_ball1 = {.x=MAX_CANVAS_SIZE.x * 1/30 * SCALE, .y=MAX_CANVAS_SIZE.y * 1/20 * SCALE};
    list_t *circle1 = draw_circle(start_pos_ball1, HOLE_SIZE * SCALE);
    body_t *ball1 = body_init_with_info(circle1, MASS, BALL1_COLOR, "golf_ball1", free);
    create_collision(scene, ball1, hole, (collision_handler_t) ball_in_hole2, scene, NULL);

    vector_t start_pos_ball2 = {.x=MAX_CANVAS_SIZE.x * 1/15 * SCALE, .y=MAX_CANVAS_SIZE.y * 1/20 * SCALE};
    list_t *circle2 = draw_circle(start_pos_ball2, HOLE_SIZE * SCALE);
    body_t *ball2 = body_init_with_info(circle2, MASS, BALL2_COLOR, "golf_ball2", free);
    create_collision(scene, ball2, hole, (collision_handler_t) ball_in_hole2, scene, NULL);

    vector_t slope_direction = {.x = 0, .y = 1};
    create_frictional_and_slope_force(scene, 0.3, 40, slope_direction, 2000, ball1, slope1);
    create_frictional_and_slope_force(scene, 0.3, 40, slope_direction, 2000, ball2, slope1);
    
    vector_t center_bouncy = {.x = MAX_CANVAS_SIZE.x * 3.5/7 * SCALE, .y = MAX_CANVAS_SIZE.y * 3.5/5 * SCALE};
    list_t *bouncy_ball_points = draw_star(center_bouncy, 100);
    body_t *bouncy_ball1 = body_init_with_info(bouncy_ball_points, INFINITY, BOUNCY_COLOR, "bouncy_ball", free);
    body_set_color2(bouncy_ball1, ALT_BOUNCY_COLOR);
    create_physics_collision(scene, 1.5, ball1, bouncy_ball1);
    create_physics_collision(scene, 1.5, ball2, bouncy_ball1);
    create_collision(scene, ball1, bouncy_ball1, (collision_handler_t) play_sound, scene, NULL);
    create_collision(scene, ball2, bouncy_ball1, (collision_handler_t) play_sound, scene, NULL);
    scene_add_body(scene, bouncy_ball1);

    golf_course_t *golf_course = golf_course_init(course, hole, start_pos_ball1, start_pos_ball2, WALL_COLOR);
    golf_course_add_walls(golf_course);
    for(size_t i = 0; i < list_size(golf_course_get_walls(golf_course)); i++){
        body_t *wall = list_get(golf_course_get_walls(golf_course), i);
        scene_add_body(scene, wall);
        body_set_color2(wall, ALT_WALL_COLOR);
        create_physics_collision(scene, 0.3, ball1, wall);
        create_physics_collision(scene, 0.3, ball2, wall);
        create_collision(scene, ball1, wall, (collision_handler_t) play_sound, scene, NULL);
        create_collision(scene, ball2, wall, (collision_handler_t) play_sound, scene, NULL);
    }
    create_friction(scene, 100, ball1, course);
    create_friction(scene, 100, ball2, course);

    scene_add_body(scene, ball1);
    scene_add_body(scene, ball2);

    create_collision(scene, ball1, hole, (collision_handler_t) play_sound, scene, NULL);
    create_collision(scene, ball2, hole, (collision_handler_t) play_sound, scene, NULL);
    
    body_t *coin1 = add_coin(scene, MAX_CANVAS_SIZE.x * 5/7 * SCALE, MAX_CANVAS_SIZE.y * 1/7 * SCALE, -0.2);
    body_t *coin2 = add_coin(scene, MAX_CANVAS_SIZE.x * 6/7 * SCALE, MAX_CANVAS_SIZE.y * 1/7 * SCALE, 0.4);
    body_t *freeze_pellet_1 = add_freeze_pellet(scene, MAX_CANVAS_SIZE.x * 2.5/7 * SCALE, MAX_CANVAS_SIZE.y * 3/5 * SCALE, 10);

    create_collision(scene, ball1, coin1, (collision_handler_t) play_sound, scene, NULL);
    create_collision(scene, ball2, coin1, (collision_handler_t) play_sound, scene, NULL);

    create_collision(scene, ball1, coin2, (collision_handler_t) play_sound, scene, NULL);
    create_collision(scene, ball2, coin2, (collision_handler_t) play_sound, scene, NULL);

    create_collision(scene, ball1, freeze_pellet_1, (collision_handler_t) play_sound, scene, NULL);
    create_collision(scene, ball2, freeze_pellet_1, (collision_handler_t) play_sound, scene, NULL);
}

void ball_in_hole1(body_t *ball, body_t *target, vector_t axis, scene_t *scene){
    //ball only "goes in the hole" if its velocity is below the max velocity
    if(vec_dot(body_get_velocity(ball), body_get_velocity(ball)) < MAX_HOLE_VEL){
        body_set_velocity(ball, VEC_ZERO);
        if(strcmp(body_get_info(ball), "golf_ball2") == 0){
            ((game_state_t *) scene_get_state(scene))->player2_done = true;
        }
        else{
            ((game_state_t *) scene_get_state(scene))->player1_done = true;
        }
        body_hide(ball);
    }
    if(((game_state_t *) scene_get_state(scene))->player2_done && ((game_state_t *) scene_get_state(scene))->player1_done){
        for(size_t i = 0; i < scene_bodies(scene); i++){
            if(!body_is_removed(scene_get_body(scene, i))) body_remove(scene_get_body(scene, i));
        }
        ((game_state_t *) scene_get_state(scene))->player1_done = false;
        ((game_state_t *) scene_get_state(scene))->player2_done = false;
        init_course_2(scene);
    }
}

void init_course_1(scene_t *scene){
    Mix_PlayMusic(((game_state_t *)scene_get_state(scene))->hole1, -1);
    ((game_state_t *) scene_get_state(scene))->night_mode = false;
    list_t *background_points = list_init(POINTS, (free_func_t) body_free_vec_list);
    vector_t *bp1 = malloc(sizeof(vector_t));
    bp1->x = 0;
    bp1->y = 0;
    list_add(background_points, bp1);
    vector_t *bp2 = malloc(sizeof(vector_t));
    bp2->x = 0;
    bp2->y = MAX_CANVAS_SIZE.y;
    list_add(background_points, bp2);
    vector_t *bp3 = malloc(sizeof(vector_t));
    bp3->x = MAX_CANVAS_SIZE.x;
    bp3->y = MAX_CANVAS_SIZE.y;
    list_add(background_points, bp3);
    vector_t *bp4 = malloc(sizeof(vector_t));
    bp4->x = MAX_CANVAS_SIZE.x;
    bp4->y = 0;
    list_add(background_points, bp4);
    body_t *background = body_init_with_info(background_points, INFINITY, BACKGROUND_COLOR, "background", free);
    scene_add_body(scene, background);
    body_set_color2(background, ALT_BACKGROUND_COLOR);

    list_t *course_points = list_init(POINTS, (free_func_t) body_free_vec_list);
    vector_t *p1 = malloc(sizeof(vector_t));
    p1->x = 1 * SCALE;
    p1->y = 1 * SCALE;
    list_add(course_points, p1);
    vector_t *p2 = malloc(sizeof(vector_t));
    p2->x = 1 * SCALE;
    p2->y = MAX_CANVAS_SIZE.y * SCALE;
    list_add(course_points, p2);
    vector_t *p3 = malloc(sizeof(vector_t));
    p3->x = MAX_CANVAS_SIZE.x * SCALE;
    p3->y = MAX_CANVAS_SIZE.y * SCALE;
    list_add(course_points, p3);
    vector_t *p4 = malloc(sizeof(vector_t));
    p4->x = MAX_CANVAS_SIZE.x * SCALE;
    p4->y = 1 * SCALE;
    list_add(course_points, p4);
    vector_t *p5 = malloc(sizeof(vector_t));
    p5->x = MAX_CANVAS_SIZE.x * 4/5 * SCALE;
    p5->y = 1 * SCALE;
    list_add(course_points, p5);
    vector_t *p6 = malloc(sizeof(vector_t));
    p6->x = MAX_CANVAS_SIZE.x * 4/5 * SCALE;
    p6->y = MAX_CANVAS_SIZE.y * 3/5 * SCALE;
    list_add(course_points, p6);
    vector_t *p7 = malloc(sizeof(vector_t));
    p7->x = MAX_CANVAS_SIZE.x * 1/5 * SCALE;
    p7->y = MAX_CANVAS_SIZE.y * 3/5 * SCALE;
    list_add(course_points, p7);
    vector_t *p8 = malloc(sizeof(vector_t));
    p8->x = MAX_CANVAS_SIZE.x * 1/5 * SCALE;
    p8->y = 1 * SCALE;
    list_add(course_points, p8);
    
    body_t *course = body_init_with_info(course_points, INFINITY, COURSE_COLOR, "course", free);
    scene_add_body(scene, course);
    body_set_color2(course, ALT_COURSE_COLOR);
    
    list_t *patch1_points = list_init(POINTS, (free_func_t) body_free_vec_list);
    vector_t *patch11 = malloc(sizeof(vector_t));
    patch11->x = MAX_CANVAS_SIZE.x * 3.7/5 * SCALE;
    patch11->y = MAX_CANVAS_SIZE.y * 3.5/5 * SCALE;
    list_add(patch1_points, patch11);
    vector_t *patch12 = malloc(sizeof(vector_t));
    patch12->x = MAX_CANVAS_SIZE.x * 3.7/5 * SCALE;
    patch12->y = MAX_CANVAS_SIZE.y * 4.2/5 * SCALE;
    list_add(patch1_points, patch12);
    vector_t *patch13 = malloc(sizeof(vector_t));
    patch13->x = MAX_CANVAS_SIZE.x * 4.2/5 * SCALE;
    patch13->y = MAX_CANVAS_SIZE.y * 4.2/5 * SCALE;
    list_add(patch1_points, patch13);
    vector_t *patch14 = malloc(sizeof(vector_t));
    patch14->x = MAX_CANVAS_SIZE.x * 4.2/5 * SCALE;
    patch14->y = MAX_CANVAS_SIZE.y * 3.5/5 * SCALE;
    list_add(patch1_points, patch14);
    body_t *patch1 = body_init_with_info(patch1_points, INFINITY, PATCH_COLOR, "patch", free);
    scene_add_body(scene, patch1);
    body_set_color2(patch1, ALT_PATCH_COLOR);
    
    list_t *slope_points = list_init(POINTS, (free_func_t) body_free_vec_list);
    vector_t *slope11 = malloc(sizeof(vector_t));
    slope11->x = MAX_CANVAS_SIZE.x * 2/5 * SCALE;
    slope11->y = MAX_CANVAS_SIZE.y * 3/5 * SCALE;
    list_add(slope_points, slope11);
    vector_t *slope12 = malloc(sizeof(vector_t));
    slope12->x = MAX_CANVAS_SIZE.x * 2/5 * SCALE;
    slope12->y = MAX_CANVAS_SIZE.y * SCALE;
    list_add(slope_points, slope12);
    vector_t *slope13 = malloc(sizeof(vector_t));
    slope13->x = MAX_CANVAS_SIZE.x * 3/5 * SCALE;
    slope13->y = MAX_CANVAS_SIZE.y * SCALE;
    list_add(slope_points, slope13);
    vector_t *slope14 = malloc(sizeof(vector_t));
    slope14->x = MAX_CANVAS_SIZE.x * 3/5 * SCALE;
    slope14->y = MAX_CANVAS_SIZE.y * 3/5 * SCALE;
    list_add(slope_points, slope14);
    body_t *slope1 = body_init_with_info(slope_points, INFINITY, SLOPE_COLOR, "slope", free);
    scene_add_body(scene, slope1);
    body_set_color2(slope1, ALT_SLOPE_COLOR);

    //A force surface is a patch that applies a force on the ball
    list_t *force_surface_points = list_init(POINTS, (free_func_t) body_free_vec_list);
    vector_t *force_surface11 = malloc(sizeof(vector_t));
    force_surface11->x = MAX_CANVAS_SIZE.x * 0.5/5 * SCALE;
    force_surface11->y = MAX_CANVAS_SIZE.y * 3.8/5 * SCALE;
    list_add(force_surface_points, force_surface11);
    vector_t *force_surface12 = malloc(sizeof(vector_t));
    force_surface12->x = MAX_CANVAS_SIZE.x * 0.5/5 * SCALE;
    force_surface12->y = MAX_CANVAS_SIZE.y * 9/10 * SCALE;
    list_add(force_surface_points, force_surface12);
    vector_t *force_surface13 = malloc(sizeof(vector_t));
    force_surface13->x = MAX_CANVAS_SIZE.x * 1.25/5 * SCALE;
    force_surface13->y = MAX_CANVAS_SIZE.y * 8.5/10 * SCALE;
    list_add(force_surface_points, force_surface13);
    vector_t *force_surface14 = malloc(sizeof(vector_t));
    force_surface14->x = MAX_CANVAS_SIZE.x * 1.25/5 * SCALE;
    force_surface14->y = MAX_CANVAS_SIZE.y * 3.3/5 * SCALE;
    list_add(force_surface_points, force_surface14);
    body_t *force_surface1 = body_init_with_info(force_surface_points, INFINITY, FORCE_COLOR, "force_surface", free);
    scene_add_body(scene, force_surface1);
    body_set_color2(force_surface1, ALT_FORCE_COLOR);
    
    double hole_pos_x = MAX_CANVAS_SIZE.x * 9/10 * SCALE;
    double hole_pos_y = MAX_CANVAS_SIZE.y * 1/10 * SCALE;

    vector_t center = {.x=hole_pos_x, .y=hole_pos_y};
    list_t *circle = draw_circle(center, HOLE_SIZE * SCALE);
    body_t *hole = body_init_with_info(circle, INFINITY, HOLE_COLOR, "hole", free);
    scene_add_body(scene, hole);
    body_set_color2(hole, ALT_HOLE_COLOR);

    list_t *circle_big = draw_circle(center, HOLE_SIZE * 2 * SCALE);
    body_t *hole_real = body_init_with_info(circle_big, INFINITY, HOLE_COLOR, "hole_real", free);
    scene_add_body(scene, hole_real);
    body_set_color2(hole_real, ALT_HOLE_COLOR);

    vector_t start_pos_ball1 = {.x=MAX_CANVAS_SIZE.x * 1/30 * SCALE, .y=MAX_CANVAS_SIZE.y * 1/20 * SCALE};
    list_t *circle1 = draw_circle(start_pos_ball1, HOLE_SIZE * SCALE);
    body_t *ball1 = body_init_with_info(circle1, MASS, BALL1_COLOR, "golf_ball1", free);
    create_collision(scene, ball1, hole, (collision_handler_t) ball_in_hole1, scene, NULL);

    vector_t start_pos_ball2 = {.x=MAX_CANVAS_SIZE.x * 2/15 * SCALE, .y=MAX_CANVAS_SIZE.y * 1/20 * SCALE};
    list_t *circle2 = draw_circle(start_pos_ball2, HOLE_SIZE * SCALE);
    body_t *ball2 = body_init_with_info(circle2, MASS, BALL2_COLOR, "golf_ball2", free);
    create_collision(scene, ball2, hole, (collision_handler_t) ball_in_hole1, scene, NULL);
    
    vector_t slope_direction = {.x = -1, .y = 0};
    create_frictional_and_slope_force(scene, 0.3, 40, slope_direction, 5000, ball1, slope1);
    create_frictional_and_slope_force(scene, 0.3, 40, slope_direction, 5000, ball2, slope1);

    vector_t force_direction = {.x = 1, .y = 0.5};
    create_force_collision(scene, force_direction, ball1, force_surface1);
    create_force_collision(scene, force_direction, ball2, force_surface1);

    vector_t center_bouncy = {.x = MAX_CANVAS_SIZE.x * 0.7/7 * SCALE, .y = MAX_CANVAS_SIZE.y * 3/5 * SCALE};
    list_t *bouncy_ball_points = draw_circle(center_bouncy, 50);
    body_t *bouncy_ball1 = body_init_with_info(bouncy_ball_points, INFINITY, BOUNCY_COLOR, "bouncy_ball", free);
    body_set_color2(bouncy_ball1, ALT_BOUNCY_COLOR);
    create_physics_collision(scene, 1.5, ball1, bouncy_ball1);
    create_physics_collision(scene, 1.5, ball2, bouncy_ball1);
    create_collision(scene, ball1, bouncy_ball1, (collision_handler_t) play_sound, scene, NULL);
    create_collision(scene, ball2, bouncy_ball1, (collision_handler_t) play_sound, scene, NULL);
    scene_add_body(scene, bouncy_ball1);
    
    golf_course_t *golf_course = golf_course_init(course, hole, start_pos_ball1, start_pos_ball2, WALL_COLOR);
    golf_course_add_walls(golf_course);
    for(size_t i = 0; i < list_size(golf_course_get_walls(golf_course)); i++){
        body_t *wall = list_get(golf_course_get_walls(golf_course), i);
        scene_add_body(scene, wall);
        body_set_color2(wall, ALT_WALL_COLOR);
        create_physics_collision(scene, 0.3, ball1, wall);
        create_physics_collision(scene, 0.3, ball2, wall);
        create_collision(scene, ball1, wall, (collision_handler_t) play_sound, scene, NULL);
        create_collision(scene, ball2, wall, (collision_handler_t) play_sound, scene, NULL);
    }
    create_friction(scene, 100, ball1, course);
    create_friction(scene, 100, ball2, course);
    create_friction(scene, 250, ball1, patch1);
    create_friction(scene, 250, ball2, patch1);
    
    scene_add_body(scene, ball1);
    scene_add_body(scene, ball2);
    
    create_collision(scene, ball1, hole, (collision_handler_t) play_sound, scene, NULL);
    create_collision(scene, ball2, hole, (collision_handler_t) play_sound, scene, NULL);

    create_collision(scene, ball1, patch1, (collision_handler_t) play_sound, scene, NULL);
    create_collision(scene, ball2, patch1, (collision_handler_t) play_sound, scene, NULL);
    
    
    body_t *coin1 = add_coin(scene, MAX_CANVAS_SIZE.x * 1/6 * SCALE, MAX_CANVAS_SIZE.y * 1/5 * SCALE, 0.6);
    body_t *freeze_pellet_1 = add_freeze_pellet(scene, MAX_CANVAS_SIZE.x * 1/7 * SCALE, MAX_CANVAS_SIZE.y * 1/5 * SCALE, 30);

    create_collision(scene, ball1, coin1, (collision_handler_t) play_sound, scene, NULL);
    create_collision(scene, ball2, coin1, (collision_handler_t) play_sound, scene, NULL);

    create_collision(scene, ball1, freeze_pellet_1, (collision_handler_t) play_sound, scene, NULL);
    create_collision(scene, ball2, freeze_pellet_1, (collision_handler_t) play_sound, scene, NULL);
}

//adds all of the text for the game
void draw_text(scene_t *scene, TTF_Font *font, double dt){
    char *points = malloc(sizeof(char) * 1000);
    char *turn = malloc(sizeof(char) * 1000);
    char *freeze1 = malloc(sizeof(char) * 1000);
    char *freeze2 = malloc(sizeof(char) * 1000);
    char *player_win = malloc(sizeof(char) * 1000);
    char *leaderboard = malloc(sizeof(char) * 1000);

    SDL_Color color = ((game_state_t *) scene_get_state(scene))->text_color_day;
    if(((game_state_t *) scene_get_state(scene))->night_mode){
        color = ((game_state_t *) scene_get_state(scene))->text_color_beach;
    }
    //displays the score
    sprintf(points, "player 1: %f  player 2: %f", ((game_state_t *) scene_get_state(scene))->player1_points, ((game_state_t *) scene_get_state(scene))->player2_points);
    SDL_Surface *surface = TTF_RenderText_Solid(font, points, color);
    sdl_draw_text(surface, 0, 0, 300, 30);
    //displays whose turn it is
    sprintf(turn, "%s turn", get_turn(scene));
    SDL_Surface *surface2 = TTF_RenderText_Solid(font, turn, color);
    sdl_draw_text(surface2, 0, 40, 200, 30);

    //displays the freeze counters if any player hits the freeze pellet 
    if(((game_state_t *) scene_get_state(scene))->freeze_player_1){
        sprintf(freeze1, "freeze time player 1: %f", ((game_state_t *) scene_get_state(scene))->freeze_time_1);
        SDL_Surface *surface3 = TTF_RenderText_Solid(font, freeze1, color);
        sdl_draw_text(surface3, 0, 80, 200, 30);
        ((game_state_t *) scene_get_state(scene))->freeze_time_1 -= dt;
        if(((game_state_t *) scene_get_state(scene))->freeze_time_1 <= 0){
            ((game_state_t *) scene_get_state(scene))->freeze_player_1 = false;
            ((game_state_t *) scene_get_state(scene))->freeze_time_1 = 0;
        }
    }
    if(((game_state_t *) scene_get_state(scene))->freeze_player_2){
        sprintf(freeze2, "freeze time player 2: %f", ((game_state_t *) scene_get_state(scene))->freeze_time_2);
        SDL_Surface *surface4 = TTF_RenderText_Solid(font, freeze2, color);
        sdl_draw_text(surface4, 0, 120, 200, 30);
        ((game_state_t *) scene_get_state(scene))->freeze_time_2 -= dt;
        if(((game_state_t *) scene_get_state(scene))->freeze_time_2 <= 0){
            ((game_state_t *) scene_get_state(scene))->freeze_player_2 = false;
            ((game_state_t *) scene_get_state(scene))->freeze_time_2 = 0;
        }
    }

    //displays the leaderboard and who won if the game is over
    if(((game_state_t *) scene_get_state(scene))->is_over){
        if(((game_state_t *) scene_get_state(scene))->player1_points < ((game_state_t *) scene_get_state(scene))->player2_points){
            sprintf(player_win, "GAME OVER player 1 won");
        }
        else if(((game_state_t *) scene_get_state(scene))->player1_points > ((game_state_t *) scene_get_state(scene))->player2_points){
            sprintf(player_win, "GAME OVER player 2 won");
        }
        else{
            sprintf(player_win, "GAME OVER it is a tie");
        }
        SDL_Surface *surface3 = TTF_RenderText_Solid(font, player_win, color);
        sdl_draw_text(surface3, 350, 30, 300, 30);

        sprintf(leaderboard, "TOP SCORES!!!");
        SDL_Surface *surface4 = TTF_RenderText_Solid(font, leaderboard, color);
        sdl_draw_text(surface4, 350, 60, 300, 30);
        
        list_t *scores = get_scores();
        int num_scores = 10;
        if(list_size(scores) < 10) num_scores = list_size(scores);
        char *score = malloc(sizeof(char) * 1000);
        for(size_t i = 0; i < num_scores; i++){
            double *str = list_get(scores, i);
            sprintf(score, "%zu: %f", i + 1, *str);
            SDL_Surface *surface4 = TTF_RenderText_Solid(font, score, color);
            sdl_draw_text(surface4, 445, 90 + (30 * i), 100, 30);
        }
        free(score);
        list_free(scores);
    }
    free(points);
    free(turn);
    free(freeze1);
    free(freeze2);
    free(player_win);
    free(leaderboard);
}

int main(int argc, char *argv[]) {
    //initialization
    game_state_t *game = game_state_init();
    sdl_init(VEC_ZERO, MAX_CANVAS_SIZE);
    scene_t *scene = scene_init();
    scene_set_state(scene, game);
    sdl_on_key(on_key);
    sdl_on_click(on_click);
    sdl_on_click_no_release(display_hit_path);
    sdl_on_scroll(on_scroll);
    sdl_mouse_button_up(on_mouse_button_up);

    TTF_Init();
    TTF_Font *font = TTF_OpenFont("static/fonts/arial.ttf", 100);
    ((game_state_t *) scene_get_state(scene))->text_color_day = (SDL_Color){ 0, 0, 255 };
    ((game_state_t *) scene_get_state(scene))->text_color_beach = (SDL_Color){ 255, 100, 0 };

    Mix_OpenAudio(44100, AUDIO_S16SYS, 2, 512);
    Mix_AllocateChannels(4);
    ((game_state_t *)scene_get_state(scene))->bounce = Mix_LoadWAV("static/sound/bounce.wav");
    ((game_state_t *)scene_get_state(scene))->hole = Mix_LoadWAV("static/sound/hole.wav");
    ((game_state_t *)scene_get_state(scene))->frict = Mix_LoadWAV("static/sound/friction.wav");
    ((game_state_t *)scene_get_state(scene))->coin = Mix_LoadWAV("static/sound/coin.wav");
    ((game_state_t *)scene_get_state(scene))->freeze = Mix_LoadWAV("static/sound/freeze.wav");
    ((game_state_t *)scene_get_state(scene))->bouncy_ball = Mix_LoadWAV("static/sound/bouncy_ball.wav");

    ((game_state_t *)scene_get_state(scene))->hole1 = Mix_LoadMUS("static/sound/music4.wav");
    ((game_state_t *)scene_get_state(scene))->hole2 = Mix_LoadMUS("static/sound/music1.wav");
    ((game_state_t *)scene_get_state(scene))->hole3 = Mix_LoadMUS("static/sound/music3.wav");
    Mix_VolumeMusic(MIX_MAX_VOLUME/20);
    init_course_1(scene);

    //runs until the window closes
    while (!sdl_is_done(scene)) {
        double dt = time_since_last_tick();
        automatic_scroll(scene);
        scene_tick(scene, dt);

        draw_text(scene, font, dt);
        sdl_render_scene(scene);

        delete_hit_path(scene);
    }

    //frees and destroys everything
    TTF_CloseFont(font);
    TTF_Quit();
    Mix_FreeMusic(((game_state_t *)scene_get_state(scene))->hole1);
    Mix_FreeMusic(((game_state_t *)scene_get_state(scene))->hole2);
    Mix_FreeMusic(((game_state_t *)scene_get_state(scene))->hole3);
    Mix_FreeChunk(((game_state_t *)scene_get_state(scene))->bounce);
    Mix_FreeChunk(((game_state_t *)scene_get_state(scene))->hole);
    Mix_FreeChunk(((game_state_t *)scene_get_state(scene))->frict);
    Mix_FreeChunk(((game_state_t *)scene_get_state(scene))->bouncy_ball);
    Mix_CloseAudio();
    scene_free(scene);
    return 0;
}