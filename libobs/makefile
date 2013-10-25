include ../config.mak

.PHONY: all default clean

all: default

SRCFILES=util/bmem.c \
	 util/base.c \
	 util/dstr.c \
	 util/lexer.c \
	 util/utf8.c \
	 util/text-lookup.c \
	 util/platform.c \
	 util/platform-windows.c \
	 util/config-file.c \
	 util/cf-lexer.c \
	 util/cf-parser.c \
	 graphics/axisang.c \
	 graphics/bounds.c \
	 graphics/effect.c \
	 graphics/effect-parser.c \
	 graphics/graphics.c \
	 graphics/graphics-imports.c \
	 graphics/math-extra.c \
	 graphics/matrix3.c \
	 graphics/matrix4.c \
	 graphics/plane.c \
	 graphics/quat.c \
	 graphics/shader-parser.c \
	 graphics/texture-render.c \
	 graphics/vec2.c \
	 graphics/vec3.c \
	 graphics/vec4.c \
	 media-io/video-io.c \
	 media-io/audio-io.c \
	 media-io/media-io.c \
	 media-io/format-conversion.c \
	 obs-module.c \
	 obs-output.c \
	 obs-source.c \
	 obs-scene.c \
	 obs-display.c \
	 obs-video.c \
	 obs.c

SONAME=../build/libobs.$(SOEXT)

OBJS += $(SRCFILES:%.c=%.$(OBJ))
LDFLAGS += -lpthread

default: $(SONAME)

.depend:
	@rm -f .depend
	@$(foreach SRC, $(addprefix $(SRCPATH)/, $(SRCFILES)), $(CCDEP) \
	       $(CPPFLAGS) $(SRC) \
	       -MT $(SRC:$(SRCPATH)/%.c=%.$(OBJ)) -MM 1>> .depend;)

$(SONAME): .depend $(OBJS)
	$(LD)$@ $(LDFLAGS) $(OBJS)

depend: .depend
ifneq ($(wildcard .depend),)
include .depend
endif

clean:
	rm -f $(OBJS) $(SONAME) *.a *.lib *.exp *.pdb .depend
