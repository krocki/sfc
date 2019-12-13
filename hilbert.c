#include <stdio.h>
#include <stdlib.h>
#include "hilbert.h"

#define H 0
#define A 1
#define B 2
#define R 3

int rules[4][4] =
  {
    { H, H, A, B },
    { R, A, H, A },
    { B, R, B, H },
    { A, B, R, R }
  };

int order[4][4] =
  {
    { 1, 2, 0, 3 },
    { 3, 2, 0, 1 },
    { 1, 0, 2, 3 },
    { 3, 0, 2, 1 }
  };

node *alloc()
{
  return calloc(1, sizeof(node));
}

int ipow(int n, int e)
{
  int i=1;
  for (int j=0; j<e; j++)
    i *= n;
  return i;
}

void print(char *a, int *o, node *n, int depth)
{
  if (0 != n) {
    for (int i=0; i<4; i++) print(a, o, n->c[i], depth);
    if ((depth-1)==n->level) {
      o[n->y * ipow(2, n->level) + n->x] = n->order;
    }
  }
}

void expand(node *n, int depth)
{
  if (0 != n && depth > 1) {
    for (int i=0; i<4; i++) {
      n->c[i] = alloc();
      n->c[i]->level = n->level+1;
      n->c[i]->type = rules[n->type][i];
      n->c[i]->order = (n->order << 2) |
        (0x3 & order[n->type][i]);
      n->c[i]->x = n->x + ipow(2, depth-2) * (i%2);
      n->c[i]->y = n->y + ipow(2, depth-2) * (i/2);
      expand(n->c[i], depth-1);
    }
  }
}

void release(node *n)
{
  if (0 != n) {
    for (int i=0; i<4; i++) release(n->c[i]);
    free(n);
  }
}

node *generate(int depth)
{

  node *r = alloc();
  expand(r, depth);
  return r;
}

void makepoints(node *n, int *o, point *p, int d)
{

  int num = ipow(2, d-1);
  char *a = malloc(num * num * sizeof(char));
  print(a, o, n, d);

  for (int i=0; i<num; i++) {
    for (int j=0; j<num; j++) {
      for (int k=(d-1)*2-1; k>=0; k--) {
        p[o[j+num*i]] =
        (point)
        {
          ((float)j+0.5f) / (float)num,
          ((float)i+0.5f) / (float)num
        };
      }
    }
  }
  free(a);
}
