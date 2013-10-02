NAME = $(GLEW_NAME)
CC = cc
LD = cc
LDFLAGS.EXTRA = -L/usr/X11R6/lib
LDFLAGS.GL = -lXmu -lXi -lGL -lXext -lX11
LDFLAGS.STATIC = -Wl,-Bstatic
LDFLAGS.DYNAMIC = -Wl,-Bdynamic
NAME = GLEW
WARN = -Wall -W
POPT = -O2
CFLAGS.EXTRA += -fPIC
BIN.SUFFIX =
LIB.SONAME    = lib$(NAME).so.$(SO_MAJOR)
LIB.DEVLNK    = lib$(NAME).so
LIB.SHARED    = lib$(NAME).so.$(SO_VERSION)
LIB.STATIC    = lib$(NAME).a
LDFLAGS.SO    = -shared -Wl,-soname=$(LIB.SONAME)
LIB.SONAME.MX = lib$(NAME)mx.so.$(SO_MAJOR)
LIB.DEVLNK.MX = lib$(NAME)mx.so
LIB.SHARED.MX = lib$(NAME)mx.so.$(SO_VERSION)
LIB.STATIC.MX = lib$(NAME)mx.a
LDFLAGS.SO.MX = -shared -Wl,-soname=$(LIB.SONAME.MX)
