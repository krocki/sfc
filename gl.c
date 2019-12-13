#include <sys/time.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <math.h>
#include "hilbert.h"

#define GL_SILENCE_DEPRECATION 1

typedef uint8_t u8;
typedef uint16_t u16;
typedef uint32_t u32;

point *p;
int *o;

int num;
int source;
int sink;

void *work(void *args);

#define NOGL 0

#if NOGL
#else
#include <pthread.h>
#include <GLFW/glfw3.h>
#define AUTO_REFRESH 60
#define OFFSET 64
#define IM_W 512
#define IM_H 512

static GLFWwindow* window;
u8 gl_ok=0;
u8 flip_y=1;
u8 grid=1;
u8 new_frame=1;
u8 limit_speed=0;
u32 cyclelimit = 0;
u8 debugmode = 0;
u8 show_close = 0;
u8 show_debug = 0;
u8 pix[3*IM_W*IM_H];

int WIDTH = 512;
int HEIGHT = 512;

int mx=-1;
int my=-1;

double t0; // global start time
double get_time() {
  struct timeval tv; gettimeofday(&tv, NULL);
  return (tv.tv_sec + tv.tv_usec * 1e-6);
}

int nsleep(long ms) {
  struct timespec req, rem;
  if (ms > 999) {
    req.tv_sec = (int)(ms/1000);
    req.tv_nsec = (ms - ((long)req.tv_sec*1000)) * 1000000;
  }
  else {
    req.tv_sec=0;
    req.tv_nsec=ms*1000000;
  }
  return nanosleep(&req, &rem);
}

