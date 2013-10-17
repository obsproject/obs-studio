include ../config.mak

.PHONY: all default clean

all: default

SRCFILES=gl-subsystem.c \
	 gl-windows.c \
	 gl-vertexbuffer.c \
	 gl-texturecube.c \
	 gl-texture2d.c \
	 gl-stagesurf.c \
	 gl-shaderparser.c \
	 gl-shader.c \
	 gl-indexbuffer.c \
	 gl-zstencil.c \
	 gl-helpers.c

SONAME=../build/libobs-opengl.$(SOEXT)

OBJS += $(SRCFILES:%.c=%.$(OBJ))

CPPFLAGS += -iquote../libobs -DGLEW_STATIC
LDFLAGS += -Lglew/lib -Wl,-Bstatic -lglew32 -Wl,-Bdynamic -lopengl32 \
	   -Wl,--subsystem,windows -mwindows -L../build -lobs -lpthread

default: makeglew $(SONAME)

makeglew: glew
	make -C glew glew.lib

.depend:
	@rm -f .depend
	@$(foreach SRC, $(addprefix $(SRCPATH)/, $(SRCFILES)), $(CCDEP) \
	       $(CPPFLAGS) $(SRC) \
	       -MT $(SRC:$(SRCPATH)/%.c=%.$(OBJ)) -MM 1>> .depend;)

$(SONAME): .depend $(OBJS)
	$(LD)$@ $(OBJS) $(LDFLAGS)

depend: .depend
ifneq ($(wildcard .depend),)
include .depend
endif

clean:
	rm -f $(OBJS) $(SONAME) *.a *.lib *.exp *.pdb .depend
	make clean -C glew
