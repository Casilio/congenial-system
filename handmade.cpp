#include <X11/Xlib.h>
#include <assert.h>
#include <unistd.h>
#include <stdio.h>

int main() {
  Display* dpy = XOpenDisplay(0);
  assert(dpy);

  int whiteColor = BlackPixel(dpy, DefaultScreen(dpy));
  int border_color, background_color;
  Colormap cmap = DefaultColormap(dpy, DefaultScreen(dpy));
  XColor xc1, xc2;

  XAllocNamedColor(dpy, cmap, "DarkGreen", &xc1, &xc2);
  background_color = xc1.pixel;
  XAllocNamedColor(dpy, cmap, "LightGreen", &xc1, &xc2);
  border_color = xc1.pixel;
  XAllocNamedColor(dpy, cmap, "Red", &xc1, &xc2);


  int width = 300, height = 200;

  Window win = XCreateSimpleWindow(dpy, DefaultRootWindow(dpy), 0, 0,
                                 width, height, 10, border_color, background_color);

  XSelectInput(dpy, win, StructureNotifyMask|ButtonPressMask|ExposureMask);
  XMapWindow(dpy, win);

  GC gc = XCreateGC(dpy, win, 0, 0);
  XSetForeground(dpy, gc, whiteColor);

  GC pen;
  XGCValues values;

  values.foreground = xc1.pixel;
  values.line_width = 3;
  values.line_style = LineSolid;

  pen = XCreateGC(dpy, win, GCForeground|GCLineWidth|GCLineStyle, &values);

  XEvent e;
  while(True) {
    XNextEvent(dpy, &e);
    switch(e.type) {
    case Expose:
      XDrawLine(dpy, win, pen, 0, 0, width, height);
      XDrawLine(dpy, win, pen, width, 0, 0, height);
      break;
    case ConfigureNotify:
      if (width != e.xconfigure.width || height != e.xconfigure.height) {
        width = e.xconfigure.width;
        height = e.xconfigure.height;
        XClearWindow(dpy, e.xany.window);
        printf("Size has changed: %d %d\n", width, height);
      }
      break;
    case ButtonPress:
      printf("");
//      XCloseDisplay(dpy);
//      return 0;
    }
  }
}
