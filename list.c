#include <stdlib.h>
#include <stdio.h>
#include <assert.h>
#include "list.h"
#include "body.h"

const int GROWING_RATE = 2;

typedef struct list {
    void **arr;
    size_t length;
    size_t capacity;
    free_func_t free_list;
} list_t;

list_t *list_init(size_t initial_size, free_func_t freer){
    list_t *new_lst = malloc(sizeof(list_t));
    assert(new_lst);
    new_lst->arr = malloc(initial_size * sizeof(void*));
    new_lst->length = 0;
    new_lst->capacity = initial_size;
    new_lst->free_list = freer;
    return new_lst;
}

void list_free(list_t *list){
    list->free_list(list);
}

size_t list_size(list_t *list){
    return list->length;
}

void *list_get(list_t *list, size_t index){
    assert(index < list->length);
    return list->arr[index];
}

void list_set(list_t *list, size_t index, void* value){
    assert(index < list->length);
    list->arr[index] = value;
}

void list_add(list_t *list, void *value){
    assert(value);
    if(list->length >= list->capacity){
        *list = *list_increase_capacity(list);
    }
    list->arr[list->length] = value;
    list->length++;
}

list_t *list_increase_capacity(list_t *list){
    list_t *new_list = list_init((list->capacity + 1) * GROWING_RATE, list->free_list);
    for(size_t i = 0; i < list->length; i++){
        new_list->arr[i] = list->arr[i];
    }
    new_list->length = list->length;
    return new_list;
}

void *list_remove(list_t *list, size_t index){
    assert(list->length > 0);
    void *removed_val = list->arr[index];
    for(int i = index; i < list->length - 1; i++) {
        list->arr[i] = list->arr[i + 1];
    }
    list->length--;
    return removed_val;
}