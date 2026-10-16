/* Launch applications with simple geometrical forms */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>

#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/extensions/Xrender.h>

#include "gesture_recogniser.h"

#define VERSION "0.0"
#define HELP "This is help for mmenu command sorry but im to lazy to write rn!"
#define MAX_POINTS 1000
#define MAX_TEMPLATES 100

/* Learn  -------------- */
int
learn (char *name, char *command, char *file_path, char *file_dir) {

  /* open file */
  FILE *file = fopen(file_path, "a");

  if (file == NULL) {
    mkdir(file_dir, 0755);
    file = fopen(file_path, "a");
  }


  fprintf(file, "\n");
  fprintf(file, "%s\n%s\n", name,command);

  /* Open window */
  Display *dpy = XOpenDisplay(NULL);

  if (!dpy) {
    fprintf(stderr, "X failed to open display.\n");
  }

  int screen = XDefaultScreen(dpy);

  Window root = XDefaultRootWindow(dpy);

  unsigned long black = XBlackPixel(dpy, screen);
  unsigned long white = XWhitePixel(dpy, screen);

  int width = 500;
  int height = 500;

  Window w = XCreateSimpleWindow(dpy, root, 0, 0, width, height, 2, white, black);

  GC gc = XCreateGC(dpy,w, 0, NULL);
  XSetForeground(dpy, gc, white);
  XSetLineAttributes(dpy, gc, 5, LineSolid, CapRound, JoinRound);

  XSelectInput(dpy, w,
               ExposureMask | KeyPressMask | KeyReleaseMask | ButtonPressMask |
               ButtonReleaseMask | StructureNotifyMask | PointerMotionMask);

  XMapWindow(dpy, w);
  XFlush(dpy);

  XSizeHints hints;
  hints.flags = PMinSize | PMaxSize;
  hints.min_width = width;
  hints.max_width = width;
  hints.min_height = height;
  hints.max_height = height;
  XSetWMNormalHints(dpy, w, &hints);


  /* Main loop */
  int running = 1;

  int x,lastx;
  int y,lasty;
  int button_state = 0;

  while (running <= 1 && running != 0) {
    XEvent event;
    XNextEvent(dpy, &event);

    switch (event.type) {
    case Expose: {
      XFlush(dpy);
      break;
    }

    case KeyPress: {
      KeySym keysym = XLookupKeysym(&event.xkey, 0);
      if (keysym == XK_q) {
        running = 0;
      }
      break;
    }

    case MotionNotify: {
      x = event.xmotion.x;
      y = event.xmotion.y;

      if (button_state) {
        fprintf(file, "%d\n%d\n", x,y);

        XDrawLine(dpy, w, gc, lastx, lasty, x, y);
      }

      lastx = x;
      lasty = y;
      break;
    }

    case ButtonPress: {
      button_state = 1;
      if (event.xbutton.button == Button2) {
        running = 0;
      }
      break;
    }

    case ButtonRelease: {
      fprintf(file, "\n");
      button_state = 0;
      running++;
      XClearWindow(dpy, w);
      break;
    }
    }

  }

  fclose(file);
  return 0;
}


