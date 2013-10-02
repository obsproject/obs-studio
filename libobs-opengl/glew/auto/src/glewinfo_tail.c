}

#endif /* _WIN32 */

/* ------------------------------------------------------------------------ */

#if defined(_WIN32) || !defined(__APPLE__) || defined(GLEW_APPLE_GLX)
int main (int argc, char** argv)
#else
int main (void)
#endif
{
  GLuint err;

#if defined(_WIN32) || !defined(__APPLE__) || defined(GLEW_APPLE_GLX)
  char* display = NULL;
  int visual = -1;

  if (glewParseArgs(argc-1, argv+1, &display, &visual))
  {
#if defined(_WIN32)
    fprintf(stderr, "Usage: glewinfo [-pf <id>]\n");
#else
    fprintf(stderr, "Usage: glewinfo [-display <display>] [-visual <id>]\n");
#endif
    return 1;
  }
#endif

#if defined(_WIN32)
  if (GL_TRUE == glewCreateContext(&visual))
#elif defined(__APPLE__) && !defined(GLEW_APPLE_GLX)
  if (GL_TRUE == glewCreateContext())
#else
  if (GL_TRUE == glewCreateContext(display, &visual))
#endif
  {
    fprintf(stderr, "Error: glewCreateContext failed\n");
    glewDestroyContext();
    return 1;
  }
  glewExperimental = GL_TRUE;
#ifdef GLEW_MX
  err = glewContextInit(glewGetContext());
#ifdef _WIN32
  err = err || wglewContextInit(wglewGetContext());
#elif !defined(__APPLE__) || defined(GLEW_APPLE_GLX)
  err = err || glxewContextInit(glxewGetContext());
#endif

#else
  err = glewInit();
#endif
  if (GLEW_OK != err)
  {
    fprintf(stderr, "Error [main]: glewInit failed: %s\n", glewGetErrorString(err));
    glewDestroyContext();
    return 1;
  }
#if defined(_WIN32)
  f = fopen("glewinfo.txt", "w");
  if (f == NULL) f = stdout;
#else
  f = stdout;
#endif
  fprintf(f, "---------------------------\n");
  fprintf(f, "    GLEW Extension Info\n");
  fprintf(f, "---------------------------\n\n");
  fprintf(f, "GLEW version %s\n", glewGetString(GLEW_VERSION));
#if defined(_WIN32)
  fprintf(f, "Reporting capabilities of pixelformat %d\n", visual);
#elif !defined(__APPLE__) || defined(GLEW_APPLE_GLX)
  fprintf(f, "Reporting capabilities of display %s, visual 0x%x\n", 
    display == NULL ? getenv("DISPLAY") : display, visual);
#endif
  fprintf(f, "Running on a %s from %s\n", 
	  glGetString(GL_RENDERER), glGetString(GL_VENDOR));
  fprintf(f, "OpenGL version %s is supported\n", glGetString(GL_VERSION));
  glewInfo();
#if defined(_WIN32)
  wglewInfo();
#else
  glxewInfo();
#endif
  if (f != stdout) fclose(f);
  glewDestroyContext();
  return 0;
}

/* ------------------------------------------------------------------------ */

#if defined(_WIN32) || !defined(__APPLE__) || defined(GLEW_APPLE_GLX)
GLboolean glewParseArgs (int argc, char** argv, char** display, int* visual)
{
  int p = 0;
  while (p < argc)
  {
#if defined(_WIN32)
    if (!strcmp(argv[p], "-pf") || !strcmp(argv[p], "-pixelformat"))
    {
      if (++p >= argc) return GL_TRUE;
      *display = 0;
      *visual = strtol(argv[p++], NULL, 0);
    }
    else
      return GL_TRUE;
#else
    if (!strcmp(argv[p], "-display"))
    {
      if (++p >= argc) return GL_TRUE;
      *display = argv[p++];
    }
    else if (!strcmp(argv[p], "-visual"))
    {
      if (++p >= argc) return GL_TRUE;
      *visual = (int)strtol(argv[p++], NULL, 0);
    }
    else
      return GL_TRUE;
#endif
  }
  return GL_FALSE;
}
#endif

/* ------------------------------------------------------------------------ */

#if defined(_WIN32)

HWND wnd = NULL;
HDC dc = NULL;
HGLRC rc = NULL;

GLboolean glewCreateContext (int* pixelformat)
{
  WNDCLASS wc;
  PIXELFORMATDESCRIPTOR pfd;
  /* register window class */
  ZeroMemory(&wc, sizeof(WNDCLASS));
  wc.hInstance = GetModuleHandle(NULL);
  wc.lpfnWndProc = DefWindowProc;
  wc.lpszClassName = "GLEW";
  if (0 == RegisterClass(&wc)) return GL_TRUE;
  /* create window */
  wnd = CreateWindow("GLEW", "GLEW", 0, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, 
                     CW_USEDEFAULT, NULL, NULL, GetModuleHandle(NULL), NULL);
  if (NULL == wnd) return GL_TRUE;
  /* get the device context */
  dc = GetDC(wnd);
  if (NULL == dc) return GL_TRUE;
  /* find pixel format */
  ZeroMemory(&pfd, sizeof(PIXELFORMATDESCRIPTOR));
  if (*pixelformat == -1) /* find default */
  {
    pfd.nSize = sizeof(PIXELFORMATDESCRIPTOR);
    pfd.nVersion = 1;
    pfd.dwFlags = PFD_DRAW_TO_WINDOW | PFD_SUPPORT_OPENGL;
    *pixelformat = ChoosePixelFormat(dc, &pfd);
    if (*pixelformat == 0) return GL_TRUE;
  }
  /* set the pixel format for the dc */
  if (FALSE == SetPixelFormat(dc, *pixelformat, &pfd)) return GL_TRUE;
  /* create rendering context */
  rc = wglCreateContext(dc);
  if (NULL == rc) return GL_TRUE;
  if (FALSE == wglMakeCurrent(dc, rc)) return GL_TRUE;
  return GL_FALSE;
}

