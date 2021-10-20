//
// Created by Karim Alatrash on 2021-09-17.
//
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "l_list.h"

l_list* create_list () {
    l_list *list = malloc(sizeof(l_list));
    list->byte_size = 0;
    list->length = 0;
    list->head = NULL;
    list->tail = NULL;
    return list;
}

node* create_node (void *data) {
    node *new = malloc(sizeof(node));
    new->data = data;
    new->previous = NULL;
    new->next = NULL;
    new->byte_size = 0;
    return new;
}
void enqueue(l_list *list, node *new_node) {
  if (list->head == NULL) {
    list->head = new_node;
    list->tail = new_node;
  }
  else {
    new_node->next = list->head;
    list->head->previous = new_node;
    list->head = new_node;
  }

  list->byte_size += new_node->byte_size;
  list->length++;
}
void push(l_list *list, node *new_node) {
    if(list->head == NULL) {
        list->head = new_node;
        list->tail = new_node;
    }
    else {
        new_node->previous = list->tail;
        list->tail->next = new_node;
        list->tail = new_node;
    }

    list->byte_size += new_node->byte_size;
    list->length++;
}

void * dequeue (l_list *list) {
  if(list->head == NULL) {
    return NULL;
  }
  list->byte_size -= list->tail->byte_size;
  list->length--;
  if(list->head == list->tail) {
    void * ret = list->tail->data;
    free(list->head);
    list->head = NULL;
    list->tail = NULL;
    return ret;
  }
  else {
    void * ret = list->tail->data;
    node* temp = list->tail;
    list->tail = list->tail->previous;
    list->tail->next = NULL;
    free(temp);
    return ret;
  } 
}
int pop(l_list *list) {
    if(list->head == NULL)  {
        return 0;
    }
    list->byte_size -= list->tail->byte_size;
    list->length--;
    if (list->head == list->tail) {
        free(list->head);
        list->head = NULL;
        list->tail = NULL;
    }
    else {
        node *temp = list->tail;
        list->tail = list->tail->previous;
        list->tail->next = NULL;
        free(temp);
    }
    return 1;
}

void clear_list(l_list *list) {
    while(pop(list) == 1);
    free(list);
}

void clear_list_helper(l_list *list, void(*helper_delete)(void *)) {
  void *dat;
  while ( (dat = dequeue(list) ) != NULL) {
    helper_delete(dat);
  }
  free(list);
}
/*
void print_list(l_list *list) {
    node *temp = list->head;
    while(temp != NULL) {
        printf("%s/", temp->name);
        temp = temp->next;
    }
    printf("\n");
    }*/
/*
char* list_as_path(l_list *list, char* f_name, int agree_to_free) {
    if(agree_to_free == 0) {
        return NULL;
    }
    char* path = malloc((sizeof(char)*list->byte_size + list->length + strlen(f_name) + 1) );
    memset(path, 0, (sizeof(char)*list->byte_size + list->length + strlen(f_name) + 1));

    node *temp = list->head;
    while(temp != NULL) {
        strcat(path, temp->name);
        strcat(path, "/");
        temp = temp->next;
    }
    strcat(path, f_name);
    return path;
}*/
