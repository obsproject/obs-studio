## Copyright (C) 2002-2008, Marcelo E. Magallon <mmagallo[]debian org>
## Copyright (C) 2002-2008, Milan Ikits <milan ikits[]ieee org>
##
## This program is distributed under the terms and conditions of the GNU
## General Public License Version 2 as published by the Free Software
## Foundation or, at your option, any later version.

include ../config/version

#GLEW_SPLIT_SOURCE = yes

SHELL = bash

### Use git repository for GL extension specifications

GIT_CLONE  ?= git clone --branch glew https://github.com/nigels-com/glfixes.git

###
### Conventional desktop OpenGL settings
###

REGISTRY     = registry/gl/specs
EXT          = extensions/gl
FILTER       = filter_gl_ext.sh
CORE         = core/gl
REGISTRY_URL = http://www.opengl.org/registry/

###
### Experimental OpenGL ES settings
###

# REGISTRY     = registry/gles
# EXT          = extensions/gles
# FILTER       = filter_gles_ext.sh
# CORE         = core/gles
# REGISTRY_URL = http://www.khronos.org/registry/gles/

BIN = bin
SRC = src
BLACKLIST = blacklist

GL_CORE_SPEC := $(CORE)/GL_VERSION*
GLX_CORE_SPEC := $(CORE)/GLX_VERSION*
ifeq (custom,$(MAKECMDGOALS))
#GL_CORE_SPEC  := $(shell grep GL_VERSION custom.txt    |                       perl -pi -e "s=^=$(CORE)/=g;")
GL_EXT_SPEC    := $(shell grep "^[ \t]*GL_" custom.txt  | grep -v GL_VERSION  | perl -pi -e "s=^=$(EXT)/=g;")
WGL_EXT_SPEC   := $(shell grep "^[ \t]*WGL_" custom.txt |                       perl -pi -e "s=^=$(EXT)/=g;")
#GLX_CORE_SPEC := $(shell grep GLX_VERSION custom.txt   |                       perl -pi -e "s=^=$(CORE)/=g;")
GLX_EXT_SPEC   := $(shell grep "^[ \t]*GLX_" custom.txt | grep -v GLX_VERSION | perl -pi -e "s=^=$(EXT)/=g;")
else
GL_EXT_SPEC := $(EXT)/GL_*
WGL_EXT_SPEC := $(EXT)/WGL_*
GLX_EXT_SPEC := $(EXT)/GLX_*
endif

PARSE_SPEC = parse_spec.pl
SYSTEM = $(strip $(shell uname -s))

TOP = ..
I.DEST = $(TOP)/include/GL
S.DEST = $(TOP)/src
D.DEST = $(TOP)/doc
B.DEST = $(TOP)/build

I.TARGETS = \
	$(I.DEST)/glew.h \
	$(I.DEST)/wglew.h \
	$(I.DEST)/glxew.h

ifeq (yes,$(GLEW_SPLIT_SOURCE))
S.TARGETS = \
	$(S.DEST)/glew_def.c \
	$(S.DEST)/glew_init.c \
	$(S.DEST)/glew_str.c \
	$(S.DEST)/glewinfo.c
else
S.TARGETS = \
	$(S.DEST)/glew.c \
	$(S.DEST)/glewinfo.c
endif

D.TARGETS = \
	$(D.DEST)/index.html \
	$(D.DEST)/install.html \
	$(D.DEST)/basic.html \
	$(D.DEST)/advanced.html \
	$(D.DEST)/build.html \
	$(D.DEST)/credits.html \
	$(D.DEST)/log.html \
	$(D.DEST)/glew.html \
	$(D.DEST)/wglew.html \
	$(D.DEST)/glxew.html

B.TARGETS = \
	$(B.DEST)/glew.rc \
	$(B.DEST)/glewinfo.rc \
	$(B.DEST)/visualinfo.rc

all custom: $(I.TARGETS) $(S.TARGETS) $(D.TARGETS) $(B.TARGETS)

