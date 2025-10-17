#include "rbtree.h"

#include <stdlib.h>
#include <stdio.h>

// Help functions
void delete_node(node_t *, node_t *);
void insert_LL(rbtree *, node_t *,node_t *,node_t *);
void insert_LR(rbtree *, node_t *,node_t *,node_t *);
void insert_RL(rbtree *, node_t *,node_t *,node_t *);
void insert_RR(rbtree *, node_t *,node_t *,node_t *);

// Use Sentinel
rbtree *new_rbtree(void) {
  rbtree *p = (rbtree *)calloc(1, sizeof(rbtree));
  p->nil = (node_t *)malloc(sizeof(node_t));
  p->nil->color = RBTREE_BLACK;
  p->nil->key = 0;
  p->nil->parent = p->nil->left = p->nil->right = p->nil;

  p->root = p->nil;
  return p;
}

void delete_node(node_t *p, node_t *nil) {
  if (p->left != nil) delete_node(p->left, nil);
  if (p->right != nil) delete_node(p->right, nil);
  free(p);
  return;
}

void delete_rbtree(rbtree *t) {
  if (t->root != t->nil) delete_node(t->root, t->nil);
  free(t->nil);
  free(t);
}

void insert_LL(rbtree *t, node_t *child, node_t *parent, node_t *grand) {
  if (grand == t->root) {
    t->root = parent;
  } else {
    if (grand->parent->right == grand) grand->parent->right = parent;
    else grand->parent->left = parent;
  }
  parent->parent = grand->parent;

  grand->left = parent->right;
  if (parent->right != t->nil) parent->right->parent = grand;

  grand->parent = parent;
  parent->right = grand;

  grand->color = RBTREE_RED;
  parent->color = RBTREE_BLACK;
}

void insert_LR(rbtree *t, node_t *child, node_t *parent, node_t *grand) {
  if (grand == t->root) {
    t->root = child;
  } else {
    if (grand->parent->right == grand) grand->parent->right = child;
    else grand->parent->left = child;
  }
  child->parent = grand->parent;

  if (child->left != t->nil) child->left->parent = parent;
  if (child->right != t->nil) child->right->parent = grand;
  parent->right = child->left;
  grand->left = child->right;

  child->left = parent;
  parent->parent = child;

  child->right = grand;
  grand->parent = child;

  child->color = RBTREE_BLACK;
  grand->color = RBTREE_RED;
}

void insert_RL(rbtree *t, node_t *child, node_t *parent, node_t *grand) {
  if (grand == t->root) {
    t->root = child;
  } else {
    if (grand->parent->right == grand) grand->parent->right = child;
    else grand->parent->left = child;
  }
  child->parent = grand->parent;

  if (child->left != t->nil) child->left->parent = grand;
  if (child->right != t->nil) child->right->parent = parent;
  parent->left = child->right;
  grand->right = child->left;

  child->left = grand;
  grand->parent = child;

  child->right = parent;
  parent->parent = child;

  child->color = RBTREE_BLACK;
  grand->color = RBTREE_RED;
}

void insert_RR(rbtree *t, node_t *child, node_t *parent, node_t *grand) {
  if (grand == t->root) {
    t->root = parent;
  } else {
    if (grand->parent->right == grand) grand->parent->right = parent;
    else grand->parent->left = parent;
  }
  parent->parent = grand->parent;

  grand->right = parent->left;
  if (parent->left != t->nil) parent->left->parent = grand;

  grand->parent = parent;
  parent->left = grand;

  grand->color = RBTREE_RED;
  parent->color = RBTREE_BLACK;
}

node_t *rbtree_insert(rbtree *t, const key_t key) {
  node_t *cur = t->root;
  node_t *new_node = malloc(sizeof(node_t));
  node_t *child, *parent, *grand, *uncle;
  
  if (t == NULL) {
    printf("tree is NULL (rbtree_insert)\n");
    return NULL;
  }

  if (new_node == NULL) {
    printf("malloc failed (rbtree_insert)\n");
    return t->root;
  }

  new_node->color = RBTREE_RED;
  new_node->key = key;
  new_node->parent = t->nil;
  new_node->left = new_node->right = t->nil;

  if (cur == t->nil) { // Case: Empty tree
    t->root = new_node;
    t->root->color = RBTREE_BLACK;
    return t->root;
  }

  while (1) {
    if (key < cur->key) {
      if (cur->left == t->nil) {
        cur->left = new_node;
        new_node->parent = cur;
        break;
      } else cur = cur->left;
    } else {
      if (cur->right == t->nil) {
        cur->right = new_node;
        new_node->parent = cur;
        break;
      } else cur = cur->right;
    }
  }

  child = new_node;
  parent = child->parent;
  grand = parent->parent;
  uncle = grand->left == parent ? grand->right : grand->left;
  while (parent->color == RBTREE_RED) {
    if (uncle->color == RBTREE_RED) { // Color Swap > Propagation
      parent->color = uncle->color = RBTREE_BLACK;
      parent->parent->color = RBTREE_RED;

      child = grand;
      parent = child->parent;
      grand = parent->parent;
      uncle = grand->left == parent ? grand->right : grand->left;
      if (child == t->root) child->color = RBTREE_BLACK;
    } else if (grand->left == parent && parent->left == child) { // Rotate > Break
      insert_LL(t, child, parent, grand);
      break;
    } else if (grand->left == parent && parent->right == child) {
      insert_LR(t, child, parent, grand);
      break;
    } else if (grand->right == parent && parent->left == child) {
      insert_RL(t, child, parent, grand);
      break;
    } else if (grand->right == parent && parent->right == child) {
      insert_RR(t, child, parent, grand);
      break;
    } else {
      printf("Edge Error (rbtree_insert)\n");
      return t->root;
    }
  }

  return t->root;
}

node_t *rbtree_find(const rbtree *t, const key_t key) {
  // TODO: implement find
  return t->root;
}

node_t *rbtree_min(const rbtree *t) {
  // TODO: implement find
  return t->root;
}

node_t *rbtree_max(const rbtree *t) {
  // TODO: implement find
  return t->root;
}

int rbtree_erase(rbtree *t, node_t *p) {
  // TODO: implement erase
  return 0;
}

int rbtree_to_array(const rbtree *t, key_t *arr, const size_t n) {
  // TODO: implement to_array
  return 0;
}
