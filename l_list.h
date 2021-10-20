#include <stdlib.h>
#include <stdio.h>
#include <string.h>

typedef struct node {
  void* data;
  struct node* previous;
  struct node* next;
  unsigned long byte_size;
}node;

typedef struct {
  unsigned long byte_size;
  unsigned int length;
  node* head;
  node* tail;
}l_list;

l_list* create_list ();
node* create_node (void *data);
void enqueue(l_list *list, node *new_node);
void push(l_list *list, node *new_node);
void * dequeue (l_list *list);
int pop(l_list *list);
void clear_list(l_list *list);
void clear_list_helper(l_list *list, void(*helper_delete)(void *));