registry: $(REGISTRY)/.dummy
ext: $(EXT)/.dummy

$(REGISTRY)/.dummy:
	@echo "--------------------------------------------------------------------"
	@echo "Downloading registry"
	@echo "--------------------------------------------------------------------"
	$(GIT_CLONE) registry
	touch $@

$(EXT)/.dummy: $(REGISTRY)/.dummy
	@echo "--------------------------------------------------------------------"
	@echo "Creating descriptors"
	@echo "--------------------------------------------------------------------"
	rm -rf $(EXT)
	$(BIN)/update_ext.sh $(EXT) $(REGISTRY) $(BLACKLIST)
	$(BIN)/$(FILTER)     $(EXT)
ifeq ($(patsubst Darwin%,Darwin,$(SYSTEM)), Darwin)
	find $(CORE) -maxdepth 1 -type f | grep -v VERSION | grep -v "~" | \
	xargs -J % cp % $(EXT)
else
	find $(CORE) -maxdepth 1 -type f | grep -v VERSION | grep -v "~" | \
	xargs cp --target-directory=$(EXT)
endif
	touch $@

$(I.DEST)/glew.h: $(EXT)/.dummy
	@echo "--------------------------------------------------------------------"
	@echo "Creating glew.h"
	@echo "--------------------------------------------------------------------"
	test -d $(I.DEST) || mkdir -p $(I.DEST)
	cp -f $(SRC)/glew_license.h $@
	cat $(SRC)/mesa_license.h >> $@
	cat $(SRC)/khronos_license.h >> $@
	cat $(SRC)/glew_head.h >> $@
	$(BIN)/make_header.pl GLAPIENTRY GL $(GL_CORE_SPEC) >> $@
	$(BIN)/make_header.pl GLAPIENTRY GL $(GL_EXT_SPEC) >> $@
	echo -e "/* ------------------------------------------------------------------------- */\n\n#if defined(GLEW_MX) && defined(_WIN32)\n#define GLEW_FUN_EXPORT\n#else\n#define GLEW_FUN_EXPORT GLEWAPI\n#endif /* GLEW_MX */\n" >> $@
	echo -e "#if defined(GLEW_MX)\n#define GLEW_VAR_EXPORT\n#else\n#define GLEW_VAR_EXPORT GLEWAPI\n#endif /* GLEW_MX */\n" >> $@
	echo -e "#if defined(GLEW_MX) && defined(_WIN32)\nstruct GLEWContextStruct\n{\n#endif /* GLEW_MX */" >> $@
	$(BIN)/make_struct_fun.pl GLEW_FUN_EXPORT $(GL_CORE_SPEC) $(GL_EXT_SPEC) >> $@
	echo -e "\n#if defined(GLEW_MX) && !defined(_WIN32)\nstruct GLEWContextStruct\n{\n#endif /* GLEW_MX */\n" >> $@
	$(BIN)/make_struct_var.pl GLEW_VAR_EXPORT $(GL_CORE_SPEC) $(GL_EXT_SPEC) >> $@
	echo -e "\n#ifdef GLEW_MX\n}; /* GLEWContextStruct */\n#endif /* GLEW_MX */\n" >> $@
	perl -e "s/GLEW_VAR_EXPORT GLboolean __GLEW_VERSION_1_2;/GLEW_VAR_EXPORT GLboolean __GLEW_VERSION_1_1;\nGLEW_VAR_EXPORT GLboolean __GLEW_VERSION_1_2;/" -pi $@
	rm -f $@.bak
	cat $(SRC)/glew_tail.h >> $@