void glewDestroyContext ()
{
  if (NULL != rc) wglMakeCurrent(NULL, NULL);
  if (NULL != rc) wglDeleteContext(rc);
  if (NULL != wnd && NULL != dc) ReleaseDC(wnd, dc);
  if (NULL != wnd) DestroyWindow(wnd);
  UnregisterClass("GLEW", GetModuleHandle(NULL));
}

/* ------------------------------------------------------------------------ */

#elif defined(__APPLE__) && !defined(GLEW_APPLE_GLX)

#include <AGL/agl.h>

AGLContext ctx, octx;

GLboolean glewCreateContext ()
{
  int attrib[] = { AGL_RGBA, AGL_NONE };
  AGLPixelFormat pf;
  /*int major, minor;
  SetPortWindowPort(wnd);
  aglGetVersion(&major, &minor);
  fprintf(stderr, "GL %d.%d\n", major, minor);*/
  pf = aglChoosePixelFormat(NULL, 0, attrib);
  if (NULL == pf) return GL_TRUE;
  ctx = aglCreateContext(pf, NULL);
  if (NULL == ctx || AGL_NO_ERROR != aglGetError()) return GL_TRUE;
  aglDestroyPixelFormat(pf);
  /*aglSetDrawable(ctx, GetWindowPort(wnd));*/
  octx = aglGetCurrentContext();
  if (GL_FALSE == aglSetCurrentContext(ctx)) return GL_TRUE;
  /* Needed for Regal on the Mac */
  #if defined(GLEW_REGAL) && defined(__APPLE__)
  RegalMakeCurrent(octx);
  #endif
  return GL_FALSE;
}

void glewDestroyContext ()
{
  aglSetCurrentContext(octx);
  if (NULL != ctx) aglDestroyContext(ctx);
}

/* ------------------------------------------------------------------------ */

#else /* __UNIX || (__APPLE__ && GLEW_APPLE_GLX) */

Display* dpy = NULL;
XVisualInfo* vi = NULL;
XVisualInfo* vis = NULL;
GLXContext ctx = NULL;
Window wnd = 0;
Colormap cmap = 0;

GLboolean glewCreateContext (const char* display, int* visual)
{
  int attrib[] = { GLX_RGBA, GLX_DOUBLEBUFFER, None };
  int erb, evb;
  XSetWindowAttributes swa;
  /* open display */
  dpy = XOpenDisplay(display);
  if (NULL == dpy) return GL_TRUE;
  /* query for glx */
  if (!glXQueryExtension(dpy, &erb, &evb)) return GL_TRUE;
  /* choose visual */
  if (*visual == -1)
  {
    vi = glXChooseVisual(dpy, DefaultScreen(dpy), attrib);
    if (NULL == vi) return GL_TRUE;
    *visual = (int)XVisualIDFromVisual(vi->visual);
  }
  else
  {
    int n_vis, i;
    vis = XGetVisualInfo(dpy, 0, NULL, &n_vis);
    for (i=0; i<n_vis; i++)
    {
      if ((int)XVisualIDFromVisual(vis[i].visual) == *visual)
        vi = &vis[i];
    }
    if (vi == NULL) return GL_TRUE;
  }
  /* create context */
  ctx = glXCreateContext(dpy, vi, None, True);
  if (NULL == ctx) return GL_TRUE;
  /* create window */
  /*wnd = XCreateSimpleWindow(dpy, RootWindow(dpy, vi->screen), 0, 0, 1, 1, 1, 0, 0);*/
  cmap = XCreateColormap(dpy, RootWindow(dpy, vi->screen), vi->visual, AllocNone);
  swa.border_pixel = 0;
  swa.colormap = cmap;
  wnd = XCreateWindow(dpy, RootWindow(dpy, vi->screen), 
                      0, 0, 1, 1, 0, vi->depth, InputOutput, vi->visual, 
                      CWBorderPixel | CWColormap, &swa);
  /* make context current */
  if (!glXMakeCurrent(dpy, wnd, ctx)) return GL_TRUE;
  return GL_FALSE;
}

void glewDestroyContext ()
{
  if (NULL != dpy && NULL != ctx) glXDestroyContext(dpy, ctx);
  if (NULL != dpy && 0 != wnd) XDestroyWindow(dpy, wnd);
  if (NULL != dpy && 0 != cmap) XFreeColormap(dpy, cmap);
  if (NULL != vis)
    XFree(vis);
  else if (NULL != vi)
    XFree(vi);
  if (NULL != dpy) XCloseDisplay(dpy);
}

#endif /* __UNIX || (__APPLE__ && GLEW_APPLE_GLX) */
