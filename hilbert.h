typedef struct {
  float x, y;
} point;

typedef struct node {
  struct node *c[4];
  int level;
  int type;
  int order;
  int x, y;
} node;

extern char C[4];
extern node *generate(int depth);
extern void release(node *n);
extern void makepoints(node *n, int *a, point *p, int d);
extern int ipow(int n, int e);
extern void print(char *a, int *o, node *n, int depth);
