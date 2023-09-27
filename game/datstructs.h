#ifndef DATSTRUCTS_H
#define DATSTRUCTS_H

#include <stdlib.h>
#include <string.h>
#include <stdbool.h>

typedef struct UQueue UQueue;
typedef struct _BBranch _BBranch;
typedef struct BTree BTree;

struct UQueue {
  unsigned int max;
  unsigned int first;
  unsigned int len;
  unsigned int size;
  unsigned char *data;
};

struct _BBranch {
  _BBranch *left;
  _BBranch *right;
  float key;
  void *value;
};
struct BTree {
  _BBranch *root;
  unsigned int size;
};

UQueue uqueue_create(unsigned int max, unsigned int size); //Create a queue.
void uqueue_destroy(UQueue *q); //Free queue memory.
bool uqueue_push(UQueue *q, void *v); //Push a value into the queue.
bool uqueue_pop(UQueue *q, void *v); //Obtain the next value from the queue.
void uqueue_reset(UQueue *q); //Clear the queue.
void uqueue_shift(UQueue *q); //Shift the queue's memory to the start.
void uqueue_restore(UQueue *q); //Set the iterator to 0 to "undo" pops.
UQueue uqueue_copy(UQueue *q); //Deep copy of a queue.

_BBranch *_bbranch_destroy(_BBranch *b);
_BBranch *_bbranch_push(_BBranch *b, float k, void *v, unsigned int size);
_BBranch *_bbranch_pop_low(_BBranch *b, void *v, unsigned int size);
_BBranch *_bbranch_pop_high(_BBranch *b, void *v, unsigned int size);
bool _bbranch_contains(_BBranch *b, float k, void *v, unsigned int size);
BTree btree_create(unsigned int size); //Create a binary tree.
void btree_destroy(BTree *t); //Free tree memory.
void btree_push(BTree *t, float k, void *v); //Push a (key, value) into the tree.
bool btree_pop_low(BTree *t, void *v); //Remove the lowest value and return it.
bool btree_pop_high(BTree *t, void *v); //Remove the highest value and return it.
bool btree_contains(BTree *t, float k, void *v); //Check if tree contains (key, value).

UQueue uqueue_create(unsigned int max, unsigned int size) {
  UQueue q;
  q.max = max;
  q.first = 0;
  q.len = 0;
  q.size = size;
  q.data = malloc(max * size);
  return q;
}

void uqueue_destroy(UQueue *q) {
  free(q->data);
}

bool uqueue_push(UQueue *q, void *v) {
  if (q->len == q->max)
    return false;
  for (unsigned int i = 0; i < q->len; i++)
    if (memcmp(q->data + (i * q->size), v, q->size) == 0)
      return true;
  memcpy(q->data + q->len * q->size, v, q->size);
  q->len++;
  return true;
}

bool uqueue_pop(UQueue *q, void *v) {
  if (q->first == q->len)
    return false;
  memcpy(v, q->data + q->first * q->size, q->size);
  q->first++;
  return true;
}

void uqueue_reset(UQueue *q) {
  q->first = 0;
  q->len = 0;
}

void uqueue_shift(UQueue *q) {
  memmove(q->data, q->data + q->first * q->size, q->len * q->size);
  q->len -= q->first;
  q->first = 0;
}

void uqueue_restore(UQueue *q) {
  q->first = 0;
}

UQueue uqueue_copy(UQueue *q) {
  UQueue new_q = *q;
  new_q.data = malloc(q->max * q->size);
  memcpy(new_q.data + q->first * q->size, q->data + q->first * q->size, (q->len - q->first) * q->size);
  return new_q;
}

_BBranch *_bbranch_destroy(_BBranch *b) {
  if (!b)
    return 0;
  if (b->left)
    b->left = _bbranch_destroy(b->left);
  if (b->right)
    b->right = _bbranch_destroy(b->right);
  if (b->value)
    free(b->value);
  free(b);
  return 0;
}

_BBranch *_bbranch_push(_BBranch *b, float k, void *v, unsigned int size) {
  if (!b) {
    b = malloc(sizeof(_BBranch));
    b->left = 0;
    b->right = 0;
    b->key = k;
    b->value = malloc(size);
    memcpy(b->value, v, size);
  }
  else if (k < b->key)
    b->left = _bbranch_push(b->left, k, v, size);
  else
    b->right = _bbranch_push(b->right, k, v, size);
  return b;
}

_BBranch *_bbranch_pop_low(_BBranch *b, void *v, unsigned int size) {
  if (!b->left) {
    memcpy(v, b->value, size);
    _BBranch *new_b = b->right;
    b->right = 0;
    b = _bbranch_destroy(b);
    return new_b;
  }
  b->left = _bbranch_pop_low(b->left, v, size);
  return b;
}

_BBranch *_bbranch_pop_high(_BBranch *b, void *v, unsigned int size) {
  if (!b->right) {
    memcpy(v, b->value, size);
    _BBranch *new_b = b->left;
    b->left = 0;
    b = _bbranch_destroy(b);
    return new_b;
  }
  b->right = _bbranch_pop_high(b->right, v, size);
  return b;
}

bool _bbranch_contains(_BBranch *b, float k, void *v, unsigned int size) {
  if (!b)
    return false;
  if (k < b->key)
    return _bbranch_contains(b->left, k, v, size);
  if (k > b->key)
    return _bbranch_contains(b->right, k, v, size);
  if (memcmp(b->value, v, size) == 0)
    return true;
  return _bbranch_contains(b->right, k, v, size);
}

BTree btree_create(unsigned int size) {
  BTree t;
  t.root = 0;
  t.size = size;
  return t;
}

void btree_destroy(BTree *t) {
  _bbranch_destroy(t->root);
}

void btree_push(BTree *t, float k, void *v) {
  t->root = _bbranch_push(t->root, k, v, t->size);
}

bool btree_pop_low(BTree *t, void *v) {
  if (!t->root)
    return false;
  t->root = _bbranch_pop_low(t->root, v, t->size);
  return true;
}

bool btree_pop_high(BTree *t, void *v) {
  if (!t->root)
    return false;
  t->root = _bbranch_pop_high(t->root, v, t->size);
  return true;
}

bool btree_contains(BTree *t, float k, void *v) {
  return _bbranch_contains(t->root, k, v, t->size);
}

#endif