/* Main ------------------------------- */
int
main(int argc, char *argv[]) {
  /* Parce options */
  int width;
  int height;

  /* File ---------------------- */

  const char *home = getenv("HOME");
  if (home == NULL) {
    fprintf(stderr, "HOME is not set\n");
    return 1;
  }

  char file_path[100];
  char file_dir[100];

  snprintf(file_path,sizeof file_path, "%s/.config/mmenu/templates.txt", home);
  snprintf(file_dir,sizeof file_dir, "%s/.config/mmenu/", home);


  FILE *file = fopen(file_path, "r");
  /* create dir if it doesnt exits */
  if (file == NULL) {
    mkdir(file_dir, 0755);
    file = fopen(file_path, "r");
  }

  /* Parce file */
  char s[1024];

  Template templates[MAX_TEMPLATES];
  int template_count = 0;
  while(fgets(s,sizeof s , file) != NULL){
    templates[template_count].label = s;

  }
  fclose(file);

  /* Mods var */
  int windowed_mode = 0;

  for (int i = 1; i < argc; i++) {
    /* Take no argument */
    if (!strcmp("-v", argv[i])) {
      printf("mmenu-" VERSION "\n");
      return 0;
    } else if (!strcmp("-h", argv[i])) {
      printf(HELP "\n");
      return 0;
    }
    /* Take two argument */
    else if (!strcmp("learn", argv[i])) {
      if (argc > 3) {
        learn(argv[i + 1], argv[i + 2], file_path, file_dir);
        return 0;
      } else {
        printf("mmenu learn <name> <command>\n");
        return 2;
      }
    } else if (!strcmp("-w", argv[i])) { // windowed mode
      windowed_mode = 1;
      if (argc > 3) {
        width = atoi(argv[i + 1]);
        height = atoi(argv[i + 2]);
      } else {
        width = 500;
        height = 500;
      }

    }
  }

  /* Open window ------------------------------------*/
  Display *dpy = XOpenDisplay(NULL);
  if (!dpy) {
    fprintf(stderr, "X failed to open display.\n");
  }

  int screen = XDefaultScreen(dpy);

  Window root = XDefaultRootWindow(dpy);

  unsigned long black = XBlackPixel(dpy, screen);
  unsigned long white = XWhitePixel(dpy, screen);

  XWindowAttributes attributes;
  XGetWindowAttributes(dpy, root, &attributes);

  if (!windowed_mode){
    width = attributes.width;
    height = attributes.height;
  }


  XVisualInfo vi;
  if (!XMatchVisualInfo(dpy, screen, 32, TrueColor, &vi)) {
    fprintf(stderr, "No 32-bit TrueColor visual found\n");
    return 1;
  }

  XRenderPictFormat *fmt = XRenderFindVisualFormat(dpy, vi.visual);
  if (!fmt || fmt->direct.alphaMask == 0) {
    fprintf(stderr, "That visual has no alpha channel\n");
    return 1;
  }

  Colormap cmap = XCreateColormap(dpy, root, vi.visual, AllocNone);

  XSetWindowAttributes attr = {0};
  attr.colormap = cmap;
  attr.background_pixel = 0; /* transparent */
  attr.border_pixel = 0;
  attr.event_mask =
      ExposureMask | ButtonPressMask | ButtonReleaseMask | PointerMotionMask;
  attr.override_redirect = True;

  Window w;
  if (windowed_mode) {
    w = XCreateSimpleWindow(dpy, root, 0, 0, width, height, 2, white, black);
  } else {
    w = XCreateWindow(dpy, root, 0, 0, width, height, 0, vi.depth, InputOutput,
                      vi.visual,
                      CWColormap | CWBackPixel | CWBorderPixel | CWEventMask |
                      CWOverrideRedirect,
                      &attr);
  }

  XSelectInput(dpy, w,
               ExposureMask | KeyPressMask | KeyReleaseMask | ButtonPressMask |
                   ButtonReleaseMask | StructureNotifyMask | PointerMotionMask);

  XMapWindow(dpy, w);
  XLowerWindow(dpy, w);
  XFlush(dpy);

  GC gc = XCreateGC(dpy, w, 0, NULL);
  XSetForeground(dpy, gc, white);
  XSetLineAttributes(dpy, gc, 5, LineSolid, CapRound, JoinRound);

  /* Make window float even in tiling wms */
  XSizeHints hints;
  hints.flags = PMinSize | PMaxSize;
  hints.min_width = width;
  hints.max_width = width;
  hints.min_height = height;
  hints.max_height = height;
  XSetWMNormalHints(dpy, w, &hints);



  /* Gesture Detection ----------------------------------------- */
  Point points[MAX_POINTS];
  int amount = 0;

  /* Main loop -------------------------------------------------*/
  int running = 1;

  int lastx, x;
  int lasty, y;

  int button_state = 0;
  while (running) {
    XEvent event;
    XNextEvent(dpy, &event);

    switch (event.type) {
    case Expose: {
      XFlush(dpy);
      break;
    }

    case KeyPress: {
      KeySym keysym = XLookupKeysym(&event.xkey, 0);
      if (keysym == XK_q) {
        running = 0;
      }
      break;
    }

    case MotionNotify: {
      x = event.xmotion.x;
      y = event.xmotion.y;

      if (button_state) {
        points[amount].x = x;
        points[amount].y = y;

        amount++;
        XDrawLine(dpy, w, gc, lastx, lasty, x, y);
      }

      lastx = x;
      lasty = y;
      break;
    }

    case ButtonPress: {
      button_state = 1;
      if (event.xbutton.button == Button2) {
        running = 0;
      }
      break;
    }

    case ButtonRelease: {

      button_state = 0;
      XClearWindow(dpy, w);
      break;
    }
    }
  }
  XClearWindow(dpy, w);
  XFreeGC(dpy, gc);
  XDestroyWindow(dpy, w);
  XCloseDisplay(dpy);
  return 0;
}