$(I.DEST)/wglew.h: $(EXT)/.dummy
	@echo "--------------------------------------------------------------------"
	@echo "Creating wglew.h"
	@echo "--------------------------------------------------------------------"
	cp -f $(SRC)/glew_license.h $@
	cat $(SRC)/khronos_license.h >> $@
	cat $(SRC)/wglew_head.h >> $@
	$(BIN)/make_header.pl WINAPI WGL $(WGL_EXT_SPEC) >> $@
	cat $(SRC)/wglew_mid.h >> $@
	echo -e "\n#ifdef GLEW_MX\nstruct WGLEWContextStruct\n{\n#endif /* GLEW_MX */" >> $@
	$(BIN)/make_struct_fun.pl WGLEW_FUN_EXPORT $(WGL_EXT_SPEC) >> $@
	$(BIN)/make_struct_var.pl WGLEW_VAR_EXPORT $(WGL_EXT_SPEC) >> $@
	echo -e "\n#ifdef GLEW_MX\n}; /* WGLEWContextStruct */\n#endif /* GLEW_MX */\n" >> $@
	cat $(SRC)/wglew_tail.h >> $@

$(I.DEST)/glxew.h: $(EXT)/.dummy
	@echo "--------------------------------------------------------------------"
	@echo "Creating glxew.h"
	@echo "--------------------------------------------------------------------"
	cp -f $(SRC)/glew_license.h $@
	cat $(SRC)/mesa_license.h >> $@
	cat $(SRC)/khronos_license.h >> $@
	cat $(SRC)/glxew_head.h >> $@
	$(BIN)/make_header.pl "" GLX $(GLX_CORE_SPEC) >> $@
	$(BIN)/make_header.pl "" GLX $(GLX_EXT_SPEC) >> $@
	cat $(SRC)/glxew_mid.h >> $@
	$(BIN)/make_struct_fun.pl GLXEW_FUN_EXPORT $(GLX_CORE_SPEC) $(GLX_EXT_SPEC) >> $@
	echo -e "\n#if defined(GLEW_MX)\nstruct GLXEWContextStruct\n{\n#endif /* GLEW_MX */\n" >> $@
	$(BIN)/make_struct_var.pl GLXEW_VAR_EXPORT $(GLX_CORE_SPEC) $(GLX_EXT_SPEC) >> $@
	echo -e "\n#ifdef GLEW_MX\n}; /* GLXEWContextStruct */\n#endif /* GLEW_MX */\n" >> $@
	perl -e "s/GLXEW_VAR_EXPORT GLboolean __GLXEW_VERSION_1_2;/GLXEW_VAR_EXPORT GLboolean __GLXEW_VERSION_1_0;\nGLXEW_VAR_EXPORT GLboolean __GLXEW_VERSION_1_1;\nGLXEW_VAR_EXPORT GLboolean __GLXEW_VERSION_1_2;/" -pi $@
	cat $(SRC)/glxew_tail.h >> $@

