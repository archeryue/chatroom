//coder : archer
#ifndef __RB_TREE_H__
#define __RB_TREE_H__

#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <string.h>

typedef int key_t;
// typedef int data_t;

typedef enum mycolor {
	RED=0,
	BLACK=1
} color_t;

typedef struct rb_node_t {
	struct rb_node_t *left,*right,*p;
	key_t key;
	char name[8];
	char ip[16];
	color_t color;
} node_t;

node_t * rb_search(key_t key,node_t * root);

node_t * rb_insert(key_t key,char * name,char * ip,node_t * root);

node_t * rb_delete(key_t key,node_t * root);

int rb_print(node_t *node);

int rb_destroy(node_t *node);

#endif
