#include "rbtree.h"

#include <stdlib.h>
#include <stdio.h>

// Help functions
void delete_subtree(node_t *, node_t *);
void rotate_L(rbtree *, node_t *);
void rotate_R(rbtree *, node_t *);
void insert_LL(rbtree *, node_t *, node_t *, node_t *);
void insert_LR(rbtree *, node_t *, node_t *, node_t *);
void insert_RL(rbtree *, node_t *, node_t *, node_t *);
void insert_RR(rbtree *, node_t *, node_t *, node_t *);
int erase_node(rbtree *, node_t *, node_t *, node_t *);

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

void delete_subtree(node_t *p, node_t *nil) {
  if (p->left != nil) delete_subtree(p->left, nil);
  if (p->right != nil) delete_subtree(p->right, nil);
  free(p);
  return;
}

void delete_rbtree(rbtree *t) {
  if (t->root != t->nil) delete_subtree(t->root, t->nil);
  free(t->nil);
  free(t);
}

void rotate_L(rbtree *t, node_t *node) {
  node_t *right = node->right;
  // Validation
  if (t == NULL) {
    printf("tree is NULL (rotate_L)\n");
    return;
  }

  if (node == NULL) {
    printf("node is NULL (rotate_L)\n");
    return;
  }

  if (right == t->nil) {
    printf("right is nil (rotate_L)\n");
    return;
  }

  // Connect node->parent <--> right 
  if (node == t->root) {
    t->root = right;
  } else {
    if (node->parent->left == node) node->parent->left = right;
    else node->parent->right = right;
  }
  right->parent = node->parent;

  // Connect node->right <--> right->left
  if (right->left != t->nil) right->left->parent = node;
  node->right = right->left;

  // Adjust node <--> right
  node->parent = right;
  right->left = node;
}

void rotate_R(rbtree *t, node_t *node) {
  node_t *left = node->left;
  // Validation
  if (t == NULL) {
    printf("tree is NULL (rotate_R)\n");
    return;
  }

  if (node == NULL) {
    printf("node is NULL (rotate_R)\n");
    return;
  }

  if (left == t->nil) {
    printf("left is nil (rotate_R)\n");
    return;
  }

  // Connect node->parent <--> left 
  if (node == t->root) {
    t->root = left;
  } else {
    if (node->parent->left == node) node->parent->left = left;
    else node->parent->right = left;
  }
  left->parent = node->parent;

  // Connect node->left <--> left->right
  if (left->right != t->nil) left->right->parent = node;
  node->left = left->right;

  // Adjust node <--> left
  node->parent = left;
  left->right = node;
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
  node_t *cur;
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

  cur = t->root;
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
  node_t *cur;

  if (t == NULL) {
    printf("tree is NULL (rbtree_find)\n");
    return NULL;
  }

  cur = t->root;
  if (cur == t->nil) return NULL;

  while (key != cur->key) {
    // printf("current key = %d\n", cur->key);
    if (key < cur->key) {
      if (cur->left == t->nil) return NULL;
      cur = cur->left;
    } else {
      if (cur->right == t->nil) return NULL;
      cur = cur->right;
    }
  }

  return cur;
}

node_t *rbtree_min(const rbtree *t) {
  node_t *cur;

  if (t == NULL) {
    printf("tree is NULL (rbtree_min)\n");
    return NULL;
  }

  cur = t->root;
  if (cur == t->nil) return NULL;

  while (cur->left != t->nil) cur = cur->left;

  return cur;
}

node_t *rbtree_max(const rbtree *t) {
  node_t *cur;

  if (t == NULL) {
    printf("tree is NULL (rbtree_max)\n");
    return NULL;
  }

  cur = t->root;
  if (cur == t->nil) return NULL;

  while (cur->right != t->nil) cur = cur->right;

  return cur;
}