$(S.DEST)/glew.c: $(EXT)/.dummy
	@echo "--------------------------------------------------------------------"
	@echo "Creating glew.c"
	@echo "--------------------------------------------------------------------"
	cp -f $(SRC)/glew_license.h $@
	cat $(SRC)/glew_head.c >> $@
	echo -e "\n#if !defined(_WIN32) || !defined(GLEW_MX)" >> $@
	$(BIN)/make_def_fun.pl GL $(GL_CORE_SPEC) >> $@
	$(BIN)/make_def_fun.pl GL $(GL_EXT_SPEC) >> $@
	echo -e "\n#endif /* !WIN32 || !GLEW_MX */" >> $@
	echo -e "\n#if !defined(GLEW_MX)" >> $@;
	echo -e "\nGLboolean __GLEW_VERSION_1_1 = GL_FALSE;" >> $@
	$(BIN)/make_def_var.pl GL $(GL_CORE_SPEC) >> $@
	$(BIN)/make_def_var.pl GL $(GL_EXT_SPEC) >> $@
	echo -e "\n#endif /* !GLEW_MX */\n" >> $@;
	$(BIN)/make_init.pl GL $(GL_CORE_SPEC) >> $@
	$(BIN)/make_init.pl GL $(GL_EXT_SPEC) >> $@
	cat $(SRC)/glew_init_gl.c >> $@
	$(BIN)/make_list.pl $(GL_CORE_SPEC) | grep -v '\"GL_VERSION' >> $@
	$(BIN)/make_list.pl $(GL_EXT_SPEC) >> $@
	echo -e "\n  return GLEW_OK;\n}\n" >> $@
	echo -e "\n#if defined(_WIN32)" >> $@
	echo -e "\n#if !defined(GLEW_MX)" >> $@
	$(BIN)/make_def_fun.pl WGL $(WGL_EXT_SPEC) >> $@
	$(BIN)/make_def_var.pl WGL $(WGL_EXT_SPEC) >> $@
	echo -e "\n#endif /* !GLEW_MX */\n" >> $@;
	$(BIN)/make_init.pl WGL $(WGL_EXT_SPEC) >> $@
	cat $(SRC)/glew_init_wgl.c >> $@
	$(BIN)/make_list.pl $(WGL_EXT_SPEC) >> $@
	echo -e "\n  return GLEW_OK;\n}" >> $@;
	echo -e "\n#elif !defined(__ANDROID__) && !defined(__native_client__) && (!defined(__APPLE__) || defined(GLEW_APPLE_GLX))" >> $@
	$(BIN)/make_def_fun.pl GLX $(GLX_CORE_SPEC) >> $@
	$(BIN)/make_def_fun.pl GLX $(GLX_EXT_SPEC) >> $@
	echo -e "\n#if !defined(GLEW_MX)" >> $@;
	echo -e "\nGLboolean __GLXEW_VERSION_1_0 = GL_FALSE;" >> $@
	echo -e "GLboolean __GLXEW_VERSION_1_1 = GL_FALSE;" >> $@
	$(BIN)/make_def_var.pl GLX $(GLX_CORE_SPEC) >> $@
	$(BIN)/make_def_var.pl GLX $(GLX_EXT_SPEC) >> $@
	echo -e "\n#endif /* !GLEW_MX */\n" >> $@;
	$(BIN)/make_init.pl GLX $(GLX_CORE_SPEC) >> $@
	$(BIN)/make_init.pl GLX $(GLX_EXT_SPEC) >> $@
	cat $(SRC)/glew_init_glx.c >> $@
	$(BIN)/make_list.pl $(CORE)/GLX_VERSION_1_3 | grep -v '\"GLX_VERSION' >> $@
	$(BIN)/make_list.pl $(GLX_EXT_SPEC) >> $@
	echo -e "\n  return GLEW_OK;\n}" >> $@
	echo -e "\n#endif /* !defined(__ANDROID__) && !defined(__native_client__) && (!defined(__APPLE__) || defined(GLEW_APPLE_GLX)) */\n" >> $@;
	cat $(SRC)/glew_init_tail.c >> $@
	cat $(SRC)/glew_str_head.c >> $@
	$(BIN)/make_str.pl $(GL_CORE_SPEC) $(GL_EXT_SPEC) >> $@
	cat $(SRC)/glew_str_wgl.c >> $@
	$(BIN)/make_str.pl $(WGL_EXT_SPEC) >> $@
	cat $(SRC)/glew_str_glx.c >> $@
	$(BIN)/make_str.pl $(GLX_CORE_SPEC) $(GLX_EXT_SPEC) >> $@
	cat $(SRC)/glew_str_tail.c >> $@
	perl -e "s/GLEW_VERSION_STRING/$(GLEW_VERSION)/g" -pi $@
	perl -e "s/GLEW_VERSION_MAJOR_STRING/$(GLEW_MAJOR)/g" -pi $@
	perl -e "s/GLEW_VERSION_MINOR_STRING/$(GLEW_MINOR)/g" -pi $@
	perl -e "s/GLEW_VERSION_MICRO_STRING/$(GLEW_MICRO)/g" -pi $@
	perl -e "s/GLEW_ARB_vertex_shader = !_glewInit_GL_ARB_vertex_shader\(GLEW_CONTEXT_ARG_VAR_INIT\);/{ GLEW_ARB_vertex_shader = !_glewInit_GL_ARB_vertex_shader(GLEW_CONTEXT_ARG_VAR_INIT); _glewInit_GL_ARB_vertex_program(GLEW_CONTEXT_ARG_VAR_INIT); }/g" -pi $@
	perl -e "s/\(\(glColorSubTable = /((glBlendEquation = (PFNGLBLENDEQUATIONPROC)glewGetProcAddress((const GLubyte*)\"glBlendEquation\")) == NULL) || r;\n  r = ((glColorSubTable = /g" -pi $@
	rm -f $@.bak