#define bind_key(x,y) \
{ if (action == GLFW_PRESS && key == (x)) (y) = 1; if (action == GLFW_RELEASE && key == (x)) (y) = 0; if (y) {printf(#y "\n");} }

static void change_size_callback(GLFWwindow* window, int width, int height)
{
  WIDTH = width;
  HEIGHT = height;
}

static void error_callback(int error, const char* description) { }
static void key_callback(GLFWwindow* window, int key, int scancode, int action, int mods) {
  if (action == GLFW_PRESS) {
    switch (key) {
      case GLFW_KEY_ESCAPE:
        glfwSetWindowShouldClose(window, GLFW_TRUE); break;
      case GLFW_KEY_G:
        grid ^= 1; break;
      case GLFW_KEY_C:
        show_close ^= 1; break;
      default: ;
    }
  }
}

static void mouse_pos_callback(GLFWwindow* window, double x, double y) {
  int xpos, ypos, width, height;
  glfwGetWindowPos(window, &xpos, &ypos);
  glfwGetWindowSize(window, &width, &height);
  mx = (int)(((double)IM_W*x)/(double)WIDTH);
  my = (int)(-0.5+((double)IM_H*y)/(double)HEIGHT);
  printf("(%8.4f, %8.4f) : ", x/(double)WIDTH, y/(double)HEIGHT);
  printf("(%4d, %4d)\n", mx, my);
  int i = (int)(((double)x*num)/(double)WIDTH);
  int j = (int)(((double)y*num)/(double)HEIGHT);
  int idx = j*num + i;
  sink = o[idx];
}

static void mouse_btn_callback(GLFWwindow* window, int button, int action, int mods) {
  if (action == GLFW_PRESS) {
    double xpos, ypos;
    glfwGetCursorPos(window, &xpos, &ypos);
    int x = (int)(((double)IM_W*xpos)/(double)WIDTH);
    int y = (int)(-0.5+((double)IM_H*ypos)/(double)HEIGHT);
    printf("click at x=%.3f (%d), y=%.3f (%d)\n", xpos, x, ypos, y);
    int i = (int)(((double)xpos*num)/(double)WIDTH);
    int j = (int)(((double)ypos*num)/(double)HEIGHT);
    int idx = j*num + i;
    printf("square (%d, %d), o[%d] = %d, p[o[%d]] = %f, %f\n",
      j, i, idx, o[idx], idx, p[o[idx]].x, p[o[idx]].y);
    switch (button) {
      case GLFW_MOUSE_BUTTON_LEFT: source = o[idx]; break;
      //case GLFW_MOUSE_BUTTON_RIGHT: sink = o[idx]; break;
      default: ;
    }
  }
}

static GLFWwindow* open_window(const char* title, GLFWwindow* share, int posX, int posY)
{
    GLFWwindow* window;

    //glfwWindowHint(GLFW_VISIBLE, GLFW_FALSE);
    window = glfwCreateWindow(WIDTH, HEIGHT, title, NULL, share);
    if (!window) return NULL;

    glfwMakeContextCurrent(window);
    glfwSwapInterval(1);
    glfwSetWindowPos(window, posX, posY);
    glfwShowWindow(window);

    glfwSetKeyCallback(window, key_callback);
    glfwSetMouseButtonCallback(window, mouse_btn_callback);
    glfwSetCursorPosCallback(window, mouse_pos_callback);
    glfwSetWindowSizeCallback(window, change_size_callback);

    return window;
}

void draw_point(int x, int y, float r, float g, float b, float a) {

    float incr_x = 1.0f/(float)IM_W; float incr_y = 1.0f/(float)IM_H;
    glColor4f(r, g, b, a);
    float i = x * incr_x;
    float j = y * incr_y;
    j = flip_y ? 1-j : j; // FLIP vert
    glVertex2f(i,      j     );     glVertex2f(i+incr_x, j     );
    glVertex2f(i+incr_x, j+incr_y); glVertex2f(i,      j+incr_y);
}

void draw_line(int x0, int y0, int x1, int y1, float r, float g, float b, float a, float w) {

    glLineWidth(w);
    float incr_x = 1.0f/(float)IM_W; float incr_y = 1.0f/(float)IM_H;
    glColor4f(r, g, b, a);
    float i0 = x0 * incr_x;
    float j0 = y0 * incr_y;
    float i1 = x1 * incr_x;
    float j1 = y1 * incr_y;
    j0 = flip_y ? 1-j0 : j0; // FLIP vert
    j1 = flip_y ? 1-j1 : j1; // FLIP vert
    glVertex2f(i0, j0);
    glVertex2f(i1, j1);
}

static void draw_quad()
{
    int width, height;
    glfwGetFramebufferSize(glfwGetCurrentContext(), &width, &height);
    glViewport(0, 0, width, height);
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    glOrtho(0.f, 1.f, 0.f, 1.f, 0.f, 1.f);

    if (1 == grid) {
      glLineWidth(1.0f);
      glBegin(GL_LINES);

      float wf = 1.0f/((float)num);

      // horizontal
      for (int i=1; i<num; i++) {
        glColor4f(0.35f, 0.35f, 0.35f, (i%16==0)? 0.3f : 0.15f);
        glVertex2f( 0.0f, i * wf); glVertex2f( 1.0f, i * wf);
      }

      // vertical
      for (int i=1; i<num; i++) {
        glColor4f(0.35f, 0.35f, 0.35f, (i%16==0)? 0.3f : 0.15f);
        glVertex2f( i * wf, 1.0f ); glVertex2f( i * wf, 0.0f );
      }
      glEnd();
    }

    glBegin(GL_LINE_STRIP);
    glLineWidth(2.0f);

    for (int i=0; i<num*num; i++) {
      //if (i >= source && i < sink)
      //  glColor4f((float)abs(i-source) / (float)abs(source-sink), 0.3f, 0.3f, 0.7f);
      //else if (i >= sink && i < source)
      //  glColor4f(0.3f, 0.5f, (float)abs(i-source) / (float)abs(source-sink), 0.7f);
      //else
      //  {
          if (1==show_close)
            glColor4f(1.0f, 1.0f, 1.0f, 0.5f + 0.3f * (1.0f - (float)abs(i - sink) * 0.01f ));
          else
            glColor4f(1.0f, 1.0f, 1.0f, 0.5f);
      //  }
      glVertex2f(p[i].x, (flip_y ? 1.0f - p[i].y: p[i].y));
    }

    glEnd();
    glBegin(GL_QUADS);
    // cursor
    draw_point(mx, my, 1, 0, 0, 0.8);
    glEnd();
};

int display_init() {
  int x, y, width;
  t0=get_time();
  //glfwSetErrorCallback(error_callback);
  if (!glfwInit()) return -1;

  window = open_window("quad", NULL, OFFSET, OFFSET);
  if (!window) { glfwTerminate(); return -1; }

  glfwGetWindowPos(window, &x, &y);
  glfwGetWindowSize(window, &width, NULL);
  glfwMakeContextCurrent(window);
  glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_HIDDEN);

  gl_ok=1;
  printf("%9.6f, GL_init OK\n", get_time()-t0);

  glEnable ( GL_POINT_SMOOTH );
  glEnable( GL_LINE_SMOOTH );
  glEnable( GL_POLYGON_SMOOTH );
  glHint ( GL_POINT_SMOOTH_HINT, GL_NICEST );
  glHint( GL_LINE_SMOOTH_HINT, GL_NICEST );
  glHint( GL_POLYGON_SMOOTH_HINT, GL_NICEST );
  glEnable ( GL_BLEND );
  glBlendFunc ( GL_SRC_ALPHA, GL_ONE );

  double frame_start=get_time();
  while (!glfwWindowShouldClose(window))
  {
    double fps = 1.0/(get_time()-frame_start);
    char wtitle[256]; wtitle[255] = '\0';
    frame_start = get_time();
    sprintf(wtitle, "%4.1f FPS", fps);
    glfwSetWindowTitle(window, wtitle);
    glfwMakeContextCurrent(window);
    glClearColor(0.3f, 0.3f, 0.3f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT);
    draw_quad();
    glfwSwapBuffers(window);
    if (AUTO_REFRESH > 0) glfwWaitEventsTimeout(1.0/(double)AUTO_REFRESH - (get_time()-frame_start));
    else glfwWaitEvents();
  }

  glfwTerminate();
  printf("%9.6f, GL terminating\n", get_time()-t0);
  gl_ok=0;
  return 0;
}
#endif

int main(int argc, char **argv) {

  #if NOGL
    work(argv[1]);
  #else
    pthread_t work_thread;
    if(pthread_create(&work_thread, NULL, work, argv[1])) {
      fprintf(stderr, "Error creating thread\n");
      return 1;
    }

    display_init();

    if(pthread_join(work_thread, NULL)) {
      fprintf(stderr, "Error joining thread\n");
      return 2;
    }

  #endif

  return 0;
}

void *work(void *args) {

  int i = 0;
  int d = (0 == args) ? 6 : atoi(args);
  sink = 0;
  node *r = generate(d);
  num = ipow(2, d-1);
  source = (num*num)/2;

  o = malloc (num * num * sizeof(int));
  p = malloc (num * num * sizeof(point));
  makepoints(r, o, p, d);
  printf("%d points\n", num*num);

  while (!gl_ok) nsleep(10);
  {
    while (gl_ok) {
      nsleep(100);
    }

  }
  free(r); free(p);
  return 0;
}
