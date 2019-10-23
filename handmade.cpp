#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <assert.h>
#include <stdio.h>
#include <X11/extensions/Xcomposite.h>
#include <time.h>
#include <stdint.h>

Pixmap pixmap;
GC gc;

int main() {
  Display* dpy = XOpenDisplay(0);
  assert(dpy);

  int screen_num = DefaultScreen(dpy);
  int whiteColor = WhitePixel(dpy, screen_num);
  int width = 300, height = 200;

  int depth = DefaultDepth(dpy, screen_num);
  XSetWindowAttributes attrs;
  attrs.border_pixel = BlackPixel(dpy, screen_num);
  attrs.background_pixel = WhitePixel(dpy, screen_num);
  attrs.event_mask = StructureNotifyMask|ButtonPressMask|ExposureMask;

  Window win = XCreateWindow(dpy, RootWindow(dpy, screen_num), 10, 10, 150, 100, 0, depth, InputOutput,
                             DefaultVisual(dpy, screen_num), CWBackPixel | CWColormap | CWBorderPixel, &attrs);

  XSync(dpy, True);
  XMapWindow(dpy, win);

  Atom wmDeleteMessage = XInternAtom(dpy, "WM_DELETE_WINDOW", False);
  XSetWMProtocols(dpy, win, &wmDeleteMessage, 1);

  XEvent event;
  bool running = True;

  pixmap = XCreatePixmap(dpy, win, width, height, depth);
  gc = XCreateGC(dpy, pixmap, 0, NULL);

  struct timespec timer = {0, 999999 / 30};

  while(running) {
    while(XPending(dpy) > 0) {
      XNextEvent(dpy, &event);

      switch(event.type) {
      case Expose:
        {
          // drawing here
        } break;
      case ConfigureNotify:
        {
          if (width != event.xconfigure.width || height != event.xconfigure.height) {
            width = event.xconfigure.width;
            height = event.xconfigure.height;
            XClearWindow(dpy, event.xany.window);
            printf("Size has changed: %d %d\n", width, height);
          }
        } break;
      case ClientMessage:
        {
          if (event.xclient.data.l[0] == wmDeleteMessage) {
            running = False;
          }
        } break;
      case ButtonPress:
        {
        } break;
      }
    } // events are handled

    //    int Pitch = width * depth / sizeof(int);
    //    uint8_t *Row = (uint8_t *)pixmap;
    //
    //    for (int y = 0; y < height; y++)
    //      {
    //        uint32_t *Pixel = (uint32_t *)Row;
    //        for (int x = 0; x < width; x++)
    //          {
    //            *Pixel++ = (0xff);
    //          }
    //        Row += Pitch;
    //      }

    XCopyArea(dpy, pixmap, win, gc,
              0, 0,
              width, height,
              0, 0);

    nanosleep(&timer, NULL);
  }
  XFreePixmap(dpy, pixmap);
  XCloseDisplay(dpy);
  XSync(dpy, False);
}
