#include <X11/Xlib.h>
#include <assert.h>
#include <unistd.h>
#include <stdio.h>

int main() {
  Display* dpy = XOpenDisplay(0);
  assert(dpy);

  int whiteColor = WhitePixel(dpy, DefaultScreen(dpy));
  int width = 300, height = 200;

  Window win = XCreateSimpleWindow(dpy, DefaultRootWindow(dpy), 0, 0,
                                 width, height, 0, 0, whiteColor);

  XSelectInput(dpy, win, StructureNotifyMask|ButtonPressMask|ExposureMask);
  XMapWindow(dpy, win);

  Atom wmDeleteMessage = XInternAtom(dpy, "WM_DELETE_WINDOW", False);
  XSetWMProtocols(dpy, win, &wmDeleteMessage, 1);

  XEvent e;
  bool running = True;

  while(running) {
    XNextEvent(dpy, &e);

    switch(e.type) {
    case Expose:
      {
        // drawing here
      } break;
    case ConfigureNotify:
      {
        if (width != e.xconfigure.width || height != e.xconfigure.height) {
          width = e.xconfigure.width;
          height = e.xconfigure.height;
          XClearWindow(dpy, e.xany.window);
          printf("Size has changed: %d %d\n", width, height);
        }
      } break;
    case ClientMessage:
      {
        if (e.xclient.data.l[0] == wmDeleteMessage) {
          running = False;
        }
      } break;
    case ButtonPress:
      {
        //      XCloseDisplay(dpy);
        //      return 0;
      } break;
    default:
      {}
    }
  }

  XSync(dpy, False);
}
