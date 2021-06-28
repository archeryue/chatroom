//coder : archer
#include "rb_tree.h"

static node_t * new_node(int key,char * name,char *ip) {
    node_t * mynode =(node_t *)malloc(sizeof(node_t));

    if (!mynode) {
        printf("new node error\n");
        exit(1);
    }

    mynode->key=key;
    mynode->left=NULL;
    mynode->right=NULL;
    mynode->p=NULL;
    mynode->color=RED;
    strcpy(mynode->name,name);
    strcpy(mynode->ip,ip);

    return mynode;
}

static node_t * rotate_left(node_t * node,node_t * root) {
    node_t * right=node->right;

    if (node->right=right->left)
        right->left->p=node;
    right->left=node;

    if (right->p=node->p) {
        if (node == node->p->left)
            node->p->left=right;
        else
            node->p->right=right;
    } else {
        root=right;
    }

    node->p=right;

    return root;
}

static node_t * rotate_right(node_t * node,node_t * root) {
    node_t * left=node->left;

    if (node->left=left->right)
        left->right->p=node;
    left->right=node;

    if (left->p=node->p) {
        if (node == node->p->left)
            node->p->left=left;
        else
            node->p->right=left;
    } else {
        root=left;
    }

    node->p=left;

    return root;
}

static node_t * rb_search_auxiliary(key_t key,node_t *root,node_t **save) {
    node_t *node=root;
    node_t *p=NULL;

    while(node) {
        p=node;
        if (node->key > key) {
            node=node->left;
        } else if (node->key < key) {
            node=node->right;
        } else {
            return node;
        }
    }

    if (save) {
        *save=p;
    }

    return NULL;
}

node_t * rb_search(key_t key,node_t * root) {
    return rb_search_auxiliary(key,root,NULL);
}


static node_t * rb_insert_fixup(node_t * node,node_t *root);
node_t * rb_insert(key_t key,char * name,char * ip,node_t * root) {
    node_t *p=NULL;
    node_t *node=NULL;

    if (node=rb_search_auxiliary(key,root,&p)) {
        return root;
    }
    node=new_node(key,name,ip);
    node->p=p;

    if (p) {
        if (p->key>key)
            p->left=node;
        else
            p->right=node;
    } else {
        root=node;
    }

    return rb_insert_fixup(node,root);
}

static node_t * rb_insert_fixup(node_t * node,node_t *root) {
    node_t *p,*gp,*uncle;

    while((p=node->p) && (p->color==RED)) {
        gp=p->p;
        if (p==gp->left) {
            uncle=gp->right;
            if (uncle && uncle->color==RED) {
                uncle->color=BLACK;
                p->color=BLACK;
                gp->color=RED;
                node=gp;
            } else {
                if (node==p->right) {
                    root=rotate_left(p,root);
                    node=p;
                    p=node->p;
                }

                p->color=BLACK;
                gp->color=RED;
                root=rotate_right(gp,root);
            }
        } else {
            uncle=gp->left;
            if (uncle && uncle->color==RED) {
                uncle->color=BLACK;
                p->color=BLACK;
                gp->color=RED;
                node=gp;
            } else {
                if (node==p->left) {
                    root=rotate_right(p,root);
                    node=p;
                    p=node->p;
                }

                p->color=BLACK;
                gp->color=RED;
                root=rotate_left(gp,root);
            }
        }
    }
    root->color=BLACK;

    return root;
}
//balance rb_tree after insert;

static node_t * rb_delete_fixup(node_t * node,node_t* p,node_t * root);
node_t * rb_delete(key_t key,node_t * root) {
    node_t * node=NULL;
    node_t * old,*p;
    node_t * left,*child;
    color_t color;

    if(node=rb_search_auxiliary(key,root,NULL)) {
        old=node;
    } else {
        printf("this key doesn`t exist!\n");
        return root;
    }

    if (node->left && node->right) {
        old=node->right;
        while(left=old->left) old=left;
    }

    if (old->left)
        child=old->left;
    else
        child=old->right;

    if (child)
        child->p=old->p;
    p=old->p;

    if (old->p==NULL) {
        root=child;
    } else {
        if (old==old->p->left) old->p->left=child;
        else old->p->right=child;
    }

    if (old!=node) {
        node->key=old->key;
        strcpy(node->name,old->name);
        strcpy(node->ip,old->ip);
    }
    color=old->color;
    free(old);
    if (color==BLACK)
        root=rb_delete_fixup(child,p,root);

    return root;
}

static node_t * rb_delete_fixup(node_t * node,node_t* p,node_t * root) {
    node_t * brother;
    while ((!node || node->color==BLACK) && node!=root) {
        if (node==p->left) {
            brother=p->right;
            if (brother && brother->color==RED) {
                brother->color=BLACK;
                p->color=RED;
                root=rotate_left(p,root);
                brother=p->right;
            }
            if ((!(brother->left) || brother->left->color==BLACK) &&
                (!(brother->right) || brother->right->color==BLACK)) {
                brother->color=RED;
                node=p;
            } else {
                if (!(brother->right) || brother->right->color==BLACK) {
                    brother->color=RED;
                    brother->left->color=BLACK;
                    root=rotate_right(brother,root);
                    brother=p->right;
                }
                brother->color=p->color;
                p->color=BLACK;
                brother->right->color=BLACK;
                root=rotate_left(p,root);
                node=root;
            }
        } else {
            brother=p->left;
            if (brother && brother->color==RED) {
                brother->color=BLACK;
                p->color=RED;
                root=rotate_right(p,root);
                brother=p->left;
            }
            if ((!(brother->right) || brother->right->color==BLACK) &&
                (!(brother->left) || brother->left->color==BLACK)) {
                brother->color=RED;
                node=p;
            } else {
                if (!(brother->left) || brother->left->color==BLACK) {
                    brother->color=RED;
                    brother->right->color=BLACK;
                    root=rotate_left(brother,root);
                    brother=p->left;
                }
                brother->color=p->color;
                p->color=BLACK;
                brother->left->color=BLACK;
                root=rotate_right(p,root);
                node=root;
            }
        }
        if (node) p=node->p;
        if (p==NULL) root=node;
    }
    if (root)
        root->color=BLACK;
    return root;
}
//balance rb_tree after delete;
int rb_print(node_t *node) {
    if (node==NULL) return 0;
    printf("key:%d name:%s ip:%s color:%d\n",node->key,node->name,node->ip,node->color);
    rb_print(node->left);
    rb_print(node->right);
    return 1;
}

int rb_destroy(node_t *node) {
    if (node==NULL) return 0;
    rb_destroy(node->left);
    node->left=NULL;
    rb_destroy(node->right);
    node->right=NULL;
    free(node);
    return 1;
}