int erase_node(rbtree *t, node_t *child, node_t *parent, node_t *sibling) {
  node_t *victim = child, *tmp;

  // Input Validation
  if (child->left != t->nil || child->right != t->nil) {
    printf("victim is not leaf (erase_node)\n");
    return 1;
  }

  // Base Case: Root
  if (child == t->root) {
    t->root = t->nil;
    free(victim);
    return 0;
  }

  // Case: RED
  if (child->color == RBTREE_RED) {
    if (parent->right == child) parent->right = t->nil;
    else parent->left = t->nil;
    free(victim);
    return 0;
  }

  while (1) {
    // Case: BLACK - Sibling: RED
    if (sibling->color == RBTREE_RED) {
      parent->color = RBTREE_RED;
      sibling->color = RBTREE_BLACK;
      if (parent == t->root) {
        t->root = sibling;
      } else {
        if (parent->parent->left == parent) parent->parent->left = sibling;
        else parent->parent->right = sibling;
      }
      sibling->parent = parent->parent;

      if (parent->left == sibling) {
        if (sibling->right != t->nil) sibling->right->parent = parent;
        parent->left = sibling->right;

        parent->parent = sibling;
        sibling->right = parent;

        sibling = parent->left;
      } else {
        if (sibling->left != t->nil) sibling->left->parent = parent;
        parent->right = sibling->left;

        parent->parent = sibling;
        sibling->left = parent;

        sibling = parent->right;
      }
    }
    // Case: BLACK - Sibling: BLACK - Nephew: ALL BLACK
    else if (sibling->left->color == RBTREE_BLACK && sibling->right->color == RBTREE_BLACK) {
      sibling->color = RBTREE_RED;
      if (parent->color == RBTREE_RED || parent == t->root) {
        parent->color = RBTREE_BLACK;
        // Escape Loop
        break;
      } else {
        child = child->parent;
        parent = parent->parent;
        sibling = parent->left == child ? parent->right : parent->left;
      }
    }
    // Case: BLACK - Sibling: BLACK - Nephew: LL RED 
    else if (parent->left == sibling && sibling->left->color == RBTREE_RED) {
      sibling->color = parent->color;
      parent->color = RBTREE_BLACK;
      sibling->left->color = RBTREE_BLACK;

      if (parent == t->root) t->root = sibling;
      else {
        if (parent->parent->left == parent) parent->parent->left = sibling;
        else parent->parent->right = sibling;
      }
      sibling->parent = parent->parent;

      if (sibling->right != t->nil) sibling->right->parent = parent;
      parent->left = sibling->right;

      sibling->right = parent;
      parent->parent = sibling;
      // Escape Loop
      break;
    }
    // Case: BLACK - Sibling: BLACK - Nephew: RR RED
    else if (parent->right == sibling && sibling->right->color == RBTREE_RED) {
      sibling->color = parent->color;
      parent->color = RBTREE_BLACK;
      sibling->right->color = RBTREE_BLACK;

      if (parent == t->root) t->root = sibling;
      else {
        if (parent->parent->left == parent) parent->parent->left = sibling;
        else parent->parent->right = sibling;
      }
      sibling->parent = parent->parent;

      if (sibling->left != t->nil) sibling->left->parent = parent;
      parent->right = sibling->left;

      sibling->left = parent;
      parent->parent = sibling;
      // Escape Loop
      break;
    }
    // Case: BLACK - Sibling: BLACK - Nephew: Only LR RED
    else if (parent->left == sibling && sibling->right->color == RBTREE_RED) {
      sibling->color = RBTREE_RED;
      sibling->right->color = RBTREE_BLACK;

      tmp = sibling->right;
      rotate_L(t, sibling);
      sibling = tmp;

      sibling->color = parent->color;
      parent->color = sibling->left->color = RBTREE_BLACK;

      rotate_R(t, parent);
      // Escape Loop
      break;
    } // Case: BLACK - Sibling: BLACK - Nephew: Only RL RED
    else if (parent->right == sibling && sibling->left->color == RBTREE_RED) {
      sibling->color = RBTREE_RED;
      sibling->left->color = RBTREE_BLACK;

      tmp = sibling->left;
      rotate_R(t, sibling);
      sibling = tmp;

      sibling->color = parent->color;
      parent->color = sibling->right->color = RBTREE_BLACK;

      rotate_L(t, parent);
      //Escape Loop
      break;
    }
  }

  if (victim->parent->right == victim) victim->parent->right = t->nil;
  else victim->parent->left = t->nil;
  free(victim);

  return 0;
}

int rbtree_erase(rbtree *t, node_t *p) {
  node_t *cur, *swap, *child, *parent, *sibling;

  // Input Validation
  if (t == NULL) {
    printf("tree is NULL (rbtree_erase)\n");
    return 1;
  }

  if (p == NULL) {
    printf("target is NULL (rbtree_erase)\n");
    return 1;
  }

  cur = t->root;
  while (cur != p) {
    if (cur == t->nil) {
      printf("target is not in tree (rbtree_erase)\n");
      return 1;
    }
    if (p->key < cur->key) cur = cur->left;
    else cur = cur->right;
  }

  // Sink
  while (cur->left != t->nil || cur->right != t->nil) {
    swap = cur;
    if (cur->right == t->nil) swap = cur->left;
    else {
      swap = cur->right;
      while (swap->left != t->nil) {
        swap = swap->left;
      }
    }
    key_t tmp = cur->key;
    cur->key = swap->key;
    swap->key = tmp;

    cur = swap;
  }

  child = cur;
  parent = child->parent;
  sibling = parent->left == child ? parent->right : parent->left;

  return erase_node(t, child, parent, sibling);
}

void in_order(const rbtree *t, key_t *arr, node_t *p, int *idx_p, int depth) {
  // for (int i = 0; i < depth; i++) {
  //   printf("|  ");
  // }
  // printf("+--");
  // for (int i = 0; i < 3 * (4 - depth); i++) {
  //   printf(" ");
  // }
  // printf("parent: %5d | key: %5d | color: %2c | left: %5d | right: %5d\n", p->parent->key, p->key, p->color == RBTREE_BLACK ? 'B' : 'R', p->left->key, p->right->key);

  if (p->left != t->nil) in_order(t, arr, p->left, idx_p, depth + 1);
  *(arr + (*idx_p)) = p->key;
  (*idx_p)++;
  if (p->right != t->nil) in_order(t, arr, p->right, idx_p, depth + 1);
}

int rbtree_to_array(const rbtree *t, key_t *arr, const size_t n) {
  int idx = 0;

  if (t == NULL) {
    printf("tree is null (rbtree_to_array)\n");
    return 1;
  }

  if (arr == NULL) {
    printf("arr is null (rbtree_to_array)\n");
    return 1;
  }

  if (t->root != t->nil) in_order(t, arr, t->root, &idx, 0);

  if (idx != n) {
    printf("Index Out of Range (rbtree_to_array)\n");
    return 1;
  }

  return 0;
}