$(S.DEST)/glew_def.c: $(EXT)/.dummy
	cp -f $(SRC)/glew_license.h $@
	echo -e "#include \"glew_utils.h\"\n\n#if !defined(_WIN32) || !defined(GLEW_MX)" >> $@
	$(BIN)/make_def_fun.pl GL $(GL_CORE_SPEC) >> $@
	$(BIN)/make_def_fun.pl GL $(GL_EXT_SPEC) >> $@
	echo -e "\n#endif /* !WIN32 || !GLEW_MX */" >> $@
	echo -e "\n#if !defined(GLEW_MX)\n\nGLboolean __GLEW_VERSION_1_1 = GL_FALSE;" >> $@
	$(BIN)/make_def_var.pl GL $(GL_CORE_SPEC) >> $@
	$(BIN)/make_def_var.pl GL $(GL_EXT_SPEC) >> $@
	echo -e "\n#if defined(_WIN32)" >> $@
	$(BIN)/make_def_fun.pl WGL $(WGL_EXT_SPEC) >> $@
	$(BIN)/make_def_var.pl WGL $(WGL_EXT_SPEC) >> $@
	echo -e "\n#endif /* _WIN32 */" >> $@
	echo -e "\n#endif /* !GLEW_MX */" >> $@;
	echo -e "\n#if !defined(_WIN32) && !defined(__ANDROID__) && !defined(__native_client__) && (!defined(__APPLE__) || defined(GLEW_APPLE_GLX))" >> $@
	$(BIN)/make_def_fun.pl GLX $(GLX_CORE_SPEC) >> $@
	$(BIN)/make_def_fun.pl GLX $(GLX_EXT_SPEC) >> $@
	echo -e "\n#if !defined(GLEW_MX)" >> $@;
	echo -e "\nGLboolean __GLXEW_VERSION_1_0 = GL_FALSE;" >> $@
	echo -e "GLboolean __GLXEW_VERSION_1_1 = GL_FALSE;" >> $@
	$(BIN)/make_def_var.pl GLX $(GLX_CORE_SPEC) >> $@
	$(BIN)/make_def_var.pl GLX $(GLX_EXT_SPEC) >> $@
	echo -e "\n#endif /* !GLEW_MX */" >> $@;
	echo -e "\n#endif /* !defined(_WIN32) && !defined(__ANDROID__) && !defined(__native_client__) && (!defined(__APPLE__) || defined(GLEW_APPLE_GLX)) */" >> $@;
	rm -f $@.bak

