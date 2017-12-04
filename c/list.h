#ifndef LIST_H
#define LIST_H

/*
 * file list.h
 * Header file for linked lists
 */

#include <stdbool.h>
#include <stdlib.h>

/*
 * struct list_cell_t
 * brief Structure describing an element of a list
 */

typedef struct list_cell_s {
    void              * element; /* Pointer to the element stored in this cell */
    struct list_cell_s * next;   /* Pointer to the next cell in the list */
} list_cell_t;

/*
 * struct list_t
 * Structure describing a list
 */

typedef struct {
    list_cell_t * head;
    list_cell_t * tail;
} list_t;


list_cell_t * list_cell_create(void * element);


void list_cell_free(list_cell_t * list_cell, void (*element_free) (void * element));


list_t * list_create();


void list_free(list_t *list, void (*element_free)(void * element));


bool list_push_element(list_t * list, void * element);


void * list_pop_element(list_t * list, void (*element_free)(void * element));

#endif