$(S.DEST)/glew_init.c: $(EXT)/.dummy
	cp -f $(SRC)/glew_license.h $@
	echo -e "#include \"glew_utils.h\"\n" >> $@
	$(BIN)/make_init.pl GL $(GL_CORE_SPEC) >> $@
	$(BIN)/make_init.pl GL $(GL_EXT_SPEC) >> $@
	cat $(SRC)/glew_init_gl.c >> $@
	$(BIN)/make_list.pl $(GL_CORE_SPEC) | grep -v '\"GL_VERSION' >> $@
	$(BIN)/make_list.pl $(GL_EXT_SPEC) >> $@
	echo -e "\n  return GLEW_OK;\n}\n\n#if defined(_WIN32)\n" >> $@;
	$(BIN)/make_init.pl WGL $(WGL_EXT_SPEC) >> $@
	cat $(SRC)/glew_init_wgl.c >> $@
	$(BIN)/make_list.pl $(WGL_EXT_SPEC) >> $@
	echo -e "\n  return GLEW_OK;\n}\n\n" >> $@;
	echo -e "\n#elif !defined(__APPLE__) || defined(GLEW_APPLE_GLX)\n" >> $@
	$(BIN)/make_init.pl GLX $(GLX_CORE_SPEC) >> $@
	$(BIN)/make_init.pl GLX $(GLX_EXT_SPEC) >> $@
	cat $(SRC)/glew_init_glx.c >> $@
	$(BIN)/make_list.pl $(CORE)/GLX_VERSION_1_3 | grep -v '\"GLX_VERSION' >> $@
	$(BIN)/make_list.pl $(GLX_EXT_SPEC) >> $@
	echo -e "\n  return GLEW_OK;\n}\n\n#endif /* !__APPLE__ || GLEW_APPLE_GLX */\n" >> $@;
	cat $(SRC)/glew_init_tail.c >> $@
	perl -e "s/GLEW_VERSION_STRING/$(GLEW_VERSION)/g" -pi $@
	perl -e "s/GLEW_VERSION_MAJOR_STRING/$(GLEW_MAJOR)/g" -pi $@
	perl -e "s/GLEW_VERSION_MINOR_STRING/$(GLEW_MINOR)/g" -pi $@
	perl -e "s/GLEW_VERSION_MICRO_STRING/$(GLEW_MICRO)/g" -pi $@
	perl -e "s/GLEW_ARB_vertex_shader = !_glewInit_GL_ARB_vertex_shader\(GLEW_CONTEXT_ARG_VAR_INIT\);/{ GLEW_ARB_vertex_shader = !_glewInit_GL_ARB_vertex_shader(GLEW_CONTEXT_ARG_VAR_INIT); _glewInit_GL_ARB_vertex_program(GLEW_CONTEXT_ARG_VAR_INIT); }/g" -pi $@
	perl -e "s/\(\(glBlendColor = /((glBlendEquation = (PFNGLBLENDEQUATIONPROC)glewGetProcAddress((const GLubyte*)\"glBlendEquation\")) == NULL) || r;\n  r = ((glBlendColor = /g" -pi $@
	rm -f $@.bak

$(S.DEST)/glew_str.c: $(EXT)/.dummy
	cp -f $(SRC)/glew_license.h $@
	echo -e "\n#include \"glew_utils.h\"\n" >> $@
	cat $(SRC)/glew_str_head.c >> $@
	$(BIN)/make_str.pl $(GL_CORE_SPEC) $(GL_EXT_SPEC) >> $@
	cat $(SRC)/glew_str_wgl.c >> $@
	$(BIN)/make_str.pl $(WGL_EXT_SPEC) >> $@
	cat $(SRC)/glew_str_glx.c >> $@
	$(BIN)/make_str.pl $(GLX_CORE_SPEC) $(GLX_EXT_SPEC) >> $@
	cat $(SRC)/glew_str_tail.c >> $@
#	perl -e "s/GLEW_VERSION_STRING/$(GLEW_VERSION)/g" -pi $@
#	perl -e "s/GLEW_ARB_vertex_shader = !_glewInit_GL_ARB_vertex_shader\(GLEW_CONTEXT_ARG_VAR_INIT\);/{ GLEW_ARB_vertex_shader = !_glewInit_GL_ARB_vertex_shader(GLEW_CONTEXT_ARG_VAR_INIT); _glewInit_GL_ARB_vertex_program(GLEW_CONTEXT_ARG_VAR_INIT); }/g" -pi $@
	rm -f $@.bak

$(S.DEST)/glewinfo.c: $(EXT)/.dummy
	@echo "--------------------------------------------------------------------"
	@echo "Creating glewinfo.c"
	@echo "--------------------------------------------------------------------"
	cp -f $(SRC)/glew_license.h $@
	cat $(SRC)/glewinfo_head.c >> $@
	$(BIN)/make_info.pl $(GL_CORE_SPEC) >> $@
	$(BIN)/make_info.pl $(GL_EXT_SPEC) >> $@
	echo -e "#ifdef _WIN32\n" >> $@
	$(BIN)/make_info.pl $(WGL_EXT_SPEC) >> $@
	echo -e "#else /* _UNIX */\n" >> $@
	$(BIN)/make_info.pl $(GLX_CORE_SPEC) >> $@
	$(BIN)/make_info.pl $(GLX_EXT_SPEC) >> $@
	echo -e "#endif /* _WIN32 */\n" >> $@

	cat $(SRC)/glewinfo_gl.c >> $@
	$(BIN)/make_info_list.pl $(GL_CORE_SPEC) >> $@
	$(BIN)/make_info_list.pl $(GL_EXT_SPEC) >> $@
	cat $(SRC)/glewinfo_wgl.c >> $@
	$(BIN)/make_info_list.pl $(WGL_EXT_SPEC) >> $@
	cat $(SRC)/glewinfo_glx.c >> $@
	$(BIN)/make_info_list.pl $(GLX_CORE_SPEC) >> $@
	$(BIN)/make_info_list.pl $(GLX_EXT_SPEC) >> $@
	cat $(SRC)/glewinfo_tail.c >> $@
	perl -e 's/"glColorSubTable"/"glBlendEquation", glBlendEquation == NULL);\n  glewInfoFunc("glColorSubTable"/g' -pi $@
	rm -f $@.bak

# Update documentation

$(D.DEST)/%.html: doc/%.html
	@echo "--------------------------------------------------------------------"
	@echo "Creating $(@F)"
	@echo "--------------------------------------------------------------------"
	cat $(SRC)/header.html $< $(SRC)/footer.html | \
	perl -pe 's#<a href="$(@F)">(.*)</a>#\1#' > $@

$(D.DEST)/glew.html: $(EXT)/.dummy
	@echo "--------------------------------------------------------------------"
	@echo "Creating glew.html"
	@echo "--------------------------------------------------------------------"
	cp -f $(SRC)/header.html $@
	echo -e "<h2>Supported OpenGL Extensions</h2>\n" >> $@
	$(BIN)/make_html.pl $(GL_EXT_SPEC) >> $@
	cat $(SRC)/footer.html >> $@
	perl -i -pe 's#<a href="$(@F)">(.*)</a>#\1#' $@

$(D.DEST)/wglew.html: $(EXT)/.dummy
	@echo "--------------------------------------------------------------------"
	@echo "Creating wglew.html"
	@echo "--------------------------------------------------------------------"
	cp -f $(SRC)/header.html $@
	echo -e "<h2>Supported WGL Extensions</h2>\n" >> $@
	$(BIN)/make_html.pl $(WGL_EXT_SPEC) >> $@
	cat $(SRC)/footer.html >> $@
	perl -i -pe 's#<a href="$(@F)">(.*)</a>#\1#' $@

$(D.DEST)/glxew.html: $(EXT)/.dummy
	@echo "--------------------------------------------------------------------"
	@echo "Creating glxew.html"
	@echo "--------------------------------------------------------------------"
	cp -f $(SRC)/header.html $@
	echo -e "<h2>Supported GLX Extensions</h2>\n" >> $@
	$(BIN)/make_html.pl $(GLX_EXT_SPEC) >> $@
	cat $(SRC)/footer.html >> $@
	perl -i -pe 's#<a href="$(@F)">(.*)</a>#\1#' $@

$(B.DEST)/%.rc: src/%.rc $(EXT)/.dummy
	perl -e "s/GLEW_MAJOR/$(GLEW_MAJOR)/g;s/GLEW_MINOR/$(GLEW_MINOR)/g;s/GLEW_MICRO/$(GLEW_MICRO)/g;" -p $< > $@

clean:
	rm -rf $(I.TARGETS) $(S.TARGETS) $(D.TARGETS) $(B.TARGETS)

clobber: clean
	rm -rf $(EXT)

destroy: clobber
	rm -rf registry
